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
 * - optional time-based rotation acceleration
 * - optional position clamping or wrap-around
 * - absolute position and accumulated delta tracking
 * - debounced push-button press and release events
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
#include "pico/time.h"

// ========================
// === Static Constants ===
// ========================

/**
 * @brief Quadrature transition lookup table.
 *
 * Index: previous state << 2 | current state
 * Result: +1 clockwise, -1 counter-clockwise, 0 unchanged or invalid
 */
static const int8_t rotary_transition_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

// ========================
// === Global Variables ===
// ========================

/**
 * @brief Maps every GPIO to its owning encoder instance.
 */
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
 * @brief Check whether one limit mode value is valid.
 */
static bool rotary_limit_mode_is_valid(
    const rotary_limit_mode_t mode
)
{
    return
        mode == ROTARY_LIMIT_NONE ||
        mode == ROTARY_LIMIT_CLAMP ||
        mode == ROTARY_LIMIT_WRAP;
}


/**
 * @brief Validate acceleration settings.
 */
static bool rotary_acceleration_is_valid(
    const bool enabled,
    const uint32_t window_us,
    const uint8_t multiplier
)
{
    /* A multiplier of one is the valid non-accelerated step size. */
    if (multiplier == 0u)
    {
        return false;
    }

    /* Enabled acceleration requires a measurable window and a larger step. */
    if (
        enabled &&
        (
            window_us == 0u ||
            multiplier <= 1u
        )
    )
    {
        return false;
    }

    return true;
}


/**
 * @brief Validate position limit settings.
 */
static bool rotary_limits_are_valid(
    const rotary_limit_mode_t mode,
    const int32_t minimum,
    const int32_t maximum
)
{
    if (!rotary_limit_mode_is_valid(mode))
    {
        return false;
    }

    if (
        mode != ROTARY_LIMIT_NONE &&
        minimum > maximum
    )
    {
        return false;
    }

    return true;
}


/**
 * @brief Validate the complete encoder configuration.
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

    /* A zero threshold can never produce a detent event. */
    if (config->steps_per_detent == 0u)
    {
        return false;
    }

    /* Keep all decoder threshold arithmetic representable by int8_t. */
    if (
        (uint16_t)config->steps_per_detent +
        (uint16_t)config->hysteresis >
        INT8_MAX
    )
    {
        return false;
    }

    if (!rotary_acceleration_is_valid(
        config->acceleration_enabled,
        config->acceleration_window_us,
        config->acceleration_multiplier
    ))
    {
        return false;
    }

    return rotary_limits_are_valid(
        config->limit_mode,
        config->minimum,
        config->maximum
    );
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
 * @brief Normalize one position according to the active limit mode.
 */
static int32_t rotary_normalize_position(
    const rotary_t *const rotary,
    const int64_t requested_position
)
{
    switch (rotary->config.limit_mode)
    {
        case ROTARY_LIMIT_CLAMP:
            if (requested_position < rotary->config.minimum)
            {
                return rotary->config.minimum;
            }

            if (requested_position > rotary->config.maximum)
            {
                return rotary->config.maximum;
            }

            return (int32_t)requested_position;

        case ROTARY_LIMIT_WRAP:
        {
            const int64_t minimum = rotary->config.minimum;
            const int64_t maximum = rotary->config.maximum;
            const int64_t range = maximum - minimum + 1;

            int64_t offset = (requested_position - minimum) % range;

            /* C remainder keeps the sign of the dividend. */
            if (offset < 0)
            {
                offset += range;
            }

            return (int32_t)(minimum + offset);
        }

        case ROTARY_LIMIT_NONE:
        default:
            if (requested_position < INT32_MIN)
            {
                return INT32_MIN;
            }

            if (requested_position > INT32_MAX)
            {
                return INT32_MAX;
            }

            return (int32_t)requested_position;
    }
}


/**
 * @brief Determine the effective step size for one accepted detent.
 */
static int32_t rotary_get_step_size(
    rotary_t *const rotary,
    const int8_t direction,
    const uint32_t current_time_us
)
{
    int32_t step_size = 1;

    if (
        rotary->config.acceleration_enabled &&
        rotary->internal.last_step_direction == direction &&
        rotary->internal.last_step_us != 0u &&
        (uint32_t)(current_time_us - rotary->internal.last_step_us) <=
            rotary->config.acceleration_window_us
    )
    {
        step_size = rotary->config.acceleration_multiplier;
    }

    rotary->internal.last_step_us = current_time_us;
    rotary->internal.last_step_direction = direction;

    return step_size;
}


/**
 * @brief Apply one accepted detent to position and delta.
 */
static void rotary_apply_step(
    rotary_t *const rotary,
    const int8_t direction
)
{
    const uint32_t current_time_us = time_us_32();
    const int32_t step_size = rotary_get_step_size(
        rotary,
        direction,
        current_time_us
    );

    const int32_t requested_delta =
        direction > 0
            ? step_size
            : -step_size;

    const int32_t old_position = rotary->output.position;
    const int32_t new_position = rotary_normalize_position(
        rotary,
        (int64_t)old_position + requested_delta
    );

    rotary->output.position = new_position;

    switch (rotary->config.limit_mode)
    {
        case ROTARY_LIMIT_WRAP:
            /* Preserve user movement across the numerical wrap boundary. */
            rotary->output.delta += requested_delta;
            break;

        case ROTARY_LIMIT_CLAMP:
        case ROTARY_LIMIT_NONE:
        default:
            /* Do not report movement discarded by a hard boundary. */
            rotary->output.delta += new_position - old_position;
            break;
    }
}


/**
 * @brief Evaluate the current movement accumulator.
 */
static void rotary_process_accumulator(
    rotary_t *const rotary
)
{
    int16_t accumulator = rotary->internal.accumulator;

    /* Process positive movement. */
    int16_t threshold = rotary_positive_threshold(rotary);

    while (accumulator >= threshold)
    {
        rotary_apply_step(rotary, 1);

        accumulator -= threshold;
        rotary->internal.direction_bias = 1;
        threshold = rotary->config.steps_per_detent;
    }

    /* Process negative movement. */
    threshold = rotary_negative_threshold(rotary);

    while (accumulator <= -threshold)
    {
        rotary_apply_step(rotary, -1);

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

    const int8_t movement = rotary_transition_table[transition];

    /* Accumulate valid quadrature transitions only. */
    if (movement != 0)
    {
        rotary->internal.accumulator += movement;
    }

    /* Evaluate completed movement only at the electrical detent state. */
    if (current_state == 0x03u)
    {
        rotary_process_accumulator(rotary);
    }
}


/**
 * @brief Accept one stable push-button state and latch its edge event.
 */
static void rotary_accept_switch_state(
    rotary_t *const rotary,
    const bool raw_switch_state
)
{
    /* Ignore an unchanged accepted state. */
    if (raw_switch_state == rotary->internal.switch_state)
    {
        return;
    }

    rotary->internal.switch_state = raw_switch_state;

    /* Internal pull-up: LOW means pressed, HIGH means released. */
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
 * @brief Validate the push-button state after the debounce interval.
 */
static int64_t rotary_switch_debounce_alarm(
    const alarm_id_t alarm_id,
    void *const user_data
)
{
    rotary_t *const rotary = user_data;

    if (rotary == NULL)
    {
        return 0;
    }

    /* Ignore a stale callback replaced by a newer edge. */
    if (rotary->internal.debounce_alarm_id != (int32_t)alarm_id)
    {
        return 0;
    }

    rotary->internal.debounce_alarm_id = 0;

    rotary_accept_switch_state(
        rotary,
        gpio_get(rotary->config.gpio_sw)
    );

    return 0;
}


/**
 * @brief Process a push-button interrupt.
 */
static void rotary_process_switch(
    rotary_t *const rotary
)
{
    /* Immediate mode keeps debounce optional. */
    if (rotary->config.button_debounce_us == 0u)
    {
        rotary_accept_switch_state(
            rotary,
            gpio_get(rotary->config.gpio_sw)
        );
        return;
    }

    /* Restart the interval after every bouncing edge. */
    if (rotary->internal.debounce_alarm_id > 0)
    {
        cancel_alarm((alarm_id_t)rotary->internal.debounce_alarm_id);
        rotary->internal.debounce_alarm_id = 0;
    }

    const alarm_id_t alarm_id = add_alarm_in_us(
        rotary->config.button_debounce_us,
        rotary_switch_debounce_alarm,
        rotary,
        true
    );

    /* A failed alarm allocation leaves the previous accepted state intact. */
    if (alarm_id > 0)
    {
        rotary->internal.debounce_alarm_id = (int32_t)alarm_id;
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
// === Configuration ===
// =====================

bool rotary_config(
    rotary_t *const rotary,
    const uint8_t gpio_clk,
    const uint8_t gpio_dt,
    const uint8_t gpio_sw,
    const uint8_t steps_per_detent,
    const uint8_t hysteresis,
    const uint32_t button_debounce_us,
    const bool acceleration_enabled,
    const uint32_t acceleration_window_us,
    const uint8_t acceleration_multiplier,
    const rotary_limit_mode_t limit_mode,
    const int32_t minimum,
    const int32_t maximum
)
{
    if (rotary == NULL)
    {
        return false;
    }

    const rotary_config_t config =
    {
        .gpio_clk = gpio_clk,
        .gpio_dt = gpio_dt,
        .gpio_sw = gpio_sw,
        .steps_per_detent = steps_per_detent,
        .hysteresis = hysteresis,
        .button_debounce_us = button_debounce_us,
        .acceleration_enabled = acceleration_enabled,
        .acceleration_window_us = acceleration_window_us,
        .acceleration_multiplier = acceleration_multiplier,
        .limit_mode = limit_mode,
        .minimum = minimum,
        .maximum = maximum
    };

    if (!rotary_config_is_valid(&config))
    {
        return false;
    }

    rotary->config = config;

    return true;
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

    const uint32_t interrupt_state = save_and_disable_interrupts();

    if (!rotary_gpio_is_available(rotary))
    {
        restore_interrupts(interrupt_state);
        return false;
    }

    /* Configure GPIOs first. */
    rotary_gpio_init(rotary->config.gpio_clk);
    rotary_gpio_init(rotary->config.gpio_dt);
    rotary_gpio_init(rotary->config.gpio_sw);

    /* Initialize application-visible state. */
    rotary->output.position = rotary_normalize_position(rotary, 0);
    rotary->output.delta = 0;
    rotary->output.button = !gpio_get(rotary->config.gpio_sw);
    rotary->output.pressed = false;
    rotary->output.released = false;

    /* Initialize decoder and acceleration state. */
    rotary->internal.state = rotary_read_state(rotary);
    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;
    rotary->internal.last_step_us = 0u;
    rotary->internal.last_step_direction = 0;

    /* Initialize push-button state. */
    rotary->internal.switch_state = gpio_get(rotary->config.gpio_sw);
    rotary->internal.debounce_alarm_id = 0;

    /* Assign GPIO ownership to this instance. */
    rotary_gpio_owner[rotary->config.gpio_clk] = rotary;
    rotary_gpio_owner[rotary->config.gpio_dt]  = rotary;
    rotary_gpio_owner[rotary->config.gpio_sw]  = rotary;

    restore_interrupts(interrupt_state);

    /* Register the global callback once. Additional GPIOs share it. */
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

    const uint32_t interrupt_state = save_and_disable_interrupts();

    if (rotary->internal.debounce_alarm_id > 0)
    {
        cancel_alarm((alarm_id_t)rotary->internal.debounce_alarm_id);
        rotary->internal.debounce_alarm_id = 0;
    }

    rotary->output.position = rotary_normalize_position(rotary, 0);
    rotary->output.delta = 0;
    rotary->output.button = !gpio_get(rotary->config.gpio_sw);
    rotary->output.pressed = false;
    rotary->output.released = false;

    rotary->internal.state = rotary_read_state(rotary);
    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;
    rotary->internal.last_step_us = 0u;
    rotary->internal.last_step_direction = 0;

    rotary->internal.switch_state = gpio_get(rotary->config.gpio_sw);
    rotary->internal.debounce_alarm_id = 0;

    restore_interrupts(interrupt_state);
}


void rotary_set_position(
    rotary_t *const rotary,
    const int32_t position
)
{
    if (rotary == NULL)
    {
        return;
    }

    const uint32_t interrupt_state = save_and_disable_interrupts();

    rotary->output.position = rotary_normalize_position(rotary, position);
    rotary->output.delta = 0;

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

    const uint32_t interrupt_state = save_and_disable_interrupts();

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

    const uint32_t interrupt_state = save_and_disable_interrupts();
    const int32_t position = rotary->output.position;
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

    const uint32_t interrupt_state = save_and_disable_interrupts();

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

    const uint32_t interrupt_state = save_and_disable_interrupts();

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

    const uint32_t interrupt_state = save_and_disable_interrupts();
    const bool pressed = rotary->output.button;
    restore_interrupts(interrupt_state);

    return pressed;
}

// =========================
// === Runtime Settings  ===
// =========================

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
        (uint8_t)(INT8_MAX - rotary->config.steps_per_detent);

    const uint8_t safe_hysteresis =
        hysteresis <= maximum_hysteresis
            ? hysteresis
            : maximum_hysteresis;

    const uint32_t interrupt_state = save_and_disable_interrupts();

    rotary->config.hysteresis = safe_hysteresis;

    /* Discard movement collected under the previous threshold. */
    rotary->internal.accumulator = 0;
    rotary->internal.direction_bias = 0;
    rotary->internal.state = rotary_read_state(rotary);

    restore_interrupts(interrupt_state);
}


void rotary_set_button_debounce(
    rotary_t *const rotary,
    const uint32_t debounce_us
)
{
    if (rotary == NULL)
    {
        return;
    }

    const uint32_t interrupt_state = save_and_disable_interrupts();

    /* Cancel validation still using the previous interval. */
    if (rotary->internal.debounce_alarm_id > 0)
    {
        cancel_alarm((alarm_id_t)rotary->internal.debounce_alarm_id);
        rotary->internal.debounce_alarm_id = 0;
    }

    rotary->config.button_debounce_us = debounce_us;

    /* Adopt the current level without synthesizing an event. */
    rotary->internal.switch_state = gpio_get(rotary->config.gpio_sw);
    rotary->output.button = !rotary->internal.switch_state;
    rotary->output.pressed = false;
    rotary->output.released = false;

    restore_interrupts(interrupt_state);
}


bool rotary_set_acceleration(
    rotary_t *const rotary,
    const bool enabled,
    const uint32_t window_us,
    const uint8_t multiplier
)
{
    if (
        rotary == NULL ||
        !rotary_acceleration_is_valid(enabled, window_us, multiplier)
    )
    {
        return false;
    }

    const uint32_t interrupt_state = save_and_disable_interrupts();

    rotary->config.acceleration_enabled = enabled;
    rotary->config.acceleration_window_us = window_us;
    rotary->config.acceleration_multiplier = multiplier;

    /* Start the new timing configuration from a clean baseline. */
    rotary->internal.last_step_us = 0u;
    rotary->internal.last_step_direction = 0;

    restore_interrupts(interrupt_state);

    return true;
}


bool rotary_set_limits(
    rotary_t *const rotary,
    const rotary_limit_mode_t mode,
    const int32_t minimum,
    const int32_t maximum
)
{
    if (
        rotary == NULL ||
        !rotary_limits_are_valid(mode, minimum, maximum)
    )
    {
        return false;
    }

    const uint32_t interrupt_state = save_and_disable_interrupts();

    rotary->config.limit_mode = mode;
    rotary->config.minimum = minimum;
    rotary->config.maximum = maximum;

    /* Normalize the current position immediately and clear stale movement. */
    rotary->output.position = rotary_normalize_position(
        rotary,
        rotary->output.position
    );
    rotary->output.delta = 0;

    restore_interrupts(interrupt_state);

    return true;
}
