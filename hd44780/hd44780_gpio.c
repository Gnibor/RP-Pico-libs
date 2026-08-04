#include "hd44780_gpio.h"

static bool _gpio_write_nibble(hd44780_t *lcd, uint8_t nibble, bool rs){
	hd44780_gpio_t *dev = lcd->iface.ctx;

	nibble &= 0x0F;

	gpio_put(dev->rs, rs);
	if(dev->rw != 0xFF) gpio_put(dev->rw, 0);

	gpio_put(dev->d4, (nibble >> 0) & 1u);
	gpio_put(dev->d5, (nibble >> 1) & 1u);
	gpio_put(dev->d6, (nibble >> 2) & 1u);
	gpio_put(dev->d7, (nibble >> 3) & 1u);

	if(dev->bl != 0xFF) gpio_put(dev->bl, lcd->backlight);

	gpio_put(dev->en, 1);
	sleep_us(1);
	gpio_put(dev->en, 0);
	sleep_us(50);

	return true;
}

bool hd44780_gpio_init(hd44780_t *lcd, hd44780_gpio_t *dev,
	uint8_t rs, uint8_t rw, uint8_t en,
	uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
	uint8_t bl, uint8_t cols, uint8_t rows){

	hd44780_iface_t iface;
	uint8_t pins[] = { rs, en, d4, d5, d6, d7 };

	if(!lcd || !dev) return false;

	dev->rs = rs;
	dev->rw = rw;
	dev->en = en;
	dev->d4 = d4;
	dev->d5 = d5;
	dev->d6 = d6;
	dev->d7 = d7;
	dev->bl = bl;

	lcd->backlight = true;

	for(size_t i = 0; i < sizeof(pins)/sizeof(pins[0]); i++){
		gpio_init(pins[i]);
		gpio_set_dir(pins[i], GPIO_OUT);
		gpio_put(pins[i], 0);
	}

	if(rw != 0xFF){
		gpio_init(rw);
		gpio_set_dir(rw, GPIO_OUT);
		gpio_put(rw, 0);
	}

	if(bl != 0xFF){
		gpio_init(bl);
		gpio_set_dir(bl, GPIO_OUT);
		gpio_put(bl, 1);
	}

	iface.ctx = dev;
	iface.write_nibble = _gpio_write_nibble;

	return hd44780_init(lcd, iface, cols, rows);
}
