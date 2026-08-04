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
 * - absolute position and accumulated delta tracking
 * - push-button state, press, and release events
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

// ========================
// === Structs / Unions ===
// ========================

/**
 * @struct rotary_config_t
 * @brief Hardware assignment and decoder configuration.
 */
typedef struct
{
    uint8_t gpio_clk;          /**< GPIO connected to the CLK channel. */
    uint8_t gpio_dt;           /**< GPIO connected to the DT channel. */
    uint8_t gpio_sw;           /**< GPIO connected to the push button. */

    uint8_t steps_per_detent;  /**< Valid quadrature transitions per detent. */
    uint8_t hysteresis;        /**< Additional reverse transitions required before changing direction. */
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
    int32_t position;  /**< Absolute position in accepted detent steps. */
    int32_t delta;     /**< Accumulated movement since the last delta read. */

    bool button;       /**< Current logical button state; true means pressed. */
    bool pressed;      /**< Latched button-pressed event. */
    bool released;     /**< Latched button-released event. */
} rotary_output_t;

/**
 * @struct rotary_internal_t
 * @brief Internal decoder and switch state.
 *
 * @warning Do not modify these fields directly.
 */
typedef struct
{
    uint8_t state;          /**< Last sampled CLK/DT state. */
    int8_t accumulator;     /**< Accumulated valid quadrature transitions. */
    int8_t direction_bias;  /**< Last accepted direction: -1, 0, or +1. */

    bool switch_state;      /**< Last sampled raw push-button state. */
} rotary_internal_t;

/**
 * @struct rotary_t
 * @brief Complete rotary encoder driver instance.
 */
typedef struct
{
    rotary_config_t config;    /**< User-provided hardware and decoder configuration. */
    rotary_output_t output;    /**< Application-visible values and events. */
    rotary_internal_t internal;/**< Private runtime state. */
} rotary_t;

// ============================
// === Function declaration ===
// ============================

/**
 * @brief Initialize one rotary encoder instance and its GPIO interrupts.
 *
 * @param[in,out] rotary Pointer to an encoder instance with a populated
 *                       @ref rotary_config_t configuration.
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
 * @brief Read and clear accumulated movement.
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
 * @return Current position in accepted detent steps.
 * @return 0 if @p rotary is NULL.
 */
int32_t rotary_get_position(const rotary_t *rotary);

/**
 * @brief Read and clear the latched button-pressed event.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 *
 * @return true if a press event occurred since the previous call.
 * @return false otherwise or if @p rotary is NULL.
 */
bool rotary_take_pressed(rotary_t *rotary);

/**
 * @brief Read and clear the latched button-released event.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 *
 * @return true if a release event occurred since the previous call.
 * @return false otherwise or if @p rotary is NULL.
 */
bool rotary_take_released(rotary_t *rotary);

/**
 * @brief Read the current logical push-button state.
 *
 * @param[in] rotary Pointer to the encoder instance.
 *
 * @return true while the button is pressed.
 * @return false while released or if @p rotary is NULL.
 */
bool rotary_is_pressed(const rotary_t *rotary);

/**
 * @brief Change the direction-change hysteresis at runtime.
 *
 * @param[in,out] rotary Pointer to the encoder instance.
 * @param hysteresis Additional reverse quadrature transitions required before
 *                   accepting a direction change. A value of 0 disables the
 *                   additional direction hysteresis.
 *
 * @details Any partial movement stored in the decoder is discarded when this
 * function is called.
 */
void rotary_set_hysteresis(rotary_t *rotary, uint8_t hysteresis);

#endif // _ROTARY_ENCODER_H_
