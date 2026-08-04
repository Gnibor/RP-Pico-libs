#ifndef _HD44780_H_
#define _HD44780_H_

#include <stdbool.h>
#include <stdint.h>

#include "hd44780_iface.h"

struct hd44780_s{
	hd44780_iface_t iface;
	uint8_t cols;
	uint8_t rows;
	bool backlight;
};

bool hd44780_init(hd44780_t *lcd, hd44780_iface_t iface, uint8_t cols, uint8_t rows);
bool hd44780_backlight(hd44780_t *lcd, bool on);
bool hd44780_data(hd44780_t *lcd, uint8_t data);
bool hd44780_clear(hd44780_t *lcd);
bool hd44780_home(hd44780_t *lcd);
bool hd44780_set_geometry(hd44780_t *lcd, uint8_t cols, uint8_t rows);
bool hd44780_set_cursor(hd44780_t *lcd, uint8_t row, uint8_t col);
bool hd44780_putc(hd44780_t *lcd, char c);
bool hd44780_puts(hd44780_t *lcd, const char *s);
bool hd44780_printf(hd44780_t *lcd, uint8_t row, uint8_t col, uint8_t clear_to, const char *fmt, ...);

#endif // #ifdef _HD44780_H_
