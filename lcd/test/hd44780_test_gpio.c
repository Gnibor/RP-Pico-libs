#include "hd44780_test_gpio.h"
#include "pico/stdlib.h"

#define BTN_PRESSED(pin) (gpio_get(pin) == 0)

static void _wait_release(uint8_t pin){
	while(BTN_PRESSED(pin)) sleep_ms(10);
	sleep_ms(20);
}

static bool _select_value(hd44780_t *lcd, uint8_t btn_up, uint8_t btn_down, uint8_t btn_ok, const char *label, uint8_t *value, uint8_t min, uint8_t max){
	absolute_time_t next_repeat;
	bool up_hold = false;
	bool down_hold = false;

	if(!lcd || !label || !value) return false;

	while(1){
		hd44780_clear(lcd);
		hd44780_printf(lcd, 0, 0, lcd->cols - 1, "%s: %u", label, *value);
		if(lcd->rows > 1) hd44780_printf(lcd, 1, 0, lcd->cols - 1, "UP/DN OK");

		up_hold = false;
		down_hold = false;

		while(1){
			if(BTN_PRESSED(btn_ok)){
				_wait_release(btn_ok);
				return true;
			}

			if(BTN_PRESSED(btn_up)){
				if(*value < max) (*value)++;
				next_repeat = make_timeout_time_ms(400);
				up_hold = true;
				down_hold = false;
				sleep_ms(20);
				break;
			}

			if(BTN_PRESSED(btn_down)){
				if(*value > min) (*value)--;
				next_repeat = make_timeout_time_ms(400);
				down_hold = true;
				up_hold = false;
				sleep_ms(20);
				break;
			}

			sleep_ms(10);
		}

		while(up_hold || down_hold){
			if(BTN_PRESSED(btn_ok)){
				_wait_release(btn_ok);
				return true;
			}

			if(up_hold){
				if(!BTN_PRESSED(btn_up)){
					up_hold = false;
					sleep_ms(20);
					break;
				}
				if(time_reached(next_repeat)){
					if(*value < max) (*value)++;
					hd44780_printf(lcd, 0, 0, lcd->cols - 1, "%s: %u", label, *value);
					next_repeat = make_timeout_time_ms(120);
				}
			}

			if(down_hold){
				if(!BTN_PRESSED(btn_down)){
					down_hold = false;
					sleep_ms(20);
					break;
				}
				if(time_reached(next_repeat)){
					if(*value > min) (*value)--;
					hd44780_printf(lcd, 0, 0, lcd->cols - 1, "%s: %u", label, *value);
					next_repeat = make_timeout_time_ms(120);
				}
			}

			sleep_ms(10);
		}
	}
}

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

	if(rows >= 1) hd44780_printf(lcd, 0, 0, cols - 1, "HD44780 GPIO");
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
			sleep_ms(150);
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

static void _test_counter(hd44780_test_gpio_t *test){
	int i = 0;

	while(1){
		for(uint8_t row = 0; row < test->rows; row++) hd44780_printf(&test->lcd, row, 0, test->cols - 1, "Row%u Cnt:%d", row, i + row);

		if(BTN_PRESSED(test->btn_ok)){
			_wait_release(test->btn_ok);
			return;
		}

		i++;
		sleep_ms(500);
	}
}

static void _run_test_sequence(hd44780_test_gpio_t *test){
	_test_intro(&test->lcd, test->cols, test->rows);
	_test_fill(&test->lcd, test->cols, test->rows);
	_test_cursor_walk(&test->lcd, test->cols, test->rows);
	_test_printf(&test->lcd, test->cols, test->rows);
	_test_numbers(&test->lcd, test->cols, test->rows);
	_test_home_clear(&test->lcd, test->cols, test->rows);
}

static uint8_t _menu(hd44780_test_gpio_t *test){
	hd44780_clear(&test->lcd);
	hd44780_printf(&test->lcd, 0, 0, test->cols - 1, "U:Test D:Geom");
	if(test->rows > 1) hd44780_printf(&test->lcd, 1, 0, test->cols - 1, "OK:Counter");

	while(1){
		if(BTN_PRESSED(test->btn_up)){
			_wait_release(test->btn_up);
			return 0;
		}
		if(BTN_PRESSED(test->btn_down)){
			_wait_release(test->btn_down);
			return 1;
		}
		if(BTN_PRESSED(test->btn_ok)){
			_wait_release(test->btn_ok);
			return 2;
		}

		sleep_ms(10);
	}
}

bool hd44780_test_gpio_init(hd44780_test_gpio_t *test, uint8_t rs, uint8_t rw, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, uint8_t bl, uint8_t cols, uint8_t rows){
	if(!test) return false;

	test->cols = cols;
	test->rows = rows;
	test->btn_up = 0xFF;
	test->btn_down = 0xFF;
	test->btn_ok = 0xFF;

	return hd44780_gpio_init(&test->lcd, &test->dev, rs, rw, en, d4, d5, d6, d7, bl, cols, rows);
}

bool hd44780_test_gpio_buttons_init(hd44780_test_gpio_t *test, uint8_t btn_up, uint8_t btn_down, uint8_t btn_ok){
	if(!test) return false;

	test->btn_up = btn_up;
	test->btn_down = btn_down;
	test->btn_ok = btn_ok;

	gpio_init(btn_up);
	gpio_set_dir(btn_up, GPIO_IN);
	gpio_pull_up(btn_up);

	gpio_init(btn_down);
	gpio_set_dir(btn_down, GPIO_IN);
	gpio_pull_up(btn_down);

	gpio_init(btn_ok);
	gpio_set_dir(btn_ok, GPIO_IN);
	gpio_pull_up(btn_ok);

	return true;
}

bool hd44780_test_gpio_select_geometry(hd44780_test_gpio_t *test){
	uint8_t cols = test->cols ? test->cols : 16;
	uint8_t rows = test->rows ? test->rows : 2;

	if(!test) return false;
	if(test->btn_up == 0xFF || test->btn_down == 0xFF || test->btn_ok == 0xFF) return false;

	if(!_select_value(&test->lcd, test->btn_up, test->btn_down, test->btn_ok, "Cols", &cols, 8, 40)) return false;
	if(!_select_value(&test->lcd, test->btn_up, test->btn_down, test->btn_ok, "Rows", &rows, 1, 4)) return false;

	test->cols = cols;
	test->rows = rows;
	test->lcd.cols = cols;
	test->lcd.rows = rows;

	hd44780_clear(&test->lcd);
	hd44780_printf(&test->lcd, 0, 0, cols - 1, "Set: %ux%u", cols, rows);
	if(rows > 1) hd44780_printf(&test->lcd, 1, 0, cols - 1, "OK");
	sleep_ms(1200);

	return true;
}

void hd44780_test_gpio_run(hd44780_test_gpio_t *test){
	uint8_t action;

	if(!test) return;

	while(1){
		action = _menu(test);

		if(action == 0){
			_run_test_sequence(test);
		}else if(action == 1){
			hd44780_test_gpio_select_geometry(test);
		}else{
			_test_counter(test);
		}
		sleep_ms(100);
	}
}
