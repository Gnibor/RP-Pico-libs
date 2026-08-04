#ifndef HD44780_TEST_PCF8574_H
#define HD44780_TEST_PCF8574_H

#include <stdbool.h>
#include <stdint.h>

#include <hardware/structs/i2c.h>

#include "hd44780.h"
#include "hd44780_pcf8574.h"

typedef struct {
	hd44780_t lcd;
	hd44780_pcf8574_t dev;
	uint8_t cols;
	uint8_t rows;
} hd44780_test_pcf8574_t;

bool hd44780_test_pcf8574_init(hd44780_test_pcf8574_t *test, i2c_hw_t *i2c_hw, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t cols, uint8_t rows);
void hd44780_test_pcf8574_run(hd44780_test_pcf8574_t *test);

#endif
