#ifndef HD44780_GPIO_H
#define HD44780_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"

#include "hd44780.h"

typedef struct{
	uint8_t rs, rw, en, d4, d5, d6, d7, bl;
} hd44780_gpio_t;

bool hd44780_gpio_init(hd44780_t *lcd, hd44780_gpio_t *dev, uint8_t rs, uint8_t rw, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, uint8_t bl, uint8_t cols, uint8_t rows);

#endif
