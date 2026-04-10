#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#include "hd44780_iface.h"
#include "pico/stdlib.h"
#include "string.h"

#include "log.h"
#include "hd44780.h"

#define HD44780_CLEAR        0x01
#define HD44780_HOME         0x02
#define HD44780_ENTRY_MODE   0x04
#define HD44780_DISPLAY_CTRL 0x08
#define HD44780_FUNCTION_SET 0x20
#define HD44780_SET_DDRAM    0x80

#define HD44780_ENTRY_INC    0x02
#define HD44780_DISPLAY_ON   0x04
#define HD44780_2LINE        0x08
#define HD44780_1LINE        0x00

static 	char hd44780_buf[21];

static bool _hd44780_write_byte(hd44780_t *lcd, uint8_t byte, bool rs){
	if(!lcd->iface.write_nibble(lcd, byte >> 4, rs)){
		LOG_E("^^^ write failed");
		return false;
	}
	if(!lcd->iface.write_nibble(lcd, byte, rs)){
		LOG_E("^^^ write failed");
		return false;
	}
	return true;
}

static bool _hd44780_command(hd44780_t *lcd, uint8_t cmd){
	if(!_hd44780_write_byte(lcd, cmd, false)){
		LOG_E("^^^ write failed");
		return false;
	}
	if(cmd == HD44780_CLEAR || cmd == HD44780_HOME) sleep_ms(2);

	return true;
}

static bool _hd44780_apply_geometry(hd44780_t *lcd){
	uint8_t function;

	if(!lcd) return false;

	function = HD44780_FUNCTION_SET | ((lcd->rows > 1) ? HD44780_2LINE : HD44780_1LINE);

	if(!_hd44780_write_byte(lcd, function, false)){
		LOG_E("geometry function set failed");
		return false;
	}

	return true;
}

bool hd44780_init(hd44780_t *lcd, hd44780_iface_t iface, uint8_t cols, uint8_t rows){
	lcd->iface = iface;
	lcd->cols = cols;
	lcd->rows = rows;

	sleep_ms(50);

	if(!iface.write_nibble(lcd, 0x03, false)){
		LOG_E("^^^ wakeup write failed");
		return false;
	}
	sleep_ms(5);

	if(!iface.write_nibble(lcd, 0x03, false)){
		LOG_E("^^^ wakeup write failed");
		return false;
	}
	sleep_us(150);

	if(!iface.write_nibble(lcd, 0x03, false)){
		LOG_E("^^^ wakeup write failed");
		return false;
	}
	sleep_us(150);

	if(!iface.write_nibble(lcd, 0x02, false)){
		LOG_E("^^^ 4-bit write failed");
		return false;
	}
	sleep_us(150);

	if(!_hd44780_apply_geometry(lcd)){
		LOG_E("geometry init failed");
		return false;
	}

	if(!_hd44780_write_byte(lcd, HD44780_DISPLAY_CTRL | HD44780_DISPLAY_ON, false)){
		LOG_E("^^^ write failed");
		return false;
	}
	if(!_hd44780_write_byte(lcd, HD44780_CLEAR, false)){
		LOG_E("^^^ write failed");
		return false;
	}
	sleep_ms(2);
	if(!_hd44780_write_byte(lcd, HD44780_ENTRY_MODE | HD44780_ENTRY_INC, false)){
		LOG_E("^^^ write failed");
		return false;
	}

	return true;
}

bool hd44780_backlight(hd44780_t *lcd, bool on){
	lcd->backlight = on;
	return _hd44780_write_byte(lcd, 0x00, false);
}

bool hd44780_clear(hd44780_t *lcd){ return _hd44780_command(lcd, HD44780_CLEAR); }
bool hd44780_home(hd44780_t *lcd){ return _hd44780_command(lcd, HD44780_HOME); }

bool hd44780_set_geometry(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	if(!lcd) return false;
	if(cols < 1 || rows < 1 || rows > 4) return false;

	lcd->cols = cols;
	lcd->rows = rows;

	if(!_hd44780_apply_geometry(lcd)) return false;

	return true;
}

bool hd44780_set_cursor(hd44780_t *lcd, uint8_t row, uint8_t col){
	static const uint8_t off[] = { 0x00, 0x40, 0x14, 0x54 };

	return _hd44780_command(lcd, HD44780_SET_DDRAM | (off[row] + col));
}

bool hd44780_putc(hd44780_t *lcd, char c){
	return _hd44780_write_byte(lcd, c, true);
}

bool hd44780_puts(hd44780_t *lcd, const char *s){
	while(*s) if(!hd44780_putc(lcd, *s++)){
		LOG_E("^^^ write failed");
		return false;
	}
	return true;
}

bool hd44780_printf(hd44780_t *lcd, uint8_t row, uint8_t col, uint8_t clear_to, const char *fmt, ...){
	memset(&hd44780_buf, 0, 21);
	va_list ap;
	int len;
	uint8_t cur;

	if(clear_to >= lcd->cols) clear_to = lcd->cols - 1;
	if(clear_to < col) clear_to = col;

	va_start(ap, fmt);
	len = vsnprintf(hd44780_buf, sizeof(hd44780_buf), fmt, ap);
	va_end(ap);

	if(len < 0) {
		LOG_E("fmt failed. len=%d", len);
		return false;
	}
	if(!hd44780_set_cursor(lcd, row, col)){
		LOG_E("set cursour failed. row=%d, col=%d", row, col);
		return false;
	}
	cur = col;

	for(size_t i = 0; hd44780_buf[i] && hd44780_buf[i] != '\n' && cur < lcd->cols; i++, cur++)
		if(!hd44780_putc(lcd,hd44780_buf[i])){
		LOG_E("putc failed. len=%d", len);
		return false;
	}

	while(cur <= clear_to && cur < lcd->cols){
		if(!hd44780_putc(lcd, ' ')){
		LOG_E("putc failed. len=%d", len);
		return false;
	}
		cur++;
	}

	return true;
}
