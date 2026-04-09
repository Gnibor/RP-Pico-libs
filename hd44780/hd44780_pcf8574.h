#ifndef HD44780_PCF8574_H
#define HD44780_PCF8574_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/structs/i2c.h"

#include "hd44780.h"

typedef struct{
	uint8_t addr;
} hd44780_pcf8574_t;

bool hd44780_pcf8574_init(hd44780_t *lcd, hd44780_pcf8574_t *dev, i2c_hw_t *i2c_hw, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t cols, uint8_t rows);

bool hd44780_pcf8574_backlight(hd44780_t *lcd, bool on);

#endif
