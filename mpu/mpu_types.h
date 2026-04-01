#ifndef _MPU_TYPES_H_
#define _MPU_TYPES_H_

#include <stdint.h>

/**
 * @brief Sensor selection and modifier flags for read and calibration operations.
 *
 * @details
 * This bitmask is used to select one or more sensor groups for driver
 * operations such as sensor reads or calibration.
 *
 * Base sensor flags:
 * - @ref MPU_ACCEL selects the accelerometer
 * - @ref MPU_TEMP selects the temperature sensor
 * - @ref MPU_GYRO selects the gyroscope
 * - @ref MPU_ALL selects all three primary sensor groups
 *
 * Additional modifier flags:
 * - @ref MPU_SCALED requests conversion from raw register values to physical
 *   units, if supported by the called function
 * - @ref MPU_ACCEL_X, @ref MPU_ACCEL_Y, and @ref MPU_ACCEL_Z indicate the
 *   accelerometer axis used as gravity reference during calibration
 *
 * @note
 * The axis calibration flags include @ref MPU_ACCEL internally.
 */
typedef enum{
	MPU_ACCEL   =  (1 << 0), /**< Accelerometer sensor flag. */
	MPU_TEMP    =  (1 << 1), /**< Temperature sensor flag. */
	MPU_GYRO    =  (1 << 2), /**< Gyroscope sensor flag. */
	MPU_SCALED  =  (1 << 3), /**< Modifier flag: use scaled values instead of raw register values. */

	/* Axis modifiers for calibration */
	MPU_ACCEL_X = ((1 << 4) | MPU_ACCEL), /**< Accelerometer calibration with X-axis as gravity reference. */
	MPU_ACCEL_Y = ((1 << 5) | MPU_ACCEL), /**< Accelerometer calibration with Y-axis as gravity reference. */
	MPU_ACCEL_Z = ((1 << 6) | MPU_ACCEL), /**< Accelerometer calibration with Z-axis as gravity reference. */

	MPU_ALL     = (MPU_ACCEL | MPU_TEMP | MPU_GYRO) /**< All primary sensor flags combined. */
} mpu_sensor_t;

/**
 * @brief Cycle mode control values.
 *
 * @details
 * These values are used by @ref mpu_cycle_mode() to configure the cycle-related
 * power mode behavior of the MPU.
 *
 * The naming is intentionally register-oriented and follows the driver's
 * low-level configuration style.
 */
typedef enum {
	MPU_CYCLE_LP  = 2, /**< Low-power cycle mode selection. */
	MPU_CYCLE_ON  = 1, /**< Cycle mode enabled. */
	MPU_CYCLE_OFF = 0  /**< Cycle mode disabled. */
} mpu_cycle_t;

/**
 * @brief Sleep and temperature control flags for PWR_MGMT_1 handling.
 *
 * @details
 * These flags are intended for register-near driver configuration through
 * @ref mpu_sleep().
 *
 * The naming follows the enabled/disabled state of the corresponding control
 * function as used by this driver:
 * - @ref MPU_SLEEP_DEVICE_ON enables device sleep control
 * - @ref MPU_SLEEP_DEVICE_OFF disables device sleep control
 * - @ref MPU_SLEEP_TEMP_ON enables the temperature-related control flag
 * - @ref MPU_SLEEP_TEMP_OFF disables the temperature-related control flag
 *
 * @note
 * This enum is intentionally kept close to the register-oriented driver design
 * and should be interpreted in the context of the power-management register
 * layout.
 */
typedef enum {
	MPU_SLEEP_DEVICE_ON  = (1 << 0), /**< Enable device sleep control. */
	MPU_SLEEP_DEVICE_OFF = (0 << 0), /**< Disable device sleep control. */
	MPU_SLEEP_TEMP_ON    = (1 << 1), /**< Enable the temperature-related control flag. */
	MPU_SLEEP_TEMP_OFF   = (0 << 1), /**< Disable the temperature-related control flag. */
	MPU_SLEEP_ALL_OFF    = 0         /**< Clear all sleep-related control flags. */
} mpu_sleep_t;

/**
 * @brief Available 7-bit I2C device addresses.
 *
 * @details
 * The effective address depends on the hardware level applied to the AD0 pin.
 */
typedef enum {
	MPU_ADDR_AD0_GND = 0x68, /**< Device address when AD0 is tied to GND. */
	MPU_ADDR_AD0_VCC = 0x69  /**< Device address when AD0 is tied to VCC. */
} mpu_addr_t;

/**
 * @brief Reset control flags for internal modules and signal paths.
 *
 * @details
 * These flags are used by @ref mpu_reset() to request individual reset actions
 * or a full device reset.
 *
 * The enum is intentionally register-near and mirrors low-level reset control
 * usage rather than providing a high-level abstraction.
 */
typedef enum {
	MPU_RESET_ALL      = (1 << 0), /**< Reset all supported internal sensor and module paths. */
	MPU_RESET_TEMP     = (1 << 1), /**< Reset the temperature signal path. */
	MPU_RESET_ACCEL    = (1 << 2), /**< Reset the accelerometer signal path. */
	MPU_RESET_GYRO     = (1 << 3), /**< Reset the gyroscope signal path. */
	MPU_RESET_SIG_COND = (1 << 4), /**< Reset sensor signal conditioning paths. */
	MPU_RESET_I2C_MST  = (1 << 5), /**< Reset the internal I2C master block. */
	MPU_RESET_FIFO     = (1 << 6), /**< Reset the FIFO buffer. */
	MPU_RESET_DEVICE   = (1 << 7)  /**< Trigger a full device reset. */
} mpu_reset_t;

// =====================
// === Data Structur ===
// =====================
/**
 * @brief Main MPU device state structure.
 *
 * @details
 * This structure stores the latest sensor values read from the device.
 *
 * It contains:
 * - raw accelerometer, gyroscope, and temperature values
 * - scaled accelerometer values in g
 * - scaled gyroscope values in degrees per second
 * - scaled temperature in degrees Celsius
 *
 * @note
 * This structure currently represents sampled measurement state.
 * It does not yet store the full hardware configuration state.
 */
typedef struct mpu_value_t{
	// =====================
	// === Sensor Values ===
	// =====================
	struct{
		struct{ int16_t x,y,z; } raw; /**< Raw accelerometer register values. */
		struct{ float x,y,z; } g;     /**< Scaled accelerometer values in g. */
	} accel;

	struct{
		struct{ int16_t x,y,z; } raw; /**< Raw gyroscope register values. */
		struct{ float x,y,z; } dps;   /**< Scaled gyroscope values in degrees per second. */
	} gyro;

	struct{
		int16_t raw;   /**< Raw temperature register value. */
		float celsius; /**< Scaled temperature in degrees Celsius. */
	} temp;
} mpu_value_t;

#endif // _MPU_TYPES_H_
