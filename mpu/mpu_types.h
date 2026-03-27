#ifndef MPU_TYPES_H
#define MPU_TYPES_H

#include <stdint.h>

/**
 * @brief 3-axis vector (raw sensor values)
 */
typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} mpu_vec3_raw_t;


/**
 * @brief MPU raw sensor frame (14 bytes, burst read)
 *
 * Layout (big-endian as provided by the MPU):
 *
 *   ACCEL_X  [0..1]
 *   ACCEL_Y  [2..3]
 *   ACCEL_Z  [4..5]
 *   TEMP     [6..7]
 *   GYRO_X   [8..9]
 *   GYRO_Y   [10..11]
 *   GYRO_Z   [12..13]
 *
 * Provides:
 * - raw byte buffer for I2C transfers
 * - structured access for convenient field usage
 *
 * Important:
 * - Data is big-endian (MPU output)
 * - RP2040 is little-endian
 * - Values must be byte-swapped before direct use
 *
 * Example conversion:
 *   int16_t v = (raw << 8) | (raw >> 8);
 *
 * Notes:
 * - struct is packed to guarantee exact 14-byte layout
 * - safe for direct overlay with raw buffer
 */
typedef union {

	uint8_t raw[14];

	struct __attribute__((packed)){
		mpu_vec3_raw_t accel;
		int16_t        temp;
		mpu_vec3_raw_t gyro;
	};

} mpu_frame_t;

#endif
