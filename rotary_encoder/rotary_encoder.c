/**
 * @file rotary_encoder.c
 * @author Robin Gerhartz (Gnibor)
 * @brief Interrupt-driven rotary encoder driver implementation.
 *
 * @details
 * This driver provides:
 * - quadrature decoding through GPIO interrupts
 * - configurable transitions per detent
 * - configurable direction-change hysteresis
 * - absolute position and accumulated delta tracking
 * - push-button press and release events
 * - support for multiple encoder instances
 *
 * The implementation uses static storage only and performs no dynamic
 * allocation.
 *
 * @project Rotary Encoder Driver for Raspberry Pi Pico
 * @license MIT License (see LICENSE file in root)
 * @copyright Copyright (c) 2026 (Gnibor) Robin Gerhartz
 * @see https://github.com/Gnibor/RP-Pico-libs
 */
#include "rotary_encoder.h"

#include <limits.h>
#include <stddef.h>

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"


/*
 * Quadrature transition table:
 *
 * Index:
 *     previous state << 2 | current state
 *
 * Result:
 *     +1 = valid clockwise transition
 *     -1 = valid counter-clockwise transition
 *      0 = unchanged or invalid transition
 */
// ========================
// === Static Constants ===
// ========================
static const int8_t rotary_transition_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

/*
 * Maps each GPIO to its owning rotary encoder instance.
 *
 * The Pico SDK exposes one global GPIO callback. This ownership table allows
 * multiple encoder instances to share that callback safely.
 */
// ========================
// === Global Variables ===
// ========================
static rotary_t *rotary_gpio_owner[NUM_BANK0_GPIOS] = { NULL };


// ===========================
// === Function prototypes ===
// ===========================

/**
 * @brief Read both encoder channels as one 2-bit state.
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
 * @brief Return the active positive movement threshold.
 */
static inline int16_t rotary_positive_threshold(
    const rotary_t *const rotary
)
{
    int16_t threshold = rotary->config.steps_per_detent;

    /* Require additional reverse movement before accepting a direction change. */
    if (rotary->internal.direction_bias < 0)
    {
        threshold += rotary->config.hysteresis;
    }

    return threshold;
}


/**
 * @brief Return the active negative movement threshold.
 */
static inline int16_t rotary_negative_threshold(
    const rotary_t *const rotary
)
{
    int16_t threshold = rotary->config.steps_per_detent;

    /* Require additional reverse movement before accepting a direction change. */
    if (rotary->internal.direction_bias > 0)
    {
        threshold += rotary->config.hysteresis;
    }

    return threshold;
}


/**
 * @brief Evaluate the current movement accumulator.
 */
static void rotary_process_accumulator(
    rotary_t *const rotary
)
{
    /*
     * Use int16_t for threshold arithmetic while the stored accumulator remains int8_t.
     */
    int16_t accumulator = rotary->internal.accumulator;

    /* Process positive movement. */
    int16_t threshold = rotary_positive_threshold(rotary);

    while (accumulator >= threshold)
    {
        rotary->output.position++;
        rotary->output.delta++;

        accumulator -= threshold;
        rotary->internal.direction_bias = 1;

        /*
         * After an accepted direction change, subsequent steps use the normal threshold.
         */
        threshold = rotary->config.steps_per_detent;
    }

    /* Process negative movement. */
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
 * @brief Process an encoder channel interrupt.
 */
static void rotary_process_rotation(
    rotary_t *const rotary
)
{
    const uint8_t current_state = rotary_read_state(rotary);

    /* Ignore interrupts without an actual state change. */
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

    /* Accumulate valid quadrature transitions only. */
    if (movement != 0)
    {
        rotary->internal.accumulator += movement;
    }

    /*
     * Evaluate steps only at the electrical detent state.
     * Keep the accumulator because it may contain movement that has not yet
     * crossed the configured direction hysteresis.
     */
    if (current_state == 0x03u)
    {
        rotary_process_accumulator(rotary);
    }
}

/**
 * @brief Process a push-button interrupt.
 */
static void rotary_process_switch(
    rotary_t *const rotary
)
{
    const bool raw_switch_state =
        gpio_get(rotary->config.gpio_sw);

    /* Ignore an unchanged switch state. */
    if (raw_switch_state == rotary->internal.switch_state)
    {
        return;
    }

    rotary->internal.switch_state = raw_switch_state;

    /*
     * Pull-up logic:
     *     LOW  = pressed
     *     HIGH = released
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
 * @brief Shared Pico SDK GPIO callback.
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
 * @brief Validate the encoder configuration.
 */
static bool rotary_config_is_valid(
    const rotary_config_t *const config
)
{
    if (config == NULL)
    {
        return false;
    }

    /* All GPIO numbers must be valid. */
    if (
        config->gpio_clk >= NUM_BANK0_GPIOS ||
        config->gpio_dt  >= NUM_BANK0_GPIOS ||
        config->gpio_sw  >= NUM_BANK0_GPIOS
    )
    {
        return false;
    }

    /* Each signal requires a dedicated GPIO. */
    if (
        config->gpio_clk == config->gpio_dt ||
        config->gpio_clk == config->gpio_sw ||
        config->gpio_dt  == config->gpio_sw
    )
    {
        return false;
    }

    /* A zero step threshold can never produce an event. */
    if (config->steps_per_detent == 0u)
    {
        return false;
    }

    /*
     * The accumulator is int8_t. The base threshold plus hysteresis must remain representable.
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
 * @brief Check whether the requested GPIOs are available.
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
 * @brief Initialize one GPIO input with its internal pull-up enabled.
 */
static void rotary_gpio_init(
    const uint8_t gpio
)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}


// =====================
// === Instance Init ===
// =====================
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

    /* Configure GPIOs first. */
    rotary_gpio_init(rotary->config.gpio_clk);
    rotary_gpio_init(rotary->config.gpio_dt);
    rotary_gpio_init(rotary->config.gpio_sw);

    /* Initialize runtime state. */
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

    /* Assign GPIO ownership to this instance. */
    rotary_gpio_owner[rotary->config.gpio_clk] = rotary;
    rotary_gpio_owner[rotary->config.gpio_dt]  = rotary;
    rotary_gpio_owner[rotary->config.gpio_sw]  = rotary;

    restore_interrupts(interrupt_state);

    /*
     * Register the global callback once. Additional GPIOs use the same callback.
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


// ===================
// === Driver State ===
// ===================
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
     * Discard partial movement so the new threshold is not applied to an old transition sequence.
     */
    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;
    rotary->internal.state =
        rotary_read_state(rotary);

    restore_interrupts(interrupt_state);
}
