#include "i2c_scan.h"

#include <stdio.h>
#include "pico/error_drv.h"

/**
 * @brief Scan the I2C bus for devices using a 1-byte read probe.
 *
 * @details
 * This checks whether a device acknowledges its 7-bit address.
 * The returned byte value is ignored. Only the ACK/NACK matters.
 */
bool i2c_scan_bus(i2c_inst_t *i2c, bool print_empty) {
    bool found_any = false;
    uint8_t dummy = 0;

    printf("Scanning I2C bus...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
        int ret = i2c_read_blocking(i2c, addr, &dummy, 1, false);

        if (ret == 1) {
            printf("I2C device found at 0x%02X\n", addr);
            found_any = true;
        } else if (print_empty) {
            printf("No response at 0x%02X\n", addr);
        }
    }

    if (!found_any) {
        printf("No I2C devices found.\n");
    }

    return found_any;
}
