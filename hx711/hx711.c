#include <pico/time.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "debug_helper.h"
#include "hx711.h"

// =====================
// === Instance Init ===
// =====================
void hx711_init(hx711_t *dev, uint8_t sck, uint8_t din, hx711_config_t conf){
	dev->sck        = sck;
	dev->din        = din;
	dev->conf       = conf;
	dev->new_conf   = true;
	dev->offset     = 0;
	dev->gram_scale = 0.0f;

	gpio_init(dev->sck);
	gpio_set_dir(dev->sck, GPIO_OUT);
	gpio_put(dev->sck, 0);

	gpio_init(dev->din);
	gpio_set_dir(dev->din, GPIO_IN);
}

void hx711_set_config(hx711_t *dev, hx711_config_t conf){
	dev->conf = conf;
	dev->new_conf = true;
}

// ===========================
// === HX711 Read ============
// ===========================
int32_t hx711_read(hx711_t *dev){
	int32_t value = 0;
	int8_t discard = dev->new_conf ? 1 : 0;

	do{
		uint8_t pulses = 0;
		value = 0;
		// 1. Warten bis Daten bereit
		while(gpio_get(dev->din));

		// 2. 24 Bit einlesen
		for(int bit = 0; bit < 24; bit++){
			gpio_put(dev->sck, 1);
			sleep_us(1);  // Timing safe
			value = (value << 1) | gpio_get(dev->din);
			gpio_put(dev->sck, 0);
			sleep_us(1);
		}

		do{
			pulses++;
			gpio_put(dev->sck, 1);
			sleep_us(1);
			gpio_put(dev->sck, 0);
			sleep_us(1);
		}while(pulses < dev->conf);
	}while(discard--);

	// 4. Sign extend (24 → 32 Bit)
	value ^= 0x800000;

	dev->new_conf = false;

	return (value - dev->offset);
}

// ===========================
// === Average ===============
// ===========================
int32_t hx711_read_avg(hx711_t *dev, int samples){
	ASSERT(samples <= 1);
	int64_t sum = 0;

	for(int i = 0; i < samples; i++){
		sum += hx711_read(dev);
	}

	return (int32_t)(sum / samples);
}

void hx711_set_offset(hx711_t *dev, int samples){
	dev->offset = hx711_read_avg(dev, samples);
}


/**
 * @brief Rechnet Rohwert in Mikrovolt um (Gain-abhängig).
 */
float hx711_read_uv(hx711_t *dev) {
	int32_t raw = hx711_read(dev);
	float gain_divisor = 1.0f;

	switch (dev->conf) {
		case CH_A_GAIN128: gain_divisor = 25.6f; break; 
		case CH_A_GAIN64:  gain_divisor = 12.8f; break;
		case CH_B_GAIN32:  gain_divisor = 6.4f;  break;
	}
	return (float)raw / gain_divisor;
}

/**
 * @brief Rechnet Rohwert in Mikrovolt um (Gain-abhängig).
 */
float hx711_read_avg_uv(hx711_t *dev, uint8_t samples) {
	int32_t raw = hx711_read_avg(dev, samples);
	float gain_divisor = 1.0f;

	switch (dev->conf) {
		case CH_A_GAIN128: gain_divisor = 25.6f; break; 
		case CH_A_GAIN64:  gain_divisor = 12.8f; break;
		case CH_B_GAIN32:  gain_divisor = 6.4f;  break;
	}
	return (float)raw / gain_divisor;
}

/**
 * @brief Liefert das aktuelle Gewicht in Gramm.
 * @return float Gewicht (unter Berücksichtigung von Tara und Kalibrierung).
 */
float hx711_get_gram(hx711_t *dev, int samples) {
	float uv = hx711_read_avg_uv(dev, samples);           // In Spannung umrechnen

	if (dev->gram_scale == 0.0f) return uv;     // Falls nicht kalibriert, µV zurückgeben
	return uv / dev->gram_scale;                // µV / (µV/g) = Gramm
}

/**
 * @brief Automatische Kalibrierung mit einem Referenzgewicht.
 * @param weight_gold Das bekannte Gewicht in Gramm (z.B. 100.0).
 */
void hx711_calibrate(hx711_t *dev, int32_t weight_gold, int samples) {
	// 2. Spannung in µV berechnen
	float uv = hx711_read_avg_uv(dev, samples);
	// 3. Berechnen: Wie viel µV pro Gramm?
	dev->gram_scale = uv / (float)weight_gold;
}

/**
 * @example hx711_simple_test.c
 * A small example how to use this driver.
 * @code
 * hx711_t dev;
 * hx711_init(&dev, 2, 3, CH_A_GAIN128);
 * hx711_set_offset(&dev, 10); // get/set the device offset
 * while(1) {
 *    printf("\rValue: %.2fmV", (hx711_read_avg(&dev, 5)/256.0f));
 * }
 * @endcode
 */

/** @} */ // Ende der defgroup hx711_driver
