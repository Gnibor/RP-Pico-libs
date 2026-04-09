#ifndef _HX711_H_
#define _HX711_H_
/**
 * @defgroup hx711_driver HX711 ADC Driver
 * @brief Driver for the HX711 24-Bit ADC (load cell amplifier).
 * @{
 */

/**
 * @file hx711.h
 * @author Robin Gerhartz
 * @brief API definitions for the 24-Bit ADC amplifier HX711 on the Raspberry Pi Pico (2040/2350)
 *
 * @details
 * This driver provides a read out from a HX711 24-Bit ADC,
 * for load cells or other tiny electrical complement currents.
 */

#include <stdint.h>   /**< Fixed-width integer types such as uint8_t and int32_t. */
#include <stdbool.h>  /**< Boolean type support for bool. */

/**
 * @enum hx711_config_t
 * @brief HX711 channel / gain selection.
 *
 * @details
 * The numeric values are intentionally chosen to match the number of
 * additional clock pulses required by the HX711 after the 24 data bits
 * have been shifted out:
 */
typedef enum{
	CH_A_GAIN128 = 1, /**< Channel A, Gain 128 (device default) */
	CH_B_GAIN32,      /**< Channel B, Gain 32 */
	CH_A_GAIN64,      /**< Channel A, Gain 64 */
} hx711_config_t;

/**
 * @struct hx711_t
 * @brief HX711 device descriptor.
 *
 * @details
 * This structure stores both the hardware pin assignment and the current
 * software state of the HX711 driver instance.
 *
 * @note Values should not edited manuell, please use the hx711_ functions.
 */
typedef struct{
	int32_t raw; /**< Raw - offset (if set) for you to read. */
	double scaled; /**< The scaled value if you used @ref hx711_read_uv or @ref hx711_read_gram */

/// @note Private (Please use the declarated functions)
	uint8_t sck, din;     /**< GPIO numbers for HX711 clock (SCK) and data output (DOUT). */
	hx711_config_t conf;  /**< Set HX711 device channel / gain configuration. */
	bool new_conf;        /**< True if a new configuration was set and one conversion must be discarded. */
	float divisor;
	int32_t offset;       /**< Software offset subtracted from each returned reading. */
	double gram_scale;    /**< Kalibrierfaktor: Wie viele µV sind 1 Gramm? */
} hx711_t;

/**
 * @brief Initialize an HX711 driver instance.
 *
 * @param dev  Pointer to the @ref hx711_t device structure to initialize.
 * @param sck  GPIO number connected to the HX711 SCK / PD_SCK pin.
 * @param din  GPIO number connected to the HX711 DOUT pin.
 * @param conf Initial channel / gain configuration.
 *
 * @details
 * This function:
 * - stores the pin mapping
 * - stores the initial channel / gain configuration
 * - clears the software offset
 * - marks the configuration as new so the first stale conversion is discarded
 * - initializes the GPIO directions for SCK and DOUT
 *
 * After initialization, the first returned value will correspond to the
 * selected configuration because the driver internally discards the old one.
 */
void hx711_init(hx711_t *dev, uint8_t sck, uint8_t din, hx711_config_t conf);

/**
 * @brief Change the HX711 channel / gain configuration for the next read.
 *
 * @param dev   Pointer to the @ref hx711_t device structure.
 * @param conf  New channel / gain configuration to request.
 *
 * @details
 * The HX711 does not apply a new configuration immediately to the value
 * currently being read. Instead, the new configuration affects the next
 * conversion cycle. The driver therefore marks the configuration as new
 * so the next stale reading is discarded automatically.
 */
void hx711_set_config(hx711_t *dev, hx711_config_t conf);

/**
 * @brief Read one HX711 conversion result.
 *
 * @param[in,out] dev Pointer to the @ref hx711_t device structure.
 *
 * @return @ref hx711_t.offset corrected signed 24-bit conversion result as int32_t.
 *
 * @details
 * This function:
 * - waits until DOUT goes low, indicating that data is ready
 * - shifts in 24 data bits from the HX711
 * - generates the extra clock pulses needed to select the next channel / gain
 * - sign-extends the 24-bit two's-complement value to 32 bits
 * - subtracts the configured software offset before returning the result
 *
 * If a new configuration was set before this call, the first conversion
 * is discarded internally and the second one is returned.
 */
void hx711_read(hx711_t *dev);

/**
 * @brief Read multiple HX711 samples and return their arithmetic mean.
 *
 * @param dev      Pointer to the @ref hx711_t device structure.
 * @param samples  Number of samples to read and average.
 *
 * @return Averaged @ref hx711_t.offset corrected reading.
 *
 * @details
 * This helper reduces noise by accumulating multiple consecutive readings
 * and dividing the sum by the number of requested samples.
 *
 * @warning
 * The caller must ensure that @p samples is greater than 1.
 */
void hx711_read_avg(hx711_t *dev, int samples);

/**
 * @brief Measure and store the current software offset in @ref hx711_t.offset
 *
 * @param dev      Pointer to the @ref hx711_t device structure.
 * @param samples  Number of samples used to determine the offset.
 *
 * @details
 * This function reads the current sensor value multiple times, averages
 * the result, and stores it in @ref hx711_t.offset
 *
 * Future calls to hx711_read() and hx711_read_avg() return values with
 * this stored offset already subtracted.
 *
 * This is intentionally a generic offset operation.
 */
void hx711_set_offset(hx711_t *dev, int samples);

/**
 * @brief Liest die Spannung in Mikrovolt [µV].
 * @details Interner Ablauf:
 * 1. @ref hx711_read (Rohwert)
 * 2. Division durch Gain-Faktor (z.B. 25.6)
 * 
 * @param dev Treiber-Handle.
 * @return double Spannung in µV.
 */
void hx711_read_uv(hx711_t *dev);
void hx711_read_avg_uv(hx711_t *dev, uint8_t samples);
void hx711_read_gram(hx711_t *dev, int samples);
void  hx711_calibrate(hx711_t *dev, int32_t weight_gold, int samples);

#endif // _HX711_H_
