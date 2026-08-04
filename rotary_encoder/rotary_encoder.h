/**
 * @file rotary_encoder.h
 * @author Robin Gerhartz (Gnibor)
 * @brief Public API for an interrupt-driven rotary encoder driver.
 *
 * @details
 * This driver provides:
 * - configurable GPIO assignments for CLK, DT, and SW
 * - configurable quadrature transitions per mechanical detent
 * - configurable direction-change hysteresis
 * - optional rotation acceleration
 * - optional position clamping or wrap-around
 * - absolute position and accumulated delta tracking
 * - debounced push-button state, press, and release events
 * - support for multiple encoder instances
 *
 * The driver uses internal pull-ups and assumes active-low encoder contacts
 * and an active-low push button.
 *
 * @project Rotary Encoder Driver for Raspberry Pi Pico
 * @license MIT License (see LICENSE file in root)
 * @copyright Copyright (c) 2026 (Gnibor) Robin Gerhartz
 * @see https://github.com/Gnibor/RP-Pico-libs
 */
#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

#include <stdbool.h>
#include <stdint.h>

// =====================
// === Enumerations  ===
// =====================

/**
 * @enum rotary_limit_mode_t
 * @brief Position limit behavior.
 */
typedef enum
{
    ROTARY_LIMIT_NONE = 0, /**< Position is only restricted by int32_t range. */
    ROTARY_LIMIT_CLAMP,    /**< Position stops at the configured minimum or maximum. */
    ROTARY_LIMIT_WRAP      /**< Position wraps from one configured limit to the other. */
} rotary_limit_mode_t;

// ========================
// === Structs / Unions ===
// ========================

/**
 * @struct rotary_config_t
 * @brief Hardware assignment and decoder configuration.
 */
typedef struct
{
    uint8_t gpio_clk;             /**< GPIO connected to the CLK channel. */
    uint8_t gpio_dt;              /**< GPIO connected to the DT channel. */
    uint8_t gpio_sw;              /**< GPIO connected to the push button. */

    uint8_t steps_per_detent;     /**< Valid quadrature transitions per detent. */
    uint8_t hysteresis;           /**< Additional reverse transitions required before changing direction. */
    uint32_t button_debounce_us;  /**< Push-button debounce interval in microseconds; 0 disables debounce. */

    bool acceleration_enabled;        /**< Enable time-based rotation acceleration. */
    uint32_t acceleration_window_us;  /**< Maximum interval between equal-direction steps for acceleration. */
    uint8_t acceleration_multiplier;  /**< Position increment used while acceleration is active. */

    rotary_limit_mode_t limit_mode; /**< Position limit behavior. */
    int32_t minimum;                /**< Minimum position for clamp or wrap mode. */
    int32_t maximum;                /**< Maximum position for clamp or wrap mode. */
} rotary_config_t;

/**
 * @struct rotary_output_t
 * @brief Application-visible encoder output values.
 *
 * @note The event flags are latched until consumed through the matching
 *       rotary_take_*() function.
 */
typedef struct
{
    int32_t position;  /**< Absolute position after acceleration and limit handling. */
    int32_t delta;     /**< Accumulated effective movement since the last delta read. */

    bool button;       /**< Current logical button state; true means pressed. */
    bool pressed;      /**< Latched button-pressed event. */
    bool released;     /**< Latched button-released event. */
} rotary_output_t;

/**
 * @struct rotary_internal_t
 * @brief Internal decoder, acceleration, and switch state.
 *
 * @warning Do not modify these fields directly.
 */
typedef struct
{
    uint8_t state;          /**< Last sampled CLK/DT state. */
    int8_t accumulator;     /**< Accumulated valid quadrature transitions. */
    int8_t direction_bias;  /**< Last accepted decoder direction: -1, 0, or +1. */

    uint32_t last_step_us;  /**< Timestamp of the previous accepted detent. */
    int8_t last_step_direction; /**< Direction of the previous accepted detent. */

    bool switch_state;         /**< Last accepted raw push-button state. */
    int32_t debounce_alarm_id; /**< Active debounce alarm ID; 0 means no alarm is pending. */
} rotary_internal_t;

/**
 * @struct rotary_t
 * @brief Complete rotary encoder driver instance.
 */
typedef struct
{
    rotary_config_t config;     /**< User-provided hardware and behavior configuration. */
    rotary_output_t output;     /**< Application-visible values and events. */
    rotary_internal_t internal; /**< Private runtime state. */
} rotary_t;

// ============================
// === Function declaration ===
// ============================

/**
 * @brief Configure one rotary encoder instance.
 *
 * @param[out] rotary Pointer to the encoder instance to configure.
 * @param gpio_clk GPIO connected to the CLK channel.
 * @param gpio_dt GPIO connected to the DT channel.
 * @param gpio_sw GPIO connected to the push button.
 * @param steps_per_detent Valid quadrature transitions per mechanical detent.
 * @param hysteresis Additional reverse transitions required before changing direction.
 * @param button_debounce_us Push-button debounce interval in microseconds; 0 disables debounce.
 * @param acceleration_enabled Enable time-based rotation acceleration.
 * @param acceleration_window_us Maximum interval between equal-direction steps for acceleration.
 * @param acceleration_multiplier Position increment used while acceleration is active.
 * @param limit_mode Position limit behavior.
 * @param minimum Minimum position for clamp or wrap mode.
 * @param maximum Maximum position for clamp or wrap mode.
 *
 * @return true if the supplied configuration is valid and was stored.
 * @return false if @p rotary is NULL or the configuration is invalid.
 *
 * @note This function only stores configuration. Call rotary_init() afterwards.
 */
bool rotary_config(
    rotary_t *rotary,
    uint8_t gpio_clk,
    uint8_t gpio_dt,
    uint8_t gpio_sw,
    uint8_t steps_per_detent,
    uint8_t hysteresis,
    uint32_t button_debounce_us,
    bool acceleration_enabled,
    uint32_t acceleration_window_us,
    uint8_t acceleration_multiplier,
    rotary_limit_mode_t limit_mode,
    int32_t minimum,
    int32_t maximum
);

/**
 * @brief Initialize one rotary encoder instance and its GPIO interrupts.
 *
 * @param[in,out] rotary Pointer to a configured encoder instance.
 *
 * @return true if initialization succeeded.
 * @return false if the configuration is invalid or a GPIO is already owned by
 *         another encoder instance.
 *
 * @warning The Pico SDK exposes one global GPIO callback per core. Another
 *          component must not replace that callback after initialization.
 */
bool rotary_init(rotary_t *rotary);

/**
 * @brief Reset position, delta, events, and decoder state.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 *
 * @details GPIO configuration and interrupt ownership remain unchanged.
 */
void rotary_reset(rotary_t *rotary);

/**
 * @brief Set the current absolute encoder position.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 * @param position Requested position. Active clamp or wrap rules are applied.
 */
void rotary_set_position(rotary_t *rotary, int32_t position);

/**
 * @brief Read and clear accumulated effective movement.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 *
 * @return Positive values for clockwise movement.
 * @return Negative values for counter-clockwise movement.
 * @return 0 if no movement is pending or @p rotary is NULL.
 */
int32_t rotary_take_delta(rotary_t *rotary);

/**
 * @brief Read the current absolute encoder position.
 *
 * @param[in] rotary Pointer to the encoder instance.
 *
 * @return Current position after acceleration and limit handling.
 * @return 0 if @p rotary is NULL.
 */
int32_t rotary_get_position(const rotary_t *rotary);

/**
 * @brief Read and clear the latched button-pressed event.
 */
bool rotary_take_pressed(rotary_t *rotary);

/**
 * @brief Read and clear the latched button-released event.
 */
bool rotary_take_released(rotary_t *rotary);

/**
 * @brief Read the current logical push-button state.
 */
bool rotary_is_pressed(const rotary_t *rotary);

/**
 * @brief Change the direction-change hysteresis at runtime.
 *
 * @details Any partial movement stored in the decoder is discarded.
 */
void rotary_set_hysteresis(rotary_t *rotary, uint8_t hysteresis);

/**
 * @brief Change the push-button debounce interval at runtime.
 *
 * @details Any pending debounce alarm is cancelled. The current physical
 *          switch level becomes the new accepted baseline without generating
 *          a press or release event.
 */
void rotary_set_button_debounce(rotary_t *rotary, uint32_t debounce_us);

/**
 * @brief Change rotation acceleration at runtime.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 * @param enabled Enable or disable acceleration.
 * @param window_us Maximum interval between equal-direction steps.
 * @param multiplier Effective step size while acceleration is active.
 *
 * @return true if the acceleration configuration is valid and was stored.
 * @return false otherwise.
 */
bool rotary_set_acceleration(
    rotary_t *rotary,
    bool enabled,
    uint32_t window_us,
    uint8_t multiplier
);

/**
 * @brief Change position limit behavior at runtime.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 * @param mode New limit behavior.
 * @param minimum New minimum position.
 * @param maximum New maximum position.
 *
 * @return true if the limit configuration is valid and was stored.
 * @return false otherwise.
 *
 * @details The current position is immediately normalized to the new limits.
 */
bool rotary_set_limits(
    rotary_t *rotary,
    rotary_limit_mode_t mode,
    int32_t minimum,
    int32_t maximum
);

#endif // _ROTARY_ENCODER_H_
