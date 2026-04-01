/**
 * @file mpu.h
 * @author Robin Gerhartz (Gnibor)
 * @brief Public low-level API for the MPU60x0 driver.
 *
 * @details
 * This header exposes a register-near driver interface for MPU60x0-based IMU
 * devices on the Raspberry Pi Pico.
 *
 * The driver provides:
 * - I2C-based device access
 * - register-oriented configuration helpers
 * - sensor readout for accelerometer, gyroscope, and temperature data
 * - optional raw-to-physical-unit conversion
 * - calibration support
 * - power-management and cycle-mode control
 * - optional interrupt pin configuration and status handling
 *
 * The API is intentionally designed in a low-level style and stays close to
 * the underlying register model wherever practical.
 *
 * @project MPU Driver for Raspberry Pi Pico
 * @license MIT License (see LICENSE file in root)
 * @copyright Copyright (c) 2026 (Gnibor) Robin Gerhartz
 * @see https://github.com/Gnibor/RP-Pico-libs
 */
#ifndef _MPU_H_
#define _MPU_H_

#include <stdint.h>
#include "hardware/structs/i2c.h"
#include "mpu_reg_map.h"
#include "mpu_types.h"
#include "rp_pico.h"
#include "log.h"

// =============================
// === Configurable Hardware ===
// =============================

/**
 * @brief Default I2C hardware block used by the driver.
 *
 * @details
 * This macro may be overridden before including this header if a different
 * I2C hardware instance should be used.
 */
#ifndef MPU_I2C_HW
#define MPU_I2C_HW i2c1_hw
#endif

/**
 * @brief Default SDA GPIO pin for the MPU I2C connection.
 *
 * @details
 * This macro may be overridden before including this header.
 */
#ifndef MPU_SDA_PIN
#define MPU_SDA_PIN 6
#endif

/**
 * @brief Default SCL GPIO pin for the MPU I2C connection.
 *
 * @details
 * This macro may be overridden before including this header.
 */
#ifndef MPU_SCL_PIN
#define MPU_SCL_PIN 7
#endif

/**
 * @brief Enable or disable RP2040 internal pull-ups on the I2C pins.
 *
 * @details
 * - 1 enables the internal pull-up resistors
 * - 0 leaves the pull-ups disabled
 *
 * @note
 * External pull-up resistors are usually preferred for reliable I2C operation.
 */
#ifndef MPU_USE_PULLUP
#define MPU_USE_PULLUP 0
#endif

/**
 * @brief Optional GPIO pin connected to the MPU interrupt output.
 *
 * @details
 * If this macro is set to 0, interrupt-related API declarations are excluded
 * from the code at compile time.
 */
#ifndef MPU_INT_PIN
#define MPU_INT_PIN 26
#endif

/**
 * @brief Enable or disable the RP2040 internal pull-up on the interrupt pin.
 *
 * @details
 * - 1 enables the internal pull-up resistor
 * - 0 leaves the pull-up disabled
 */
#ifndef MPU_INT_PULLUP
#define MPU_INT_PULLUP 0
#endif

// ============================
// === Function declaration ===
// ============================

/**
 * @brief Initialize an MPU device structure.
 *
 * @param i2c_hw Pointer to the I2C hardware block used for communication.
 * @param addr 7-bit I2C device address, see @ref mpu_addr_t in @c mpu_types.h.
 * @return Initialized device structure.
 *
 * @details
 * This function prepares and returns an @ref mpu_value_t instance for later driver
 * use. The exact initialization scope depends on the implementation.
 */
mpu_value_t *mpu_init(i2c_hw_t *i2c_hw, mpu_addr_t addr);

/**
 * @brief Read and validate the WHO_AM_I register.
 *
 * @return true if the detected device identity matches an expected MPU value,
 *         false otherwise.
 *
 * @details
 * This function performs a low-level device identity check based on the
 * WHO_AM_I register.
 */
bool mpu_who_am_i(void);

/**
 * @brief Enable or disable I2C bypass mode.
 *
 * @param active true to enable bypass mode, false to disable it.
 * @return true on success, false on failure.
 *
 * @details
 * Bypass mode allows direct primary-bus access to auxiliary devices connected
 * through the MPU, depending on device variant and wiring.
 */
bool mpu_bypass(bool active);

/**
 * @brief Execute one or more reset operations.
 *
 * @param reset Reset control flags, see @ref mpu_reset_t in @c mpu_types.h.
 * @return true on success, false on failure.
 *
 * @details
 * Depending on the provided flags, this may reset signal paths, internal
 * submodules, FIFO state, or the complete device.
 */
bool mpu_reset(mpu_reset_t reset);

/**
 * @brief Configure sleep-related control bits.
 *
 * @param sleep Sleep and temperature control flags, see @ref mpu_sleep_t in
 *              @c mpu_types.h.
 * @return true on success, false on failure.
 *
 * @details
 * This function applies register-near sleep and temperature-related power
 * configuration.
 */
bool mpu_sleep(mpu_sleep_t sleep);

/**
 * @brief Configure standby control bits.
 *
 * @param stby Standby bitmask, see @ref mpu_stby_t in @c mpu_reg_map.h.
 * @return true on success, false on failure.
 *
 * @details
 * This function controls axis-specific standby behavior through the underlying
 * power-management register configuration.
 */
bool mpu_stby(mpu_stby_t stby);

/**
 * @brief Select the clock source.
 *
 * @param clksel Clock selection value, see @ref mpu_clk_sel_t in
 *               @c mpu_reg_map.h.
 * @return true on success, false on failure.
 *
 * @details
 * This function updates the device clock-source configuration in a
 * register-oriented way.
 */
bool mpu_clk_sel(mpu_clk_sel_t clksel);

/**
 * @brief Configure the digital low-pass filter.
 *
 * @param cfg Digital low-pass filter configuration, see @ref mpu_dlpf_cfg_t
 *            in @c mpu_reg_map.h.
 * @return true on success, false on failure.
 */
bool mpu_dlpf_cfg(mpu_dlpf_cfg_t cfg);

/**
 * @brief Configure the sample rate divider.
 *
 * @param smplrt_div Sample-rate divider value, see @ref mpu_smplrt_div_t in
 *                   @c mpu_reg_map.h.
 * @return true on success, false on failure.
 *
 * @details
 * This function writes the sample-rate divider configuration used by the MPU.
 */
bool mpu_smplrt_div(mpu_smplrt_div_t smplrt_div);

/**
 * @brief Configure the accelerometer high-pass filter.
 *
 * @param ahpf Accelerometer high-pass filter configuration, see
 *             @ref mpu_ahpf_t in @c mpu_reg_map.h.
 * @return true on success, false on failure.
 */
bool mpu_ahpf(mpu_ahpf_t ahpf);

/**
 * @brief Configure gyroscope and accelerometer full-scale ranges.
 *
 * @param fsr Gyroscope full-scale range, see @ref mpu_fsr_t in
 *            @c mpu_reg_map.h.
 * @param afsr Accelerometer full-scale range, see @ref mpu_afsr_t in
 *             @c mpu_reg_map.h.
 * @return true on success, false on failure.
 *
 * @details
 * This function updates the full-scale range settings for both motion sensors.
 */
bool mpu_fsr(mpu_fsr_t fsr, mpu_afsr_t afsr);

/**
 * @brief Calibrate one or more sensor groups.
 *
 * @param sensor Sensor selection and modifier flags, see @ref mpu_sensor_t in
 *               @c mpu_types.h.
 * @param sample Number of samples used during calibration.
 * @return true on success, false on failure.
 *
 * @details
 * This function performs calibration based on repeated sampling.
 *
 * For accelerometer calibration, an axis reference flag such as
 * @ref MPU_ACCEL_X, @ref MPU_ACCEL_Y, or @ref MPU_ACCEL_Z should be provided
 * when required by the implementation.
 */
bool mpu_calibrate(mpu_sensor_t sensor, uint8_t samples);

/**
 * @brief Read one or more sensor groups from the device.
 *
 * @param sensor Sensor selection and modifier flags, see @ref mpu_sensor_t in
 *               @c mpu_types.h.
 * @return true on success, false on failure.
 *
 * @details
 * Depending on the selected flags, this function reads accelerometer,
 * gyroscope, and/or temperature data and may optionally update scaled values.
 */
bool mpu_read(mpu_sensor_t sensor);

/**
 * @brief Configure cycle-mode behavior.
 *
 * @param mode Cycle-mode control value, see @ref mpu_cycle_t in
 *             @c mpu_types.h.
 * @param wake_up_rate Low-power wake-up rate selection, see
 *                     @ref mpu_lp_wake_t in @c mpu_reg_map.h.
 * @return true on success, false on failure.
 *
 * @details
 * This function applies cycle-mode and low-power wake-up configuration.
 *
 * @warning
 * Marked as work in progress in the current API design.
 */
bool mpu_cycle_mode(mpu_cycle_t mode, mpu_lp_wake_t wake_up_rate);

#if MPU_INT_PIN

/**
 * @brief Configure interrupt pin behavior.
 *
 * @param cfg Interrupt pin configuration flags, see @ref mpu_int_pin_cfg_t in
 *            @c mpu_reg_map.h.
 * @return true on success, false on failure.
 */
bool mpu_int_pin_cfg(mpu_int_pin_cfg_t cfg);

/**
 * @brief Enable interrupt sources.
 *
 * @param enable Interrupt enable flags, see @ref mpu_int_enable_t in
 *               @c mpu_reg_map.h.
 * @return true on success, false on failure.
 */
bool mpu_int_enable(mpu_int_enable_t enable);

/**
 * @brief Configure motion interrupt threshold and duration.
 *
 * @param ms Motion duration in milliseconds.
 * @param mg Motion threshold in milli-g (mg).
 * @return true on success, false on failure.
 *
 * @details
 * This function configures the motion interrupt using user-facing metric input
 * values and converts them to the required register representation internally.
 *
 * Conversion behavior:
 * - @p ms is interpreted as milliseconds and limited to the range 1..255
 * - @p mg is interpreted as milli-g and converted to the motion threshold
 *   register value using:
 *
 *     threshold_lsb = mg / 32
 *
 *   according to the MPU datasheet resolution of 1 LSB = 32 mg
 *
 * - the resulting threshold register value is limited to the range 1..255
 *
 * @note
 * The threshold conversion uses integer division. Values that are not exact
 * multiples of 32 mg are truncated toward zero.
 *
 * @warning
 * Very small threshold values may collapse to the minimum effective register
 * value after conversion and clamping.
 */
bool mpu_int_motion_cfg(uint8_t ms, uint16_t mg);

/**
 * @brief Read interrupt status information.
 *
 * @return true if the relevant interrupt condition is active, false otherwise.
 *
 * @details
 * This function evaluates the interrupt status register according to the
 * implementation-specific status logic.
 */
bool mpu_int_status(void);
#endif

#endif // _MPU_H_
