#ifndef I2C_SCAN_H
#define I2C_SCAN_H

#include "hardware/i2c.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Scan the I2C bus for 7-bit device addresses.
 *
 * @param i2c Pointer to the I2C instance (e.g. i2c0 or i2c1)
 * @param print_empty If true, also print addresses where no device responded
 * @return true if at least one device was found, otherwise false
 */
bool i2c_scan_bus(i2c_inst_t *i2c, bool print_empty);

#ifdef __cplusplus
}
#endif

#endif /* I2C_SCAN_H */
