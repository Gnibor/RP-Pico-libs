/**
 * @file mpu_reg_map.h
 * @author Robin Gerhartz (Gnibor)
 * @brief Register map definitions for the MPU IMU sensor.
 *
 * @details
 * MIT License
 *
 * Copyright (c) 2026 (Gnibor) Robin Gerhartz
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file contains the register addresses and bitfield definitions 
 * for the MPU series.
 *
 * @project MPU Driver for Raspberry Pi Pico
 * @license MIT License
 * @see https://github.com/Gnibor/MPU-Driver-Raspberry-Pi-Pico
 */
#ifndef _MPU_REG_MAP_H_
#define _MPU_REG_MAP_H_

/* --- Self Test Registers --- */
#define MPU_REG_SELF_TEST_X        0x0D /**< Self-test response for X-axis */
#define MPU_XG_TEST                (0x1F) /**< Gyro X-axis self-test exponent mask */
#define MPU_XA_TEST                (0xE0) /**< Accel X-axis self-test exponent mask */

#define MPU_REG_SELF_TEST_Y        0x0E /**< Self-test response for Y-axis */
#define MPU_YG_TEST                (0x1F) /**< Gyro Y-axis self-test exponent mask */
#define MPU_YA_TEST                (0xE0) /**< Accel Y-axis self-test exponent mask */

#define MPU_REG_SELF_TEST_Z        0x0F /**< Self-test response for Z-axis */
#define MPU_ZG_TEST                (0x1F) /**< Gyro Z-axis self-test exponent mask */
#define MPU_ZA_TEST                (0xE0) /**< Accel Z-axis self-test exponent mask */

#define MPU_REG_SELF_TEST_A        0x10 /**< Self-test response for Accelerometer */
#define MPU_ZA_TEST_A              (3 << 0) /**< Accel Z-axis self-test 2-bit field */
#define MPU_YA_TEST_A              (3 << 2) /**< Accel Y-axis self-test 2-bit field */
#define MPU_XA_TEST_A              (3 << 4) /**< Accel X-axis self-test 2-bit field */

/* --- Offset Registers (Factory Trim / MPU-6050) --- */
#define MPU_REG_XA_OFFS_H          0x06 /**< Accel X-axis offset High byte (MPU-6050) */
#define MPU_REG_XA_OFFS_L          0x07 /**< Accel X-axis offset Low byte (MPU-6050) */
#define MPU_REG_YA_OFFS_H          0x08 /**< Y-axis accel offset High byte (MPU-6050) */
#define MPU_REG_YA_OFFS_L          0x09 /**< Y-axis accel offset Low byte (MPU-6050) */
#define MPU_REG_ZA_OFFS_H          0x0A /**< Z-axis accel offset High byte (MPU-6050) */
#define MPU_REG_ZA_OFFS_L          0x0B /**< Z-axis accel offset Low byte (MPU-6050) */

#define MPU_REG_XG_OFFS_H          0x13 /**< Gyro X-axis offset High byte */
#define MPU_REG_XG_OFFS_L          0x14 /**< Gyro X-axis offset Low byte */
#define MPU_REG_YG_OFFS_H          0x15 /**< Gyro Y-axis offset High byte */
#define MPU_REG_YG_OFFS_L          0x16 /**< Gyro Y-axis offset Low byte */
#define MPU_REG_ZG_OFFS_H          0x17 /**< Gyro Z-axis offset High byte */
#define MPU_REG_ZG_OFFS_L          0x18 /**< Gyro Z-axis offset Low byte */

/* --- Configuration Registers --- */
#define MPU_REG_SMPLRT_DIV         0x19 /**< Sample rate divider (Divides Gyro output) */

/** @brief Sample rate divider presets. Formula: Rate = Gyro_Rate / (1 + div). */
typedef enum {
    MPU_SMPLRT_8KHZ  = (0 << 0),    /**< 8kHz output (DLPF must be disabled or set to 7). */
    MPU_SMPLRT_1KHZ  = (0x7 << 0),  /**< 1kHz output rate. */
    MPU_SMPLRT_500HZ = (0xE << 0),  /**< 500Hz output rate. */
    MPU_SMPLRT_200HZ = (0x27 << 0), /**< 200Hz output rate. */
    MPU_SMPLRT_100HZ = (0x5C << 0)  /**< 100Hz output rate. */
} mpu_smplrt_div_t;

#define MPU_REG_CONFIG             0x1A /**< General configuration (DLPF & Sync) */

/** @brief Digital Low Pass Filter (DLPF) bandwidth configurations. */
typedef enum {
    MPU_DLPF_CFG_260HZ  = (0 << 0), /**< 260Hz bandwidth, 0.6ms delay. */
    MPU_DLPF_CFG_184HZ  = (1 << 0), /**< 184Hz bandwidth, 2.0ms delay. */
    MPU_DLPF_CFG_94HZ   = (2 << 0), /**< 94Hz bandwidth, 3.0ms delay. */
    MPU_DLPF_CFG_44HZ   = (3 << 0), /**< 44Hz bandwidth, 4.9ms delay. */
    MPU_DLPF_CFG_21HZ   = (4 << 0), /**< 21Hz bandwidth, 8.5ms delay. */
    MPU_DLPF_CFG_10HZ   = (5 << 0), /**< 10Hz bandwidth, 13.8ms delay. */
    MPU_DLPF_CFG_5HZ    = (6 << 0), /**< 5Hz bandwidth, 19.0ms delay. */
    MPU_DLPF_CFG_3600HZ = (7 << 0)  /**< No filter (8kHz gyro, 1kHz accel). */
} mpu_dlpf_cfg_t;

/** @brief External Frame Sync (FSYNC) configuration for @ref mpu_fsync_config. */
typedef enum {
    MPU_EXT_SYNC_DISABLED = (0 << 3), /**< FSYNC pin function disabled. */
    MPU_EXT_SYNC_TEMP_OUT = (1 << 3), /**< FSYNC bit stored in TEMP_OUT_L[0]. */
    MPU_EXT_SYNC_XG       = (2 << 3), /**< FSYNC bit stored in GYRO_XOUT_L[0]. */
    MPU_EXT_SYNC_YG       = (3 << 3), /**< FSYNC bit stored in GYRO_YOUT_L[0]. */
    MPU_EXT_SYNC_ZG       = (4 << 3), /**< FSYNC bit stored in GYRO_ZOUT_L[0]. */
    MPU_EXT_SYNC_ACCEL_X  = (5 << 3), /**< FSYNC bit stored in ACCEL_XOUT_L[0]. */
    MPU_EXT_SYNC_ACCEL_Y  = (6 << 3), /**< FSYNC bit stored in ACCEL_YOUT_L[0]. */
    MPU_EXT_SYNC_ACCEL_Z  = (7 << 3)  /**< FSYNC bit stored in ACCEL_ZOUT_L[0]. */
} mpu_ext_sync_set_t;

#define MPU_REG_GYRO_CONFIG        0x1B /**< Gyroscope configuration (Full scale range) */

/** @brief Gyroscope full scale range (FSR) settings. */
typedef enum {
    MPU_FSR_250DPS  = (0 << 3), /**< +/- 250 deg/s range. */
    MPU_FSR_500DPS  = (1 << 3), /**< +/- 500 deg/s range. */
    MPU_FSR_1000DPS = (2 << 3), /**< +/- 1000 deg/s range. */
    MPU_FSR_2000DPS = (3 << 3)  /**< +/- 2000 deg/s range. */
} mpu_fsr_t;

#define MPU_REG_ACCEL_CONFIG       0x1C /**< Accelerometer configuration (FSR & HPF) */

/**
 * @brief Accelerometer High Pass Filter (AHPF) settings (Register 0x1C).
 * The filter removes DC offset (gravity) from data for motion detection.
 */
typedef enum {
    MPU_AHPF_RESET  = (0 << 0), /**< Reset high pass filter. */
    MPU_AHPF_5HZ    = (1 << 0), /**< Filter cutoff at 5Hz. */
    MPU_AHPF_2_5HZ  = (2 << 0), /**< Filter cutoff at 2.5Hz. */
    MPU_AHPF_1_25HZ = (3 << 0), /**< Filter cutoff at 1.25Hz. */
    MPU_AHPF_0_63HZ = (4 << 0), /**< Filter cutoff at 0.63Hz. */
    MPU_AHPF_HOLD   = (7 << 0)  /**< Freeze filter at current value. */
} mpu_ahpf_t;

/** @brief Accelerometer full scale range (AFSR) settings. */
typedef enum {
    MPU_AFSR_2G  = (0 << 3), /**< +/- 2g range. */
    MPU_AFSR_4G  = (1 << 3), /**< +/- 4g range. */
    MPU_AFSR_8G  = (2 << 3), /**< +/- 8g range. */
    MPU_AFSR_16G = (3 << 3)  /**< +/- 16g range. */
} mpu_afsr_t;

/* --- Threshold & Duration Registers --- */
#define MPU_ZA_ST                  (1 << 5) /**< Accel Z self-test trigger bit */
#define MPU_YA_ST                  (1 << 6) /**< Accel Y self-test trigger bit */
#define MPU_XA_ST                  (1 << 7) /**< Accel X self-test trigger bit */
#define MPU_REG_FF_THR             0x1D /**< Free-fall threshold */
#define MPU_REG_FF_DUR             0x1E /**< Free-fall duration */

#define MPU_REG_MOT_THR            0x1F /**< Motion detection threshold */
#define MPU_REG_MOT_DUR            0x20 /**< Motion detection duration */
#define MPU_REG_ZRMOT_THR          0x21 /**< Zero-motion detection threshold */
#define MPU_REG_ZRMOT_DUR          0x22 /**< Zero-motion detection duration */

/* --- I2C Master Control --- */
#define MPU_REG_I2C_MST_CTRL       0x24 /**< I2C Master clock and logic control */
#define MPU_I2C_MST_CLK_DIV_23     (0 << 0) /**< 348kHz (Clock 23) */
#define MPU_I2C_MST_CLK_DIV_24     (1 << 0) /**< 333kHz (Clock 24) */
#define MPU_I2C_MST_CLK_DIV_25     (2 << 0) /**< 320kHz (Clock 25) */
#define MPU_I2C_MST_CLK_DIV_26     (3 << 0) /**< 308kHz (Clock 26) */
#define MPU_I2C_MST_CLK_DIV_27     (4 << 0) /**< 296kHz (Clock 27) */
#define MPU_I2C_MST_CLK_DIV_28     (5 << 0) /**< 286kHz (Clock 28) */
#define MPU_I2C_MST_CLK_DIV_29     (6 << 0) /**< 276kHz (Clock 29) */
#define MPU_I2C_MST_CLK_DIV_30     (7 << 0) /**< 267kHz (Clock 30) */
#define MPU_I2C_MST_CLK_DIV_31     (8 << 0) /**< 258kHz (Clock 31) */
#define MPU_I2C_MST_CLK_DIV_16     (9 << 0) /**< 500kHz (Clock 16) */
#define MPU_I2C_MST_CLK_DIV_17     (10 << 0) /**< 471kHz (Clock 17) */
#define MPU_I2C_MST_CLK_DIV_18     (11 << 0) /**< 444kHz (Clock 18) */
#define MPU_I2C_MST_CLK_DIV_19     (12 << 0) /**< 421kHz (Clock 19) */
#define MPU_I2C_MST_CLK_DIV_20     (13 << 0) /**< 400kHz (Clock 20) */
#define MPU_I2C_MST_CLK_DIV_21     (14 << 0) /**< 381kHz (Clock 21) */
#define MPU_I2C_MST_CLK_DIV_22     (15 << 0) /**< 364kHz (Clock 22) */
#define MPU_I2C_MST_P_NSR          (1 << 4) /**< Stop/Start sequence between slave reads */
#define MPU_SLV_3_FIFO_EN          (1 << 5) /**< Slave 3 FIFO write enable */
#define MPU_WAIT_FOR_ES            (1 << 6) /**< Wait for external sensor data before interrupt */
#define MPU_MULTI_MST_EN           (1 << 7) /**< Enable multi-master mode */

/* --- I2C Slave 0 Control --- */
#define MPU_REG_I2C_SLV0_ADDR      0x25 /**< Slave 0 I2C address */
#define MPU_I2C_SLV0_ADDR_MSK      (0x7F)   /**< Slave 0 address mask */
#define MPU_I2C_SLV0_RW            (1 << 7) /**< Read/Write flag for Slave 0 */

#define MPU_REG_I2C_SLV0_REG       0x26 /**< Slave 0 register address */
#define MPU_REG_I2C_SLV0_CTRL      0x27 /**< Slave 0 control bits */
#define MPU_I2C_SLV0_LEN           (7 << 0) /**< Bytes to read from Slave 0 */
#define MPU_I2C_SLV0_GRP           (1 << 4) /**< External sensor word grouping */
#define MPU_I2C_SLV0_REG_DIS       (1 << 5) /**< Disable register access for this slave */
#define MPU_I2C_SLV0_BYTE_SW       (1 << 6) /**< Swap bytes (Little/Big Endian) */
#define MPU_I2C_SLV0_EN            (1 << 7) /**< Enable Slave 0 operations */

/* --- Slave 1 to 3 Registers --- */
#define MPU_REG_I2C_SLV1_ADDR      0x28 /**< Slave 1 address */
#define MPU_I2C_SLV1_RW            (1 << 7) /**< Slave 1 R/W flag */
#define MPU_REG_I2C_SLV1_REG       0x29 /**< Slave 1 register */
#define MPU_REG_I2C_SLV1_CTRL      0x2A /**< Slave 1 control */
#define MPU_I2C_SLV1_EN            (1 << 7) /**< Enable Slave 1 */

#define MPU_REG_I2C_SLV2_ADDR      0x2B /**< Slave 2 address */
#define MPU_I2C_SLV2_RW            (1 << 7) /**< Slave 2 R/W flag */
#define MPU_REG_I2C_SLV2_REG       0x2C /**< Slave 2 register */
#define MPU_REG_I2C_SLV2_CTRL      0x2D /**< Slave 2 control */
#define MPU_I2C_SLV2_EN            (1 << 7) /**< Enable Slave 2 */

#define MPU_REG_I2C_SLV3_ADDR      0x2E /**< Slave 3 address */
#define MPU_I2C_SLV3_RW            (1 << 7) /**< Slave 3 R/W flag */
#define MPU_REG_I2C_SLV3_REG       0x2F /**< Slave 3 register */
#define MPU_REG_I2C_SLV3_CTRL      0x30 /**< Slave 3 control */
#define MPU_I2C_SLV3_EN            (1 << 7) /**< Enable Slave 3 */

/* --- I2C Slave 4 (Indirect Master access) --- */
#define MPU_REG_I2C_SLV4_ADDR      0x31 /**< Slave 4 I2C address */
#define MPU_REG_I2C_SLV4_REG       0x32 /**< Slave 4 register address */
#define MPU_REG_I2C_SLV4_DO        0x33 /**< Slave 4 Data Output for writing */
#define MPU_REG_I2C_SLV4_CTRL      0x34 /**< Slave 4 control */
#define MPU_I2C_MST_DLY            (0xF << 0) /**< Master sample rate delay */
#define MPU_I2C_SLV4_INT_EN        (1 << 6) /**< Enable interrupt for Slave 4 */
#define MPU_I2C_SLV4_EN            (1 << 7) /**< Enable Slave 4 operation */
#define MPU_REG_I2C_SLV4_DI        0x35 /**< Slave 4 Data Input for reading */

/* --- Status Registers --- */
#define MPU_REG_I2C_MST_STATUS     0x36 /**< I2C Master status register */
#define MPU_I2C_SLV0_NACK          (1 << 0) /**< Slave 0 NACK received */
#define MPU_I2C_SLV1_NACK          (1 << 1) /**< Slave 1 NACK received */
#define MPU_I2C_SLV2_NACK          (1 << 2) /**< Slave 2 NACK received */
#define MPU_I2C_SLV3_NACK          (1 << 3) /**< Slave 3 NACK received */
#define MPU_I2C_SLV4_NACK          (1 << 4) /**< Slave 4 NACK received */
#define MPU_I2C_LOST_ARB           (1 << 5) /**< Lost arbitration on I2C bus */
#define MPU_I2C_SLV4_DONE          (1 << 6) /**< Slave 4 transfer complete */
#define MPU_PASS_THROUGH           (1 << 7) /**< FSYNC pin logic status */

/* --- Interrupt Configuration --- */
#define MPU_REG_INT_PIN_CFG        0x37 /**< Interrupt pin and Bypass configuration */

/** @brief Configuration flags for INT/FSYNC pin behavior (Register 0x37). */
typedef enum {
    MPU_I2C_BYPASS_EN   = (1 << 1), /**< Direct I2C access for secondary sensors on Aux bus. */
    MPU_FSYNC_INT_EN    = (1 << 2), /**< Enable FSYNC pin to trigger an interrupt. */
    MPU_FSYNC_INT_LEVEL = (1 << 3), /**< FSYNC logic level (0=Active High, 1=Active Low). */
    MPU_INT_RD_CLEAR    = (1 << 4), /**< Status bits cleared on any read operation. */
    MPU_LATCH_INT_EN    = (1 << 5), /**< Hold INT pin until status is read. */
    MPU_INT_OPEN_DRAIN  = (1 << 6), /**< Set INT pin to open drain (default: push-pull). */
    MPU_INT_LEVEL_LOW   = (1 << 7), /**< Set INT pin active level to Low (default: High). */
    MPU_INT_PIN_CFG_ALL = 0xFE      /**< Mask for all pin configuration bits. */
} mpu_int_pin_cfg_t;

#define MPU_REG_INT_ENABLE         0x38 /**< Interrupt enable register */

/** @brief Available interrupt sources for MPU_INT_ENABLE (Register 0x38). */
typedef enum {
    MPU_DATA_RDY_EN    = (1 << 0), /**< Trigger interrupt on new data sample. */
    MPU_I2C_MST_INT_EN = (1 << 3), /**< Trigger on I2C Master transaction finish. */
    MPU_FIFO_OFLOW_EN  = (1 << 4), /**< Trigger on FIFO buffer overflow. */
    MPU_INT_MOTION_EN  = (1 << 6), /**< Trigger on motion detection. */
    MPU_INT_ENABLE_ALL = 0x59      /**< Mask for common interrupt enable bits. */
} mpu_int_enable_t;

#define MPU_REG_DMP_INT_STATUS     0x39 /**< DMP (Digital Motion Processor) status */
#define MPU_REG_INT_STATUS         0x3A /**< General interrupt status register */
#define MPU_DATA_RDY_INT           (1 << 0) /**< New data is ready */
#define MPU_I2C_MST_INT            (1 << 3) /**< Master I2C interrupt event */
#define MPU_FIFO_OFLOW_INT         (1 << 4) /**< FIFO overflow occurred */
#define MPU_MOTION_INT             (1 << 6) /**< Motion detection triggered */

/* --- Sensor Data Output (Read-only) --- */
#define MPU_REG_ACCEL_XOUT_H       0x3B /**< Accel X-axis High byte */
#define MPU_REG_ACCEL_XOUT_L       0x3C /**< Accel X-axis Low byte */
#define MPU_REG_ACCEL_YOUT_H       0x3D /**< Accel Y-axis High byte */
#define MPU_REG_ACCEL_YOUT_L       0x3E /**< Accel Y-axis Low byte */
#define MPU_REG_ACCEL_ZOUT_H       0x3F /**< Accel Z-axis High byte */
#define MPU_REG_ACCEL_ZOUT_L       0x40 /**< Accel Z-axis Low byte */

#define MPU_REG_TEMP_OUT_H         0x41 /**< Temperature sensor High byte */
#define MPU_REG_TEMP_OUT_L         0x42 /**< Temperature sensor Low byte */

#define MPU_REG_GYRO_XOUT_H        0x43 /**< Gyro X-axis High byte */
#define MPU_REG_GYRO_XOUT_L        0x44 /**< Gyro X-axis Low byte */
#define MPU_REG_GYRO_YOUT_H        0x45 /**< Gyro Y-axis High byte */
#define MPU_REG_GYRO_YOUT_L        0x46 /**< Gyro Y-axis Low byte */
#define MPU_REG_GYRO_ZOUT_H        0x47 /**< Gyro Z-axis High byte */
#define MPU_REG_GYRO_ZOUT_L        0x48 /**< Gyro Z-axis Low byte */

/* --- External Sensor Data (Slave read results) --- */
#define MPU_REG_EXT_SENS_DATA_00   0x49 /**< Start of external sensor data block */
#define MPU_REG_EXT_SENS_DATA_23   0x60 /**< End of external sensor data block */

/* --- Slave Data Output (Writing to slaves) --- */
#define MPU_REG_I2C_SLV0_DO        0x63 /**< Data output for Slave 0 */
#define MPU_REG_I2C_SLV1_DO        0x64 /**< Data output for Slave 1 */
#define MPU_REG_I2C_SLV2_DO        0x65 /**< Data output for Slave 2 */
#define MPU_REG_I2C_SLV3_DO        0x66 /**< Data output for Slave 3 */

/* --- I2C Master Delay Control --- */
#define MPU_REG_I2C_MST_DELAY_CTRL 0x67 /**< External sensor sample rate delay control */
#define MPU_I2C_SLV0_DLY_EN        (1 << 0) /**< Enable sample rate delay for Slave 0 */
#define MPU_I2C_SLV1_DLY_EN        (1 << 1) /**< Enable sample rate delay for Slave 1 */
#define MPU_I2C_SLV2_DLY_EN        (1 << 2) /**< Enable sample rate delay for Slave 2 */
#define MPU_I2C_SLV3_DLY_EN        (1 << 3) /**< Enable sample rate delay for Slave 3 */
#define MPU_I2C_SLV4_DLY_EN        (1 << 4) /**< Enable sample rate delay for Slave 4 */
#define MPU_DELAY_ES_SHADOW        (1 << 7) /**< Delay shadow register updates */

/* --- Path Resets --- */
#define MPU_REG_SIGNAL_PATH_RESET  0x68 /**< Reset sensor signal paths */
#define MPU_TEMP_RESET             (1 << 0) /**< Reset temperature signal path */
#define MPU_ACCEL_RESET            (1 << 1) /**< Reset accelerometer signal path */
#define MPU_GYRO_RESET             (1 << 2) /**< Reset gyroscope signal path */

/* --- Motion Detection Control --- */
#define MPU_REG_MOT_DETECT_CTRL    0x69 /**< Motion detection counter and delay settings */

/** @brief Motion detection hardware counter decrement settings. */
typedef enum {
    MOT_COUNT_RESET   = 0x00, /**< Reset counter to 0 immediately. */
    MOT_COUNT_DEC_1   = 0x01, /**< Decrement counter by 1. */
    MOT_COUNT_DEC_2   = 0x02, /**< Decrement counter by 2. */
    MOT_COUNT_DEC_4   = 0x03  /**< Decrement counter by 4. */
} mpu_mot_count_t;

/** @brief Power-on delay settings for motion detection (Register 0x69). */
typedef enum {
    MOT_DELAY_0MS     = (0 << 4), /**< 0ms delay before detection starts. */
    MOT_DELAY_1MS     = (1 << 4), /**< 1ms power-on delay. */
    MOT_DELAY_2MS     = (2 << 4), /**< 2ms power-on delay. */
    MOT_DELAY_3MS     = (3 << 4)  /**< 3ms power-on delay. */
} mpu_mot_delay_t;

/* --- User Control Register --- */
#define MPU_REG_USER_CTRL          0x6A /**< Main user-facing control register */
#define MPU_SIG_COND_RESET         (1 << 0) /**< Reset all sensor signal conditions */
#define MPU_I2C_MST_RESET          (1 << 1) /**< Reset I2C Master logic */
#define MPU_FIFO_RESET             (1 << 2) /**< Reset FIFO buffer content */
#define MPU_I2C_IF_DIS             (1 << 4) /**< Disable I2C and use SPI mode */
#define MPU_I2C_MST_EN             (1 << 5) /**< Enable I2C Master mode */
#define MPU_FIFO_EN_BIT            (1 << 6) /**< Enable FIFO buffer operations */

/* --- Power Management 1 --- */
#define MPU_REG_PWR_MGMT_1         0x6B /**< Main power and clock source control */

/** @brief Clock source selection for timing and sampling. */
typedef enum {
    MPU_CLK_INTERNAL = (0 << 0), /**< Internal 8MHz oscillator. */
    MPU_CLK_XGYRO    = (1 << 0), /**< PLL with X-Gyro reference (Recommended). */
    MPU_CLK_YGYRO    = (2 << 0), /**< PLL with Y-Gyro reference. */
    MPU_CLK_ZGYRO    = (3 << 0), /**< PLL with Z-Gyro reference. */
    MPU_CLK_EXT32KHZ = (4 << 0), /**< External 32.768kHz clock. */
    MPU_CLK_EXT19MHZ = (5 << 0), /**< External 19.2MHz clock. */
    MPU_CLK_STOP     = (7 << 0)  /**< Stop clock and reset timing. */
} mpu_clk_sel_t;

#define MPU_TEMP_DIS               (1 << 3) /**< Disable temperature sensor */
#define MPU_CYCLE                  (1 << 5) /**< Enable low-power cycle mode */
#define MPU_SLEEP                  (1 << 6) /**< Put device into sleep mode */
#define MPU_DEVICE_RESET           (1 << 7) /**< Trigger full chip reset */

/* --- Power Management 2 --- */
#define MPU_REG_PWR_MGMT_2         0x6C /**< Standby and wake-up control register */

/** @brief Standby settings for individual sensor axes (Register 0x6C). */
typedef enum {
    MPU_STBY_ZG    = (1 << 0), /**< Put Z-Gyro in standby. */
    MPU_STBY_YG    = (1 << 1), /**< Put Y-Gyro in standby. */
    MPU_STBY_XG    = (1 << 2), /**< Put X-Gyro in standby. */
    MPU_STBY_GYRO  = (7 << 0), /**< Put all Gyro axes in standby. */
    MPU_STBY_ACCEL = (7 << 3), /**< Put all Accel axes in standby. */
    MPU_STBY_ALL   = 0x3F      /**< Put all 6 sensor axes in standby. */
} mpu_stby_t;

/**
 * @brief Wake-up frequencies for Low Power Accelerometer mode.
 * 
 * @details 
 * To distinguish between hardware generations, we use a 16-bit marker:
 * - 0x000: MPU-6050 specific (Value for PWR_MGMT_2 [7:6])
 * - 0x200: MPU-6500/9250 specific (Value for LP_ACCEL_ODR [3:0])
 */
typedef enum {
    /* Legacy MPU (60X0) - Marker 0x0000 | Bits for PWR_MGMT_2 [7:6] */
    MPU_LP_WAKE_1_25HZ      = (0x000 | (0 << 6)), /**< 1.25Hz (Legacy) */
    MPU_LP_WAKE_5HZ         = (0x000 | (1 << 6)), /**< 5Hz (Legacy) */
    MPU_LP_WAKE_20HZ        = (0x000 | (2 << 6)), /**< 20Hz (Legacy) */
    MPU_LP_WAKE_40HZ        = (0x000 | (3 << 6)), /**< 40Hz (Legacy) */

    /* Modern MPU (6500/925X) - Marker 0x0100 | Bits for LP_ACCEL_ODR [3:0] */
    MPU6500_LP_WAKE_0_24HZ  = (0x100 | 0x00), /**< 0.24Hz (Modern) */
    MPU6500_LP_WAKE_0_49HZ  = (0x100 | 0x01), /**< 0.49Hz (Modern) */
    MPU6500_LP_WAKE_0_98HZ  = (0x100 | 0x02), /**< 0.98Hz (Modern) */
    MPU6500_LP_WAKE_1_95HZ  = (0x100 | 0x03), /**< 1.95Hz (Modern) */
    MPU6500_LP_WAKE_3_91HZ  = (0x100 | 0x04), /**< 3.91Hz (Modern) */
    MPU6500_LP_WAKE_7_81HZ  = (0x100 | 0x05), /**< 7.81Hz (Modern) */
    MPU6500_LP_WAKE_15_63HZ = (0x100 | 0x06), /**< 15.63Hz (Modern) */
    MPU6500_LP_WAKE_31_25HZ = (0x100 | 0x07), /**< 31.25Hz (Modern) */
    MPU6500_LP_WAKE_62_5HZ  = (0x100 | 0x08), /**< 62.5Hz (Modern) */
    MPU6500_LP_WAKE_125HZ   = (0x100 | 0x09), /**< 125Hz (Modern) */
    MPU6500_LP_WAKE_250HZ   = (0x100 | 0x0A), /**< 250Hz (Modern) */
    MPU6500_LP_WAKE_500HZ   = (0x100 | 0x0B)  /**< 500Hz (Modern) */
} mpu_lp_wake_t;

#define MPU6500_BIT_DIS_LP_WAIT    (1 << 7) /**< Disable low-power wait mode (MPU-6500 only) */

/* --- FIFO Buffer Status & Access --- */
#define MPU_REG_FIFO_COUNTH        0x72 /**< FIFO byte count (High byte) */
#define MPU_REG_FIFO_COUNTL        0x73 /**< FIFO byte count (Low byte) */
#define MPU_REG_FIFO_R_W           0x74 /**< FIFO read/write portal register */

/* --- Identification --- */
#define MPU_REG_WHO_AM_I           0x75 /**< Device identification register */
#define MPU60X0_WHO_AM_I           0x68 /**< ID for MPU-6000/6050 */
#define MPU6500_WHO_AM_I           0x70 /**< ID for MPU-6500 */
#define MPU9250_WHO_AM_I           0x71 /**< ID for MPU-9250 */
#define MPU9255_WHO_AM_I           0x73 /**< ID for MPU-9255 */

/* --- MPU-6500 / 9250 / 9255 Specific Accel Offsets --- */
/** 
 * @note These registers differ from MPU-6050 (which uses 0x06-0x0B).
 * Bit 0 of each pair is often reserved or used for specific trim.
 */
#define MPU6500_REG_XA_OFFS_H      0x77  /**< X-axis accel offset high byte */
#define MPU6500_REG_XA_OFFS_L      0x78  /**< X-axis accel offset low byte */
#define MPU6500_REG_YA_OFFS_H      0x7A  /**< Y-axis accel offset high byte */
#define MPU6500_REG_YA_OFFS_L      0x7B  /**< Y-axis accel offset low byte */
#define MPU6500_REG_ZA_OFFS_H      0x7D  /**< Z-axis accel offset high byte */
#define MPU6500_REG_ZA_OFFS_L      0x7E  /**< Z-axis accel offset low byte */

/* --- Low Power Accelerometer ODR Control (MPU-6500 / 9250 only) --- */
#define MPU6500_REG_LP_ACCEL_ODR   0x1E  /**< Low-power accel output data rate register */

#endif /* _MPU_REG_MAP_H_ */
