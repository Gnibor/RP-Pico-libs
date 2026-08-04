#ifndef _MPU_CFG_H_

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

#endif // _MPU_CFG_H_
