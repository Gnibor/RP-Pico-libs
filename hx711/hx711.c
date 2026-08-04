#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "debug_helper.h"
#include "hx711.h"

#define HX711_SET_DIVISOR() switch(dev->conf){ \
		case CH_A_GAIN128: dev->divisor = 25.6f; break; \
		case CH_A_GAIN64:  dev->divisor = 12.8f; break; \
		case CH_B_GAIN32:  dev->divisor = 6.4f;  break; \
		default: dev->divisor = 1.0f;}


// =====================
// === Instance Init ===
// =====================
void hx711_init(hx711_t *dev, uint8_t sck, uint8_t din, hx711_config_t conf){
	dev->raw        = 0;
	dev->scaled     = 0.0f;

	dev->sck        = sck;
	dev->din        = din;
	dev->conf       = conf;
	dev->new_conf   = true;
	dev->offset     = 0;
	dev->gram_scale = 0.0f;

	HX711_SET_DIVISOR();

	gpio_init(dev->sck);
	gpio_set_dir(dev->sck, GPIO_OUT);
	gpio_put(dev->sck, 0);

	gpio_init(dev->din);
	gpio_set_dir(dev->din, GPIO_IN);
}

void hx711_set_config(hx711_t *dev, hx711_config_t conf){
	dev->conf = conf;
	dev->new_conf = true;
	HX711_SET_DIVISOR();
}

// ===========================
// === HX711 Read ============
// ===========================
void hx711_read(hx711_t *dev){
	dev->raw = 0;
	int8_t discard = dev->new_conf ? 1 : 0;
	uint8_t pulses = 0;
	int bit = 0;

	do{
		pulses = 0;
		bit = 0;
		dev->raw = 0;
		// 1. Warten bis Daten bereit
		while(gpio_get(dev->din));

		// 2. 24 Bit einlesen
		do{
			gpio_put(dev->sck, 1);
			sleep_us(1);  // Timing safe
			dev->raw = (dev->raw << 1) | gpio_get(dev->din);
			gpio_put(dev->sck, 0);
			sleep_us(1);

			bit++;
		}while(bit < 24);

		do{
			gpio_put(dev->sck, 1);
			sleep_us(1);
			gpio_put(dev->sck, 0);
			sleep_us(1);

			pulses++;
		}while(pulses < dev->conf);
	}while(discard--);

	// 4. Sign extend (24 → 32 Bit)
	dev->raw ^= 0x800000;

	dev->new_conf = false;

	dev->raw -= dev->offset;
}

// ===========================
// === Average ===============
// ===========================
void hx711_read_avg(hx711_t *dev, int samples){
	ASSERT(samples <= 1);
	int64_t sum = 0;

	for(int i = 0; i < samples; i++){
		hx711_read(dev);
		sum += dev->raw;
	}

	dev->raw = (int32_t)(sum / samples);
}

void hx711_set_offset(hx711_t *dev, int samples){
	hx711_read_avg(dev, samples);
	dev->offset = dev->raw;
}


/**
 * @brief Rechnet Rohwert in Mikrovolt um (Gain-abhängig).
 */
void hx711_read_uv(hx711_t *dev){
	hx711_read(dev);
	dev->scaled = (double)dev->raw / dev->divisor;
}

/**
 * @brief Rechnet Rohwert in Mikrovolt um (Gain-abhängig).
 */
void hx711_read_avg_uv(hx711_t *dev, uint8_t samples){
	hx711_read_avg(dev, samples);
	dev->scaled = (double)dev->raw / dev->divisor;
}

/**
 * @brief Liefert das aktuelle Gewicht in Gramm.
 * @return double Gewicht (unter Berücksichtigung von Tara und Kalibrierung).
 */
void hx711_read_gram(hx711_t *dev, int samples){
	hx711_read_avg_uv(dev, samples);           // In Spannung umrechnen
	dev->scaled /= dev->gram_scale;                // µV / (µV/g) = Gramm
}

/**
 * @brief Automatische Kalibrierung mit einem Referenzgewicht.
 * @param weight_gold Das bekannte Gewicht in Gramm (z.B. 100.0).
 */
void hx711_calibrate(hx711_t *dev, int32_t weight_gold, int samples){
	hx711_read_avg_uv(dev, samples);
	dev->gram_scale = dev->scaled / (double)weight_gold;
}

/**
 * @example hx711_simple_test.c
 * A small example how to use this driver.
 * @code
 * hx711_t dev;
 * hx711_init(&dev, 2, 3, CH_A_GAIN128);
 * hx711_set_offset(&dev, 10); // get/set the device offset
 * while(1){
 *    printf("\rValue: %.2fmV", (hx711_read_avg(&dev, 5)/256.0f));
 * }
 * @endcode
 */

/** @} */ // Ende der defgroup hx711_driver
