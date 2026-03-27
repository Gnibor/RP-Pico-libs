/**
 * @file mpu.h
 * @author Robin Gerhartz (Gnibor)
 * @brief Low-level driver implementation for the MPU IMU sensor.
 *
 * @details
 * This driver for the Raspberry Pi Pico implements:
 * - I²C communication
 * - Register-level configuration
 * - Sensor data acquisition
 * - Automatic scaling (raw -> physical units)
 * - Gyroscope zero-point calibration
 * - Power management features
 * - Timing and cycle management
 * - Interrupt configuration and status monitoring
 *
 * The driver is written in a lightweight embedded style, optimized for 
 * performance and memory efficiency on the Raspberry Pi Pico.
 *
 * @project MPU Driver for Raspberry Pi Pico
 * @license MIT License (see LICENSE file in root)
 * @copyright Copyright (c) 2026 (Gnibor) Robin Gerhartz
 * @see https://github.com/Gnibor/MPU-Driver-Raspberry-Pi-Pico
 */
#ifndef MPU60X0_H
#define MPU60X0_H

#include <stdint.h>
#include "hardware/structs/i2c.h"
#include "mpu_reg_map.h"
#include "rp_pico.h"
#include "log.h"

// =============================
// === Configurable Hardware ===
// =============================
#ifndef MPU_I2C_HW
#define MPU_I2C_HW i2c1_hw // Default I2C port
#endif

#ifndef MPU_SDA_PIN
#define MPU_SDA_PIN 6    // Default SDA pin (can be overridden)
#endif

#ifndef MPU_SCL_PIN
#define MPU_SCL_PIN 7   // Default SCL pin (can be overridden)
#endif

#ifndef MPU_USE_PULLUP
#define MPU_USE_PULLUP 0 // 1 = enable internal pull-up, 0 = disabled
#endif

#ifndef MPU_INT_PIN
#define MPU_INT_PIN 26  // Optional interrupt pin (0 and the interrupt parts are not loaded)
#endif

#ifndef MPU_INT_PULLUP
#define MPU_INT_PULLUP 0 // 1 = enable internal pull-up, 0 = disabled
#endif

/**
 * @brief Sensors and modifiers for read and calibration operations.
 * 
 * Use @ref MPU_ACCEL, @ref MPU_GYRO or @ref MPU_TEMP to select sensors.
 * For @ref mpu_calibrate(), you must also specify which accelerometer axis 
 * is pointing "up" (against gravity) using the axis modifiers.
 */
typedef enum{
	MPU_ACCEL   =  (1 << 0), /**< Accelerometer sensor flag. 0b00000001 0x01 */
	MPU_TEMP    =  (1 << 1), /**< Temperature sensor flag. 0b00000010 0x02 */
	MPU_GYRO    =  (1 << 2), /**< Gyroscope sensor flag. 0b00000100 0x04 */
	MPU_SCALED  =  (1 << 3), /**< Modifier: Apply scaling to raw values. 0b00001000 0x08 */
	/* Axis modifiers for calibration */
	MPU_ACCEL_X = ((1 << 4) | MPU_ACCEL), /**< Calibrate Accel with X-axis against gravity. 0b00010001 0x11 */
	MPU_ACCEL_Y = ((1 << 5) | MPU_ACCEL), /**< Calibrate Accel with Y-axis against gravity. 0b00100001 0x21 */
	MPU_ACCEL_Z = ((1 << 6) | MPU_ACCEL), /**< Calibrate Accel with Z-axis against gravity. 0b01000001 0x41 */

	MPU_ALL     = (MPU_ACCEL | MPU_TEMP | MPU_GYRO) /**< All primary sensors. 0b00000111 0x07 */
} mpu_sensor_t;

/** @brief Power cycle modes for low power operation. */
typedef enum {
    MPU_CYCLE_LP  = 2, /**< Low Power Cycle mode. */
    MPU_CYCLE_ON  = 1, /**< Cycle mode enabled. */
    MPU_CYCLE_OFF = 0  /**< Cycle mode disabled (normal operation). */
} mpu_cycle_t;

/** @brief Sleep and Temperature sensor configurations for PWR_MGMT_1. */
typedef enum {
    MPU_SLEEP_DEVICE_ON  = (1 << 0), /**< Device is awake. */
    MPU_SLEEP_DEVICE_OFF = (0 << 0), /**< Device is in sleep mode. */
    MPU_SLEEP_TEMP_ON    = (1 << 1), /**< Temperature sensor is enabled. */
    MPU_SLEEP_TEMP_OFF   = (0 << 1), /**< Temperature sensor is disabled. */
    MPU_SLEEP_ALL_OFF    = 0         /**< Turn off all sleep-related overrides. */
} mpu_sleep_t;

/** @brief Possible I2C slave addresses depending on AD0 pin configuration. */
typedef enum {
    MPU_ADDR_AD0_GND = 0x68, /**< AD0 pin tied to Ground (default). */
    MPU_ADDR_AD0_VCC = 0x69  /**< AD0 pin tied to VCC. */
} mpu_addr_t;

/** @brief Bitmask for resetting specific internal registers or signal paths. */
typedef enum {
    MPU_RESET_ALL      = (1 << 0), /**< Reset all sensor registers and signal paths. */
    MPU_RESET_TEMP     = (1 << 1), /**< Reset temperature sensor signal path. */
    MPU_RESET_ACCEL    = (1 << 2), /**< Reset accelerometer signal path. */
    MPU_RESET_GYRO     = (1 << 3), /**< Reset gyroscope signal path. */
    MPU_RESET_SIG_COND = (1 << 4), /**< Reset all sensor signal conditioning. */
    MPU_RESET_I2C_MST  = (1 << 5), /**< Reset I2C Master module. */
    MPU_RESET_FIFO     = (1 << 6), /**< Reset FIFO buffer. */
    MPU_RESET_DEVICE   = (1 << 7)  /**< Trigger full chip reset. */
} mpu_reset_t;

// =====================
// === Data Structur ===
// =====================
/*
 * Main device structure
 *
 * This struct contains:
 * - Measured values
 * - Configuration state
 */
typedef struct mpu_s{
	// =====================
	// === Sensor Values ===
	// =====================
	struct{
		struct{
			struct{ int16_t x,y,z; } raw; // Raw accelerometer values
			struct{ float x,y,z; } g; // Converted acceleration in G
		} accel;

		struct{
			struct{ int16_t x,y,z; } raw; // Raw gyro values
			struct{ float x,y,z; } dps; // Converted gyro in °/s
		} gyro;

		struct{
			int16_t raw; // Raw temperature values
			float celsius; // Converted temperature in °C
		} temp;
	} v;

	// =====================
	// === Configuration ===
	// =====================
	struct{
		mpu_addr_t addr; // Device Address
		struct{ int32_t x, y, z; } offset_gyro, offset_accel;

		struct{ float accel, gyro; } fsr_div;
	} conf;
} mpu_s;

// ============================
// === Function declaration ===
// ============================
mpu_s mpu_init(i2c_hw_t *i2c_hw, mpu_addr_t addr);
bool mpu_use_struct(mpu_s *device);
bool mpu_who_am_i(void);
bool mpu_reset(mpu_reset_t reset);
bool mpu_sleep(mpu_sleep_t sleep); // Set sleep configuration
bool mpu_stby(mpu_stby_t stby);
bool mpu_clk_sel(mpu_clk_sel_t clksel);
bool mpu_smplrt_div(mpu_smplrt_div_t smplrt_div);
bool mpu_dlpf_cfg(mpu_dlpf_cfg_t cfg);
bool mpu_ahpf(mpu_ahpf_t ahpf);
bool mpu_fsr(mpu_fsr_t fsr, mpu_afsr_t afsr);

// need to be done
bool mpu_bypass(bool active);
bool mpu_calibrate(mpu_sensor_t sensor, uint8_t sample); // calibrate sensor offsets
bool mpu_read_sensor(mpu_sensor_t sensor);
bool mpu_cycle_mode(mpu_cycle_t mode, mpu_lp_wake_t wake_up_rate);
#if MPU_INT_PIN
bool mpu_int_pin_cfg(mpu_int_pin_cfg_t cfg);
bool mpu_int_enable(mpu_int_enable_t enable);
bool mpu_int_motion_cfg(uint8_t ms, uint16_t mg);
bool mpu_int_status(void);
#endif
#endif // MPU60X0_H
