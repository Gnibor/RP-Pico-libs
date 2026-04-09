#ifndef HD44780_IFACE_H
#define HD44780_IFACE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct hd44780_s hd44780_t;

typedef struct{
	void *ctx;
	bool (*write_nibble)(hd44780_t *lcd, uint8_t nibble, bool rs);
} hd44780_iface_t;

#endif
