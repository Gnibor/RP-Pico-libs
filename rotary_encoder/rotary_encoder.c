#include "rotary_encoder.h"

#include <limits.h>
#include <stddef.h>

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"


/*
 * Übergangstabelle:
 *
 * Index:
 *     alter Zustand << 2 | neuer Zustand
 *
 * Ergebnis:
 *     +1 = gültiger Übergang rechts
 *     -1 = gültiger Übergang links
 *      0 = identisch oder ungültig
 */
static const int8_t rotary_transition_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

/*
 * Ordnet jedem GPIO seine Encoderinstanz zu.
 *
 * Der Pico-SDK-GPIO-Callback ist global. Über diese Tabelle können
 * trotzdem mehrere Encoderinstanzen denselben Callback verwenden.
 */
static rotary_t *rotary_gpio_owner[NUM_BANK0_GPIOS] = { NULL };


/**
 * @brief Liest beide Encoderkanäle als gemeinsamen 2-Bit-Zustand.
 */
static inline uint8_t rotary_read_state(
    const rotary_t *const rotary
)
{
    return
        ((uint8_t)gpio_get(rotary->config.gpio_clk) << 1u) |
        (uint8_t)gpio_get(rotary->config.gpio_dt);
}


/**
 * @brief Liefert den aktuell erforderlichen positiven Schwellwert.
 */
static inline int16_t rotary_positive_threshold(
    const rotary_t *const rotary
)
{
    int16_t threshold = rotary->config.steps_per_detent;

    /* Richtungswechsel erst nach zusätzlicher Gegenbewegung akzeptieren. */
    if (rotary->internal.direction_bias < 0)
    {
        threshold += rotary->config.hysteresis;
    }

    return threshold;
}


/**
 * @brief Liefert den aktuell erforderlichen negativen Schwellwert.
 */
static inline int16_t rotary_negative_threshold(
    const rotary_t *const rotary
)
{
    int16_t threshold = rotary->config.steps_per_detent;

    /* Richtungswechsel erst nach zusätzlicher Gegenbewegung akzeptieren. */
    if (rotary->internal.direction_bias > 0)
    {
        threshold += rotary->config.hysteresis;
    }

    return threshold;
}


/**
 * @brief Wertet den aktuellen Bewegungsakkumulator aus.
 */
static void rotary_process_accumulator(
    rotary_t *const rotary
)
{
    /*
     * int16_t verhindert Probleme bei der Schwellwertberechnung.
     * Der gespeicherte Akkumulator selbst bleibt gemäß Header int8_t.
     */
    int16_t accumulator = rotary->internal.accumulator;

    /* Positive Bewegung auswerten. */
    int16_t threshold = rotary_positive_threshold(rotary);

    while (accumulator >= threshold)
    {
        rotary->output.position++;
        rotary->output.delta++;

        accumulator -= threshold;
        rotary->internal.direction_bias = 1;

        /*
         * Nach akzeptierter Richtungsumkehr gilt für weitere Schritte
         * wieder der normale Schwellwert.
         */
        threshold = rotary->config.steps_per_detent;
    }

    /* Negative Bewegung auswerten. */
    threshold = rotary_negative_threshold(rotary);

    while (accumulator <= -threshold)
    {
        rotary->output.position--;
        rotary->output.delta--;

        accumulator += threshold;
        rotary->internal.direction_bias = -1;

        threshold = rotary->config.steps_per_detent;
    }

    rotary->internal.accumulator = (int8_t)accumulator;
}


/**
 * @brief Verarbeitet einen Encoderkanal-Interrupt.
 */
static void rotary_process_rotation(
    rotary_t *const rotary
)
{
    const uint8_t current_state = rotary_read_state(rotary);

    /* Kein tatsächlicher Zustandswechsel. */
    if (current_state == rotary->internal.state)
    {
        return;
    }

    const uint8_t transition =
        (uint8_t)(
            (rotary->internal.state << 2u) |
            current_state
        );

    rotary->internal.state = current_state;

    const int8_t movement =
        rotary_transition_table[transition];

    /* Nur gültige Quadraturübergänge übernehmen. */
    if (movement != 0)
    {
        rotary->internal.accumulator += movement;
    }

    /*
     * Schritte ausschließlich an der Rastposition auswerten.
     * Den Akkumulator nicht löschen: Er enthält gegebenenfalls
     * die noch nicht überwundene Richtungs-Hysterese.
     */
    if (current_state == 0x03u)
    {
        rotary_process_accumulator(rotary);
    }
}

/**
 * @brief Verarbeitet einen Taster-Interrupt.
 */
static void rotary_process_switch(
    rotary_t *const rotary
)
{
    const bool raw_switch_state =
        gpio_get(rotary->config.gpio_sw);

    /* Identischen Zustand ignorieren. */
    if (raw_switch_state == rotary->internal.switch_state)
    {
        return;
    }

    rotary->internal.switch_state = raw_switch_state;

    /*
     * Pull-up-Logik:
     *     LOW  = gedrückt
     *     HIGH = losgelassen
     */
    const bool pressed = !raw_switch_state;

    rotary->output.button = pressed;

    if (pressed)
    {
        rotary->output.pressed = true;
    }
    else
    {
        rotary->output.released = true;
    }
}


/**
 * @brief Gemeinsamer Pico-SDK-GPIO-Callback.
 */
static void rotary_gpio_irq_handler(
    const uint gpio,
    const uint32_t events
)
{
    (void)events;

    if (gpio >= NUM_BANK0_GPIOS)
    {
        return;
    }

    rotary_t *const rotary = rotary_gpio_owner[gpio];

    if (rotary == NULL)
    {
        return;
    }

    if (gpio == rotary->config.gpio_sw)
    {
        rotary_process_switch(rotary);
        return;
    }

    rotary_process_rotation(rotary);
}


/**
 * @brief Prüft die Encoderkonfiguration.
 */
static bool rotary_config_is_valid(
    const rotary_config_t *const config
)
{
    if (config == NULL)
    {
        return false;
    }

    /* GPIOs müssen im gültigen Bereich liegen. */
    if (
        config->gpio_clk >= NUM_BANK0_GPIOS ||
        config->gpio_dt  >= NUM_BANK0_GPIOS ||
        config->gpio_sw  >= NUM_BANK0_GPIOS
    )
    {
        return false;
    }

    /* Jeder Anschluss benötigt einen eigenen GPIO. */
    if (
        config->gpio_clk == config->gpio_dt ||
        config->gpio_clk == config->gpio_sw ||
        config->gpio_dt  == config->gpio_sw
    )
    {
        return false;
    }

    /* Eine Schrittweite von null kann niemals auslösen. */
    if (config->steps_per_detent == 0u)
    {
        return false;
    }

    /*
     * Akkumulator ist int8_t.
     * Normaler Schwellwert plus Hysterese muss darstellbar bleiben.
     */
    if (
        (uint16_t)config->steps_per_detent +
        (uint16_t)config->hysteresis >
        INT8_MAX
    )
    {
        return false;
    }

    return true;
}


/**
 * @brief Prüft, ob die GPIOs bereits einer anderen Instanz gehören.
 */
static bool rotary_gpio_is_available(
    const rotary_t *const rotary
)
{
    const uint8_t gpio_clk = rotary->config.gpio_clk;
    const uint8_t gpio_dt  = rotary->config.gpio_dt;
    const uint8_t gpio_sw  = rotary->config.gpio_sw;

    return
        (
            rotary_gpio_owner[gpio_clk] == NULL ||
            rotary_gpio_owner[gpio_clk] == rotary
        ) &&
        (
            rotary_gpio_owner[gpio_dt] == NULL ||
            rotary_gpio_owner[gpio_dt] == rotary
        ) &&
        (
            rotary_gpio_owner[gpio_sw] == NULL ||
            rotary_gpio_owner[gpio_sw] == rotary
        );
}


/**
 * @brief Initialisiert einen Eingang mit internem Pull-up.
 */
static void rotary_gpio_init(
    const uint8_t gpio
)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}


bool rotary_init(
    rotary_t *const rotary
)
{
    if (
        rotary == NULL ||
        !rotary_config_is_valid(&rotary->config)
    )
    {
        return false;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    if (!rotary_gpio_is_available(rotary))
    {
        restore_interrupts(interrupt_state);
        return false;
    }

    /* GPIOs zunächst konfigurieren. */
    rotary_gpio_init(rotary->config.gpio_clk);
    rotary_gpio_init(rotary->config.gpio_dt);
    rotary_gpio_init(rotary->config.gpio_sw);

    /* Laufzeitdaten initialisieren. */
    rotary->output.position = 0;
    rotary->output.delta = 0;

    rotary->output.button =
        !gpio_get(rotary->config.gpio_sw);

    rotary->output.pressed = false;
    rotary->output.released = false;

    rotary->internal.state =
        rotary_read_state(rotary);

    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;

    rotary->internal.switch_state =
        gpio_get(rotary->config.gpio_sw);

    /* GPIOs der Instanz zuordnen. */
    rotary_gpio_owner[rotary->config.gpio_clk] = rotary;
    rotary_gpio_owner[rotary->config.gpio_dt]  = rotary;
    rotary_gpio_owner[rotary->config.gpio_sw]  = rotary;

    restore_interrupts(interrupt_state);

    /*
     * Einmal den globalen Callback registrieren.
     * Weitere GPIOs verwenden anschließend denselben Callback.
     */
    gpio_set_irq_enabled_with_callback(
        rotary->config.gpio_clk,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &rotary_gpio_irq_handler
    );

    gpio_set_irq_enabled(
        rotary->config.gpio_dt,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true
    );

    gpio_set_irq_enabled(
        rotary->config.gpio_sw,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true
    );

    return true;
}


void rotary_reset(
    rotary_t *const rotary
)
{
    if (rotary == NULL)
    {
        return;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    rotary->output.position = 0;
    rotary->output.delta = 0;

    rotary->output.button =
        !gpio_get(rotary->config.gpio_sw);

    rotary->output.pressed = false;
    rotary->output.released = false;

    rotary->internal.state =
        rotary_read_state(rotary);

    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;

    rotary->internal.switch_state =
        gpio_get(rotary->config.gpio_sw);

    restore_interrupts(interrupt_state);
}


int32_t rotary_take_delta(
    rotary_t *const rotary
)
{
    if (rotary == NULL)
    {
        return 0;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    const int32_t delta = rotary->output.delta;
    rotary->output.delta = 0;

    restore_interrupts(interrupt_state);

    return delta;
}


int32_t rotary_get_position(
    const rotary_t *const rotary
)
{
    if (rotary == NULL)
    {
        return 0;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    const int32_t position =
        rotary->output.position;

    restore_interrupts(interrupt_state);

    return position;
}


bool rotary_take_pressed(
    rotary_t *const rotary
)
{
    if (rotary == NULL)
    {
        return false;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    const bool pressed = rotary->output.pressed;
    rotary->output.pressed = false;

    restore_interrupts(interrupt_state);

    return pressed;
}


bool rotary_take_released(
    rotary_t *const rotary
)
{
    if (rotary == NULL)
    {
        return false;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    const bool released = rotary->output.released;
    rotary->output.released = false;

    restore_interrupts(interrupt_state);

    return released;
}


bool rotary_is_pressed(
    const rotary_t *const rotary
)
{
    if (rotary == NULL)
    {
        return false;
    }

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    const bool pressed = rotary->output.button;

    restore_interrupts(interrupt_state);

    return pressed;
}


void rotary_set_hysteresis(
    rotary_t *const rotary,
    const uint8_t hysteresis
)
{
    if (rotary == NULL)
    {
        return;
    }

    const uint8_t maximum_hysteresis =
        (uint8_t)(
            INT8_MAX -
            rotary->config.steps_per_detent
        );

    const uint8_t safe_hysteresis =
        hysteresis <= maximum_hysteresis
            ? hysteresis
            : maximum_hysteresis;

    const uint32_t interrupt_state =
        save_and_disable_interrupts();

    rotary->config.hysteresis = safe_hysteresis;

    /*
     * Angefangene Bewegung verwerfen, damit die neue Schwelle
     * nicht auf einen alten Teilweg angewendet wird.
     */
    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;
    rotary->internal.state =
        rotary_read_state(rotary);

    restore_interrupts(interrupt_state);
}
