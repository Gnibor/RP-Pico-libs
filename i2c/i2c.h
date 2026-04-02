#ifndef _I2C_H_
#define _I2C_H_

/**
 * @file _i2c_lowlevel.h
 * @brief Low-level I2C driver for RP2040 and RP2350 using direct register access.
 */

#include <stdint.h>
#include <stdbool.h>
#include <hardware/structs/i2c.h>

/**
 * @struct _i2c_hw_config
 * @brief Hardware configuration for a specific I2C instance.
 */
typedef struct {
    i2c_hw_t *hw;          ///< Base address of the I2C controller (i2c0_hw or i2c1_hw)
    uint32_t sda_pin;      ///< GPIO number for Data line
    uint32_t scl_pin;      ///< GPIO number for Clock line
    uint32_t baudrate;     ///< Target bus speed in Hz (e.g., 400000)
    uint32_t timeout_us;   ///< Safety timeout for hardware loops in microseconds
} _i2c_hw_config;

/** @brief Checks if the hardware block is enabled and in Master Mode. */
bool _i2c_is_initialized(const _i2c_hw_config *cfg);

/** @brief Returns true if the I2C hardware Finite State Machine is active. */
bool _i2c_is_busy(const _i2c_hw_config *cfg);

/** @brief Attempts to clear a bus hang (SDA stuck LOW) by toggling SCL. */
void _i2c_recover_bus(const _i2c_hw_config *cfg);

/** @brief Resets and initializes the I2C controller with the given config. */
void _i2c_init(_i2c_hw_config *cfg);

/** @brief Writes an array of bytes to the specified slave address. */
bool _i2c_write_buffer(const _i2c_hw_config *cfg, uint8_t addr, const uint8_t *src, size_t len, bool nostop);

/** @brief Reads an array of bytes from the specified slave address. */
bool _i2c_read_buffer(const _i2c_hw_config *cfg, uint8_t addr, uint8_t *dst, size_t len);

#endif // _I2C_H_
