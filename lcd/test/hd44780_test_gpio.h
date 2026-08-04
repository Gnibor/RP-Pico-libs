#ifndef HD44780_TEST_GPIO_H
#define HD44780_TEST_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "hd44780.h"
#include "hd44780_gpio.h"

typedef struct {
	hd44780_t lcd;
	hd44780_gpio_t dev;

	uint8_t cols;
	uint8_t rows;

	uint8_t btn_up;
	uint8_t btn_down;
	uint8_t btn_ok;
} hd44780_test_gpio_t;

/**
 * Init LCD + GPIO backend
 */
bool hd44780_test_gpio_init(
	hd44780_test_gpio_t *test,
	uint8_t rs,
	uint8_t rw,
	uint8_t en,
	uint8_t d4,
	uint8_t d5,
	uint8_t d6,
	uint8_t d7,
	uint8_t bl,
	uint8_t cols,
	uint8_t rows
);

/**
 * Init buttons (intern mit Pull-Ups)
 */
bool hd44780_test_gpio_buttons_init(
	hd44780_test_gpio_t *test,
	uint8_t btn_up,
	uint8_t btn_down,
	uint8_t btn_ok
);

/**
 * Interaktive Auswahl von cols/rows über Buttons
 */
bool hd44780_test_gpio_select_geometry(
	hd44780_test_gpio_t *test
);

/**
 * Startet Menü + Testsuite
 */
void hd44780_test_gpio_run(
	hd44780_test_gpio_t *test
);

#endif
