#include "hd44780_test_pcf8574.h"
#include "pico/stdlib.h"

static void _fill_line(hd44780_t *lcd, uint8_t row, uint8_t cols, char start){
	char c = start;

	for(uint8_t col = 0; col < cols; col++){
		hd44780_set_cursor(lcd, row, col);
		hd44780_putc(lcd, c);
		c++;
		if(c > 'Z') c = 'A';
	}
}

static void _test_intro(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	hd44780_clear(lcd);

	if(rows >= 1) hd44780_printf(lcd, 0, 0, cols - 1, "HD44780 PCF");
	if(rows >= 2) hd44780_printf(lcd, 1, 0, cols - 1, "%ux%u Init OK", cols, rows);
	if(rows >= 3) hd44780_printf(lcd, 2, 0, cols - 1, "Row 3 OK");
	if(rows >= 4) hd44780_printf(lcd, 3, 0, cols - 1, "Row 4 OK");

	sleep_ms(1500);
}

static void _test_fill(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	hd44780_clear(lcd);

	for(uint8_t row = 0; row < rows; row++) _fill_line(lcd, row, cols, 'A' + (row * 6));

	sleep_ms(1500);
}

static void _test_cursor_walk(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	hd44780_clear(lcd);

	for(uint8_t row = 0; row < rows; row++){
		for(uint8_t col = 0; col < cols; col++){
			hd44780_set_cursor(lcd, row, col);
			hd44780_putc(lcd, '*');
			sleep_ms(60);
			hd44780_set_cursor(lcd, row, col);
			hd44780_putc(lcd, ' ');
		}
	}
}

static void _test_printf(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	hd44780_clear(lcd);

	for(uint8_t row = 0; row < rows; row++) hd44780_printf(lcd, row, 0, cols - 1, "row=%u cols=%u", row, cols);

	sleep_ms(1500);
}

static void _test_numbers(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	hd44780_clear(lcd);

	for(int i = 0; i < 10; i++){
		for(uint8_t row = 0; row < rows; row++) hd44780_printf(lcd, row, 0, cols - 1, "R%u Val:%d", row, i + row);

		sleep_ms(250);
	}
}

static void _test_home_clear(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	hd44780_clear(lcd);

	for(uint8_t row = 0; row < rows; row++) hd44780_printf(lcd, row, 0, cols - 1, "Home/Clear %u", row);

	sleep_ms(1000);
	hd44780_home(lcd);
	sleep_ms(300);
	hd44780_clear(lcd);
	sleep_ms(300);

	for(uint8_t row = 0; row < rows; row++) hd44780_printf(lcd, row, 0, cols - 1, "Clear OK %u", row);

	sleep_ms(1200);
}

static void _test_counter(hd44780_t *lcd, uint8_t cols, uint8_t rows){
	int i = 0;

	while(1){
		for(uint8_t row = 0; row < rows; row++) hd44780_printf(lcd, row, 0, cols - 1, "Row%u Cnt:%d", row, i + row);

		i++;
		sleep_ms(500);
	}
}

bool hd44780_test_pcf8574_init(hd44780_test_pcf8574_t *test, i2c_hw_t *i2c_hw, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t cols, uint8_t rows){
	if(!test) return false;

	test->cols = cols;
	test->rows = rows;

	return hd44780_pcf8574_init(&test->lcd, &test->dev, i2c_hw, sda, scl, addr, cols, rows);
}

void hd44780_test_pcf8574_run(hd44780_test_pcf8574_t *test){
	if(!test) return;

	_test_intro(&test->lcd, test->cols, test->rows);
	_test_fill(&test->lcd, test->cols, test->rows);
	_test_cursor_walk(&test->lcd, test->cols, test->rows);
	_test_printf(&test->lcd, test->cols, test->rows);
	_test_numbers(&test->lcd, test->cols, test->rows);
	_test_home_clear(&test->lcd, test->cols, test->rows);
	_test_counter(&test->lcd, test->cols, test->rows);
}
