#include "hd44780_pcf8574.h"
#include "log.h"
#include <hardware/structs/i2c.h>
#include "i2c.h"

#define PCF_RS (1u << 0)
#define PCF_RW (1u << 1)
#define PCF_EN (1u << 2)
#define PCF_BL (1u << 3)

static i2c_hw_config pcf8574_i2c;

static bool _pcf_write(hd44780_pcf8574_t *dev, uint8_t *buf, uint8_t size){
	return i2c_write_buffer(&pcf8574_i2c, dev->addr, buf, size, false) == 1;
}

static bool _pcf_write_nibble(hd44780_t *lcd, uint8_t nibble, bool rs){
	hd44780_pcf8574_t *dev = lcd->iface.ctx;
	uint8_t v = (nibble & 0x0F) << 4;
	uint8_t seq[3];

	if(rs) v |= PCF_RS;
	if(lcd->backlight) v |= PCF_BL;

	seq[0] = v;
	seq[1] = v | PCF_EN;
	seq[2] = v;

	if(!_pcf_write(dev, seq, 3)){
		LOG_E("write failed. v=0x%02X", v);
		return false;
	}
	sleep_us(50);

	return true;
}

bool hd44780_pcf8574_init(hd44780_t *lcd, hd44780_pcf8574_t *dev, i2c_hw_t *i2c_hw, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t cols, uint8_t rows){
	hd44780_iface_t iface;

	if(!i2c_hw){
		pcf8574_i2c.hw = i2c0_hw;
		LOG_W("i2c_hw = NULL, fallback=i2c0_hw");
	}else pcf8574_i2c.hw = i2c_hw;

	pcf8574_i2c.sda_pin = sda;
	pcf8574_i2c.scl_pin = scl;
	pcf8574_i2c.baudrate = 400000;
	pcf8574_i2c.timeout_us = 5000;

	if(!i2c_is_initialized(&pcf8574_i2c)){
		i2c_init(&pcf8574_i2c); // 400 kHz I2C
		LOG_I("I2C initialized baudrate=400000 sda=%d scl=%d", pcf8574_i2c.sda_pin, pcf8574_i2c.scl_pin);
	}else{
		LOG_W("I2C already initialized");
	}

	dev->addr = addr;
	lcd->backlight = true;

	if(!_pcf_write(dev, (uint8_t[]){PCF_BL}, 1)) return false;

	iface.ctx = dev;
	iface.write_nibble = _pcf_write_nibble;

	return hd44780_init(lcd, iface, cols, rows);
}
