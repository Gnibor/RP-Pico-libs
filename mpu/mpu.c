/**
 * @file mpu.c
 * @author Robin Gerhartz (Gnibor)
 * @brief Low-level driver implementation for the MPU IMU sensor.
 *
 * @details
 * This driver for the Raspberry Pi Pico implements:
 * - I²C communication
 * - Register-level configuration
 * - Sensor data acquisition
 * - Automatic scaling (raw -> physical units)
 * - Gyroscope zero-point calibration
 * - Power management features
 * - Timing and cycle management
 * - Interrupt configuration and status monitoring
 *
 * The driver is written in a lightweight embedded style, optimized for 
 * performance and memory efficiency on the Raspberry Pi Pico.
 *
 * @project MPU Driver for Raspberry Pi Pico
 * @license MIT License (see LICENSE file in root)
 * @copyright Copyright (c) 2026 (Gnibor) Robin Gerhartz
 * @see https://github.com/Gnibor/MPU-Driver-Raspberry-Pi-Pico
 */
#include "hardware/gpio.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mpu_reg_map.h"
#include "mpu.h"
#include "i2c.h"
#include "byte_cache.h"
#include "byte_ops.h"
#include "mpu_types.h"

// ===============
// === Structs ===
// ===============
typedef union {
    uint8_t raw[14];

    struct {
        uint8_t head;
        uint8_t data[13];
    } tx;

    struct {
        byte_u8_t head;
        byte_u8_t data[13];
    } bits;

    uint16_t u16[7];
} byte_cache14_t;

// Configuration
typedef struct mpu_conf{
	mpu_addr_t addr; // Device Address
	uint8_t who_am_i;
	struct{ int32_t x, y, z; } offset_gyro, offset_accel;
	struct{ float accel, gyro; } fsr_div;
} mpu_conf_t;

// ================
// === Typedefs ===
// ================
typedef uint8_t mpu_cache_t;

// ===========================
// === Function prototypes ===
// ===========================
bool _mpu_write_reg(uint8_t *data, uint8_t how_many, bool nostop);
bool _mpu_read_reg(uint8_t reg, uint8_t *out, uint8_t how_many);

// helper function
static inline uint16_t be16_idx(const uint8_t *b, uint8_t i){
	return ((uint16_t)b[i] << 8) | b[i+1];
}

// ========================
// === Global Variables ===
// ========================
/** @brief Internal I2C data cache for burst reads and register manipulations. */
static byte_cache14_t gb_mpu = {0};

/** @brief Active device pointer used by all standalone functions.
 *  Set via @ref mpu_init or @ref mpu_use_struct. */
 mpu_s g_mpu = {0};
 mpu_conf_t g_mpu_conf = {0};

/** @brief Cache for the last I2C operation return value (byte count or error). */
static int gc_mpu_ret = 0;

static _i2c_hw_config g_i2c;

/**
 * @brief Initializes the MPU device struct and the Raspberry Pi Pico I2C hardware.
 *
 * Sets up I2C at 400kHz, configures GPIO pins (SDA, SCL, and optional INT),
 * performs a @ref mpu_who_am_i check, and wakes the device from its default sleep state.
 *
 * @param i2c_port Pointer to the Pico I2C instance (e.g., i2c0 or i2c1).
 * @param addr     I2C slave address from @ref mpu_addr_t.
 *
 * @return mpu_s   A populated device structure.
 * @note This function automatically sets @ref g_mpu to the newly created instance.
 * @warning Ensure @ref MPU_SDA_PIN and @ref MPU_SCL_PIN are defined in your config.
 */
mpu_s *mpu_init(i2c_hw_t *i2c_hw, mpu_addr_t addr){
int size = sizeof(g_mpu_conf);
	LOG_W("conf=%d, value=%d", size, sizeof(g_mpu));
	if(!i2c_hw){
		g_i2c.hw = i2c1_hw;
		LOG_W("i2c_hw = NULL, fallback=i2c1");
	}else g_i2c.hw = i2c_hw;

	if(!addr){
		g_mpu_conf.addr = MPU_ADDR_AD0_GND;
		LOG_W("invalid addr = 0x%02X, fallback=0x%02X", addr, MPU_ADDR_AD0_GND);
	}else g_mpu_conf.addr = addr;

	g_mpu_conf.fsr_div.accel = 16384.0f;
	g_mpu_conf.fsr_div.gyro = 131.0f;

	g_i2c.scl_pin = MPU_SCL_PIN;
	g_i2c.sda_pin = MPU_SDA_PIN;
	g_i2c.baudrate = 400000;
	g_i2c.timeout_us = 1000;
	if(!_i2c_is_initialized(&g_i2c)){
		_i2c_init(&g_i2c); // 400 kHz I2C
		LOG_I("I2C initialized baudrate=400000 sda=%d scl=%d", MPU_SDA_PIN, MPU_SCL_PIN);
	}else{
		LOG_W("I2C already initialized");
	}

#if MPU_USE_PULLUP
	gpio_pull_up(MPU_SDA_PIN);
	gpio_pull_up(MPU_SCL_PIN);
	LOG_D("I2C pull-up enabled");
#endif

#if MPU_INT_PIN
	// Configure optional interrupt pin
	gpio_init(MPU_INT_PIN);
	gpio_set_dir(MPU_INT_PIN, GPIO_IN);
	LOG_I("interrupt pin configured gpio=%d", MPU_INT_PIN);
#endif
#if MPU_INT_PULLUP
	gpio_pull_up(MPU_INT_PIN);
	LOG_D("interrupt pin pull-up enabled");
#endif

	if(!mpu_who_am_i()){
		LOG_E("mpu_who_am_i() failed");
		memset(&g_mpu, 0, sizeof(g_mpu));
		return &g_mpu;
	}

	if(!mpu_sleep(MPU_SLEEP_ALL_OFF)){
		LOG_E("mpu_sleep() failed sleep=0x%02X", MPU_SLEEP_ALL_OFF);
		memset(&g_mpu, 0, sizeof(g_mpu));
	}
	return &g_mpu;
}

/**
 * @brief Performs a raw I2C write operation to the MPU.
 *
 * @param data     Pointer to the data array (First byte must be the register address).
 * @param how_many Number of bytes to write (including the register address).
 * @param block    If true, the function will block until transmission is complete.
 *
 * @return true  If the number of bytes written matches @p how_many.
 * @return false If @ref g_mpu is not set or I2C communication failed.
 */
bool _mpu_write_reg(uint8_t *data, uint8_t how_many, bool nostop){
	gc_mpu_ret = _i2c_write_buffer(&g_i2c, g_mpu_conf.addr, data, how_many, nostop);
	if(gc_mpu_ret){
		LOG_D("register write ok reg=0x%02X len=%u", data[0], how_many);
	}else{
		LOG_E("I2C write failed reg=0x%02X len=%u ret=%d", data[0], how_many, gc_mpu_ret);
		return false;
	}

	return true;
}

/**
 * @brief Performs a raw I2C read operation from a specific MPU register.
 *
 * This function first writes the register address and then reads the requested
 * number of bytes back into the provided buffer.
 *
 * @param reg      The starting register address to read from.
 * @param out      Pointer to the buffer where the read data will be stored.
 * @param how_many Number of bytes to read.
 * @param block    If true, uses blocking I2C calls.
 *
 * @return true  If the number of bytes read matches @p how_many.
 * @return false If @ref g_mpu is not set, or I2C communication failed.
 */
bool _mpu_read_reg(uint8_t reg, uint8_t *out, uint8_t how_many){
	if(!_mpu_write_reg(&reg, 1, true)){
		LOG_E("register write failed reg=0x%02X", reg);
		return false;
	}
	gc_mpu_ret = _i2c_read_buffer(&g_i2c, g_mpu_conf.addr, out, how_many);
	if(gc_mpu_ret){
		LOG_D("register read ok reg=0x%02X len=%u", reg, how_many);
		return true;
	}else{
		LOG_E("I2C read failed reg=0x%02X len=%u ret=%d", reg, how_many, gc_mpu_ret);
		return false;
	}
}

/**
 * @brief Validates the connection by reading the MPU's unique ID.
 *
 * Checks the @c WHO_AM_I register against the expected @ref MPU_WHO_AM_I value.
 *
 * @return true  If the device responded and the ID is correct (0x68).
 * @return false If communication failed or the device ID is unknown.
 */
bool mpu_who_am_i(void){
	if(!_mpu_read_reg(MPU_REG_WHO_AM_I, &g_mpu_conf.who_am_i, 1)){
		LOG_E("_mpu_read_reg() failed");
		return false;
	}

	switch (g_mpu_conf.who_am_i) {
		case MPU60X0_WHO_AM_I:
		    LOG_I("device detected type=MPU60X0 who_am_i=0x%02X", g_mpu_conf.who_am_i);
		    return true;
		case MPU9250_WHO_AM_I:
		    LOG_I("device detected type=MPU9250 who_am_i=0x%02X", g_mpu_conf.who_am_i);
		    return true;
		case MPU9255_WHO_AM_I:
		    LOG_I("device detected type=MPU9255 who_am_i=0x%02X", g_mpu_conf.who_am_i);
		    return true;
		case MPU6500_WHO_AM_I:
		    LOG_I("device detected type=MPU6500 who_am_i=0x%02X", g_mpu_conf.who_am_i);
		    return true;
		default:
		    LOG_E("device not recognized who_am_i=0x%02X", g_mpu_conf.who_am_i);
		    return false;
	}
}

/**
 * @brief Puts the MPUs I²C in pass-through-mode
 */
bool mpu_bypass(bool active){
	if(!_mpu_read_reg(MPU_REG_INT_PIN_CFG, gb_mpu.tx.data, 1)){
		LOG_E("I2C read failed reg=0x%02X len=1", MPU_REG_INT_PIN_CFG);
		return false;
	}

	gb_mpu.tx.head = MPU_REG_INT_PIN_CFG;
	if(active){
		gb_mpu.tx.data[0] |= MPU_I2C_BYPASS_EN;
	}else{
		gb_mpu.tx.data[0] &= ~MPU_I2C_BYPASS_EN;
	}

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("I2C write failed reg=0x%02X value=0x%02X len=2 stop=false", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}
	LOG_I("bypass set active=%s", active ? "true" : "false");

	return true;
}

/**
 * @brief Performs a software reset on specific internal components or the entire device.
 *
 * This function handles both partial resets of signal paths (Accel, Gyro, Temp)
 * and full device reboots. It modifies @c SIGNAL_PATH_RESET, @c USER_CTRL,
 * and @c PWR_MGMT_1 depending on the provided bitmask.
 *
 * @param reset Bitmask defining which components to reset using @ref mpu_reset_t.
 *              Use @ref MPU_RESET_ALL for a full hardware-level reboot.
 *
 * @return true  If all I2C read/write operations and timing sequences succeeded.
 * @return false If @ref g_mpu is NULL or any I2C transaction failed.
 *
 * @note A full reset (@ref MPU_RESET_ALL) triggers a @c DEVICE_RESET and
 *       includes mandatory delays (up to 550ms) to allow the analog sensors
 *       and digital logic to stabilize.
 * @warning Performing a full reset will revert all sensor configurations
 *          (FSR, DLPF, etc.) to their power-on default states.
 */
bool mpu_reset(mpu_reset_t reset){
	// 1. Read current register values into global cache to preserve existing bits
	// We read 3 bytes starting from SIGNAL_PATH_RESET (likely covering USER_CTRL & PWR_MGMT_1)
	gb_mpu.tx.head = MPU_REG_SIGNAL_PATH_RESET;
	if (!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 3)){
		LOG_E("register read failed reg=0x%02X len=3", MPU_REG_SIGNAL_PATH_RESET);
		return false;
	}

	// 2. Modify SIGNAL_PATH_RESET bits (TEMP, ACCEL, GYRO)
	if (reset & MPU_RESET_TEMP)  gb_mpu.tx.data[0] |= MPU_TEMP_RESET;
	if (reset & MPU_RESET_ACCEL) gb_mpu.tx.data[0] |= MPU_ACCEL_RESET;
	if (reset & MPU_RESET_GYRO)  gb_mpu.tx.data[0] |= MPU_GYRO_RESET;

	// 3. Modify USER_CTRL bits (SIG_COND, I2C_MST, FIFO)
	if (reset & MPU_RESET_SIG_COND) gb_mpu.tx.data[1] |= MPU_SIG_COND_RESET;
	if (reset & MPU_RESET_I2C_MST)  gb_mpu.tx.data[1] |= MPU_I2C_MST_RESET;
	if (reset & MPU_RESET_FIFO)     gb_mpu.tx.data[1] |= MPU_FIFO_RESET;

	// 4. Modify PWR_MGMT_1 bits (DEVICE_RESET)
	if (reset & MPU_RESET_DEVICE) gb_mpu.tx.data[2] |= MPU_DEVICE_RESET;

	// 5. Execution Logic
	if (reset & MPU_RESET_ALL){
		LOG_I("full chip reset sequence started");

		// Trigger Main Device Reset via PWR_MGMT_1
		gb_mpu.tx.head = MPU_REG_PWR_MGMT_1;
		gb_mpu.tx.data[0] = gb_mpu.tx.data[2] | MPU_DEVICE_RESET; // Warning: index logic should match your register mapping
		if (!_mpu_write_reg(gb_mpu.raw, 2, false)) return false;
		sleep_ms(150); // Wait for internal reboot

		// Reset Signal Paths (Analog/Digital Filters)
		LOG_D("signal path reset started");
		gb_mpu.tx.head = MPU_REG_SIGNAL_PATH_RESET;
		gb_mpu.tx.data[0] |= (MPU_TEMP_RESET | MPU_ACCEL_RESET | MPU_GYRO_RESET);
		if (!_mpu_write_reg(gb_mpu.raw, 2, false)) return false;
		sleep_ms(200);

		// Reset User Control (FIFO, I2C Master, Logic)
		LOG_D("user control reset started");
		gb_mpu.tx.head = MPU_REG_USER_CTRL;
		gb_mpu.tx.data[0] = gb_mpu.tx.data[1] | (MPU_SIG_COND_RESET | MPU_I2C_MST_RESET | MPU_FIFO_RESET);
		if (!_mpu_write_reg(gb_mpu.raw, 2, false)) return false;
		sleep_ms(200);

		LOG_I("full reset complete");
	} else {
		// Partial reset of specific signal paths
		LOG_I("partial reset requested mask=0x%02X", reset);
		gb_mpu.tx.head = MPU_REG_SIGNAL_PATH_RESET;
		if (!_mpu_write_reg(gb_mpu.raw, 4, false)){
			LOG_E("partial reset write failed");
			return false;
		}
		sleep_ms(100);
	}

	return true;
}

/**
 * @brief Controls the sleep states of the device and the temperature sensor.
 *
 * @param sleep Combination of @ref mpu_sleep_t flags to enable/disable components.
 * @return true if communication succeeded, false otherwise.
 * @note This function manages bits in the @c PWR_MGMT_1 register.
 */
bool mpu_sleep(mpu_sleep_t sleep){
	gb_mpu.tx.head = MPU_REG_PWR_MGMT_1;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
		LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
		return false;
	}

	// Sleep Bit
	if(sleep & MPU_SLEEP_DEVICE_ON){
		gb_mpu.tx.data[0] |= MPU_SLEEP;
		LOG_I("device sleep sequence started");
	}else if(!(sleep & (MPU_SLEEP_DEVICE_ON << 1))){
		gb_mpu.tx.data[0] &= ~MPU_SLEEP;
		LOG_I("device wake-up sequence started");
	}

	// Temperature disable Bit
	if(sleep & MPU_SLEEP_TEMP_ON){
		gb_mpu.tx.data[0] |= MPU_TEMP_DIS;
		LOG_I("temp sleep sequence started");
	}else if(!(sleep & (MPU_SLEEP_TEMP_ON << 2))){
		gb_mpu.tx.data[0] &= ~MPU_TEMP_DIS;
		LOG_I("temp wake-up sequence started");
	}

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(5); // Activation pause

	LOG_I("sleep config applied");

	return true;
}

/**
 * @brief Places specific sensor axes into standby mode to save power.
 *
 * @param stby Bitmask of axes from @ref mpu_stby_t to put into standby.
 * @return true if @c PWR_MGMT_2 was updated successfully.
 */
bool mpu_stby(mpu_stby_t stby){
	gb_mpu.tx.head = MPU_REG_PWR_MGMT_2;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
		LOG_E("register read failed reg=0x%02X len=1", MPU_REG_PWR_MGMT_2);
		return false;
	}

	gb_mpu.tx.data[0] &= ~MPU_STBY_ALL;
	gb_mpu.tx.data[0] |= stby;

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(5);

	LOG_I("standby config applied reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);

	return true;
}

/**
 * @brief Selects the clock source for the MPU.
 *
 * @param clksel Clock source from @ref mpu_clk_sel_t.
 *               Using a Gyro PLL is recommended for better stability.
 * @return true if clock source was updated.
 */
bool mpu_clk_sel(mpu_clk_sel_t clksel){
	gb_mpu.tx.head = MPU_REG_PWR_MGMT_1;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
		LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
		return false;
	}

	gb_mpu.tx.data[0] &= ~MPU_CLK_STOP;
	gb_mpu.tx.data[0] |= clksel;

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(5);

	LOG_I("clock source applied reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);

	return true;
}

/**
 * @brief Configures the Digital Low Pass Filter (DLPF).
 *
 * The DLPF filters high-frequency noise from both accelerometer and gyroscope.
 * Lower frequencies result in cleaner data but higher latency.
 *
 * @param cfg Filter setting from @ref mpu_dlpf_cfg_t.
 * @return true if @c CONFIG register was updated.
 */
bool mpu_dlpf_cfg(mpu_dlpf_cfg_t cfg){
	gb_mpu.tx.head = MPU_REG_CONFIG;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
		LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
		return false;
	}

	gb_mpu.tx.data[0] &= ~MPU_DLPF_CFG_3600HZ;
	gb_mpu.tx.data[0] |= cfg;

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(5);

	LOG_I("dlpf config applied reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);

	return true;
}

/**
 * @brief Sets the Sample Rate Divider.
 *
 * The sample rate is determined by: Sample Rate = Internal_Sample_Rate / (1 + smplrt_div).
 *
 * @param smplrt_div Divider value (0-255).
 * @return true if @c SMPLRT_DIV register was updated.
 * @note The @c Internal_Sample_Rate is 1kHz unless DLPF is disabled (8kHz).
 */
bool mpu_smplrt_div(mpu_smplrt_div_t smplrt_div){
	gb_mpu.tx.head = MPU_REG_SMPLRT_DIV;
	gb_mpu.tx.data[0] = smplrt_div;

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(5);

	LOG_I("sample rate divider applied reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);

	return true;
}

/**
 * @brief Configures the Accelerometer High Pass Filter.
 *
 * Sets the cutoff frequency for the internal digital high pass filter.
 * This is used to eliminate constant acceleration (gravity) from the
 * motion detection logic.
 *
 * @param ahpf The desired filter setting from @ref mpu_ahpf_t.
 * @return true  If the configuration was written successfully.
 * @return false If I2C communication failed.
 *
 * @note After setting the filter, a 5ms delay is included to let the
 *       filter coefficients stabilize.
 */
bool mpu_ahpf(mpu_ahpf_t ahpf){
	gb_mpu.tx.head = MPU_REG_ACCEL_CONFIG;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
		LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
		return false;
	}

	gb_mpu.tx.data[0] &= ~MPU_AHPF_HOLD;
	gb_mpu.tx.data[0] |= ahpf;

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(5);

	LOG_I("ahpf config applied reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);

	return true;
}

/**
 * @brief Sets the MPU-6050 cycle mode for low-power operation.
 *
 * In cycle mode, the device sleeps and wakes up at a specific rate to take a
 * single accelerometer sample. Gyroscope axes are disabled to save energy.
 *
 * @param mode         Selects @ref MPU_CYCLE_ON, @ref MPU_CYCLE_LP, or @ref MPU_CYCLE_OFF.
 * @param wake_up_rate Frequency of the wake-up cycle (use @ref mpu_lp_wake_t).
 *
 * @return true  If the power management registers were updated successfully.
 * @return false If I2C communication failed.
 *
 * @note Low-power cycle mode (@ref MPU_CYCLE_LP) also disables the temperature
 *       sensor and puts all gyroscope axes into standby.
 */
bool mpu_cycle_mode(mpu_cycle_t mode, mpu_lp_wake_t wake_up_rate){
	// Read current PWR_MGMT_1 and PWR_MGMT_2
	gb_mpu.tx.head = MPU_REG_PWR_MGMT_1;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 2)){
		LOG_E("register read failed reg=0x%02X len=2", gb_mpu.tx.head);
		return false;
	}

	// Backup cached register values
	gb_mpu.tx.data[2] = gb_mpu.tx.data[0]; // backup PWR_MGMT_1
	gb_mpu.tx.data[3] = gb_mpu.tx.data[1]; // backup PWR_MGMT_2

	bool is_modern =
		(g_mpu_conf.who_am_i == MPU6500_WHO_AM_I) ||
		(g_mpu_conf.who_am_i == MPU9250_WHO_AM_I) ||
		(g_mpu_conf.who_am_i == MPU9255_WHO_AM_I);

	bool wake_is_modern = ((wake_up_rate & 0x100) == 0x100);

	if(mode == MPU_CYCLE_ON || mode == MPU_CYCLE_LP){
		// Validate wake-rate family against device family
		if(is_modern && !wake_is_modern){
			LOG_E("invalid wake_up_rate=0x%04X for who_am_i=0x%02X",
				wake_up_rate, g_mpu_conf.who_am_i);
			return false;
		}
		if(!is_modern && wake_is_modern){
			LOG_E("invalid wake_up_rate=0x%04X for who_am_i=0x%02X",
				wake_up_rate, g_mpu_conf.who_am_i);
			return false;
		}

		// ==========================================
		// Step 1: prepare PWR_MGMT_1 without CYCLE
		// ==========================================
		gb_mpu.tx.data[2] &= ~MPU_SLEEP;
		gb_mpu.tx.data[2] &= ~MPU_CYCLE;
		gb_mpu.tx.data[2] &= ~MPU_CLK_STOP;
		gb_mpu.tx.data[2] &= ~MPU_TEMP_DIS; // default: temp on

		if(mode == MPU_CYCLE_LP){
			gb_mpu.tx.data[2] |= MPU_TEMP_DIS;
			gb_mpu.tx.data[2] |= MPU_CLK_INTERNAL;
		}else{
			gb_mpu.tx.data[2] |= MPU_CLK_XGYRO;
		}

		gb_mpu.raw[0] = MPU_REG_PWR_MGMT_1;
		gb_mpu.raw[1] = gb_mpu.tx.data[2];
		if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
			LOG_E("register write failed reg=0x%02X value=0x%02X",
				MPU_REG_PWR_MGMT_1, gb_mpu.tx.data[2]);
			return false;
		}
		sleep_ms(1);

		// ======================================
		// Step 2: configure wake behavior
		// ======================================
		if(is_modern){
			if(mode == MPU_CYCLE_LP)
				gb_mpu.tx.data[3] |= MPU_STBY_GYRO;
			else
				gb_mpu.tx.data[3] &= ~MPU_STBY_GYRO;

			gb_mpu.raw[0] = MPU_REG_PWR_MGMT_2;
			gb_mpu.raw[1] = gb_mpu.tx.data[3];
			if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
				LOG_E("register write failed reg=0x%02X value=0x%02X",
					MPU_REG_PWR_MGMT_2, gb_mpu.tx.data[3]);
				return false;
			}
			sleep_ms(1);

			gb_mpu.raw[0] = MPU6500_REG_LP_ACCEL_ODR;
			gb_mpu.raw[1] = (uint8_t)(wake_up_rate & 0x0F);
			if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
				LOG_E("register write failed reg=0x%02X value=0x%02X",
					MPU6500_REG_LP_ACCEL_ODR, gb_mpu.raw[1]);
				return false;
			}
			sleep_ms(1);
		}else{
			gb_mpu.tx.data[3] &= ~MPU_LP_WAKE_40HZ;
			gb_mpu.tx.data[3] |= (uint8_t)wake_up_rate;

			if(mode == MPU_CYCLE_LP)
				gb_mpu.tx.data[3] |= MPU_STBY_GYRO;
			else
				gb_mpu.tx.data[3] &= ~MPU_STBY_GYRO;

			gb_mpu.raw[0] = MPU_REG_PWR_MGMT_2;
			gb_mpu.raw[1] = gb_mpu.tx.data[3];
			if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
				LOG_E("register write failed reg=0x%02X value=0x%02X",
					MPU_REG_PWR_MGMT_2, gb_mpu.tx.data[3]);
				return false;
			}
			sleep_ms(1);
		}

		// ======================================
		// Step 3: enable CYCLE last
		// ======================================
		gb_mpu.tx.data[2] |= MPU_CYCLE;

		gb_mpu.raw[0] = MPU_REG_PWR_MGMT_1;
		gb_mpu.raw[1] = gb_mpu.tx.data[2];
		if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
			LOG_E("register write failed reg=0x%02X value=0x%02X",
				MPU_REG_PWR_MGMT_1, gb_mpu.tx.data[2]);
			return false;
		}
		sleep_ms(3);

		if(mode == MPU_CYCLE_LP)
			LOG_I("low power cycle mode started who_am_i=0x%02X", g_mpu_conf.who_am_i);
		else
			LOG_I("cycle mode started who_am_i=0x%02X", g_mpu_conf.who_am_i);
	}else{
		LOG_I("cycle mode deactivation started");

		gb_mpu.tx.data[2] &= ~MPU_CYCLE;
		gb_mpu.tx.data[2] &= ~MPU_TEMP_DIS;
		gb_mpu.tx.data[3] &= ~MPU_STBY_GYRO;

		if(!is_modern)
			gb_mpu.tx.data[3] &= ~MPU_LP_WAKE_40HZ;

		gb_mpu.tx.head    = MPU_REG_PWR_MGMT_1;
		gb_mpu.tx.data[0] = gb_mpu.tx.data[2];
		gb_mpu.tx.data[1] = gb_mpu.tx.data[3];

		if(!_mpu_write_reg(gb_mpu.raw, 3, false)){
			LOG_E("register write failed reg=0x%02X values=0x%02X,0x%02X",
				gb_mpu.tx.head, gb_mpu.tx.data[0], gb_mpu.tx.data[1]);
			return false;
		}
		sleep_ms(2);
	}

	LOG_I("cycle mode applied who_am_i=0x%02X pwr_mgmt_1=0x%02X pwr_mgmt_2=0x%02X",
		g_mpu_conf.who_am_i, gb_mpu.tx.data[2], gb_mpu.tx.data[3]);

	return true;
}

/**
 * @brief Sets the Full-Scale Range (FSR) for Gyro and Accel and updates scaling factors.
 *
 * This function configures the sensitivity of the sensors and automatically
 * calculates the required divisors for converting raw data into G-force and
 * Degrees-per-Second.
 *
 * @param fsr  The gyroscope range (use @ref mpu_fsr_t, e.g., @ref MPU_FSR_2000DPS).
 * @param afsr The accelerometer range (use @ref mpu_afsr_t, e.g., @ref MPU_AFSR_16G).
 *
 * @return true  If the configuration registers were written successfully.
 * @return false If I2C communication failed.
 *
 * @note The calculated divisors are stored in @c g_mpu_cfg.fsr_div and
 *       applied during @ref mpu_read() when @ref MPU_SCALED is requested.
 */
bool mpu_fsr(mpu_fsr_t fsr, mpu_afsr_t afsr){
	// Read FSR Register
	gb_mpu.tx.head = MPU_REG_GYRO_CONFIG;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 2)){
		LOG_E("register read failed reg=0x%02X len=2", gb_mpu.tx.head);
		return false;
	}

	// Gyro FSR bits
	gb_mpu.tx.data[0] &= ~MPU_FSR_2000DPS; // Delete bits 4:3
	gb_mpu.tx.data[0] |= fsr; // Set FSR Bits
	LOG_I("gyro fsr prepared value=0x%02X", fsr);

	// Automatic scaling calculation:
	// 131 / 2^bits → sensitivity in °/s
	g_mpu_conf.fsr_div.gyro = 131.0f / (1 << ((fsr >> 3) & 0x03));
	LOG_I("gyro fsr_div set value=%.3f", g_mpu_conf.fsr_div.gyro);

	// Accel FSR bits
	gb_mpu.tx.data[1] &= ~MPU_AFSR_16G;
	gb_mpu.tx.data[1] |= afsr;
	LOG_I("accel afsr prepared value=0x%02X", afsr);

	// Automatic scaling calculation (raw / divider = G)
	g_mpu_conf.fsr_div.accel = 16384.0f / (1 << ((afsr >> 3) & 0x03));
	LOG_I("accel afsr_div set value=%.3f", g_mpu_conf.fsr_div.accel);

	// Write back to registers
	if(!_mpu_write_reg(gb_mpu.raw, 3, false)){
		LOG_E("register write failed reg=0x%02X values=0x%02X,0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0], gb_mpu.tx.data[1]);
		return false;
	}

	sleep_ms(5);

	return true;
}

/**
 * @brief Calibrates the sensors to determine zero-point offsets.
 *
 * Averaging multiple samples, this function calculates the static bias of
 * the sensors. The resulting offsets are stored in the global @ref g_mpu struct
 * and used automatically during @ref mpu_read() if @ref MPU_SCALED is set.
 *
 * @param sensor  Bitmask of sensors to calibrate.
 *                For @ref MPU_ACCEL, you MUST provide an axis modifier
 *                (e.g., @ref MPU_ACCEL_Z) to account for earth's gravity.
 * @param samples Number of measurements to average (e.g., 100).
 *
 * @return true  If calibration was successful.
 * @return false If @ref g_mpu is NULL or I2C communication failed.
 *
 * @note For the accelerometer, the function subtracts 16384 LSB (1g) from
 *       the axis pointing "up" to ensure the offset represents true 0g.
 * @warning Ensure the device is perfectly still during calibration!
 */
bool mpu_calibrate(mpu_sensor_t sensor, uint8_t samples){
	int64_t sum_x = 0; // cache
	int64_t sum_y = 0; // cache
	int64_t sum_z = 0; // cache

	uint8_t mask = (sensor & MPU_ALL); // mask where only the sensors bits are given

	if(mask & MPU_GYRO){ // Checks if gyro should be calibrated
		LOG_I("gyro calibration started");
		for(uint8_t i = 0; i < samples; i++){
			if(!_mpu_read_reg(MPU_REG_GYRO_XOUT_H, gb_mpu.raw, 6)){ // Read the gyro output
				LOG_E("register read failed reg=0x%02X len=6", MPU_REG_GYRO_XOUT_H);
				return false;
			}

			g_mpu.v.gyro.raw.x = be16_idx(gb_mpu.raw, 0); // Store x axis output in gyro.raw.x
			g_mpu.v.gyro.raw.y = be16_idx(gb_mpu.raw, 2); // Store y axis output in gyro.raw.y
			g_mpu.v.gyro.raw.z = be16_idx(gb_mpu.raw, 4); // Store z axis output in gyro.raw.z

			sum_x += g_mpu.v.gyro.raw.x; // Add x axis output to sum_x
			sum_y += g_mpu.v.gyro.raw.y; // Add y axis output to sum_y
			sum_z += g_mpu.v.gyro.raw.z; // Add z axis output to sum_z

			sleep_ms(5); // small delay between measurements
		}

		g_mpu_conf.offset_gyro.x = sum_x / samples; // Store x axis average as offset_gyro.x
		g_mpu_conf.offset_gyro.y = sum_y / samples; // Store y axis average as offset_gyro.y
		g_mpu_conf.offset_gyro.z = sum_z / samples; // Store z axis average as offset_gyro.z

		LOG_I("gyro calibration done");
		LOG_D("gyro offset x=%d y=%d z=%d", g_mpu_conf.offset_gyro.x, g_mpu_conf.offset_gyro.y, g_mpu_conf.offset_gyro.z);

	}
	if (mask & MPU_ACCEL){ // Checks if accelerometer should be calibrated
		g_mpu_conf.offset_accel.x = 0;
		g_mpu_conf.offset_accel.y = 0;
		g_mpu_conf.offset_accel.z = 0;

		sum_x = 0; sum_y = 0; sum_z = 0; // Set sum back to `0` in case both sensors got read
		LOG_I("accel calibration started");
		for(uint8_t i = 0; i < samples; i++){
			if(!_mpu_read_reg(MPU_REG_ACCEL_XOUT_H, gb_mpu.raw, 6)){ // Read the accel output
				LOG_E("register read failed reg=0x%02X len=6", MPU_REG_ACCEL_XOUT_H);
				return false;
			}

			g_mpu.v.accel.raw.x = be16_idx(gb_mpu.raw, 0); // Store x axis output in accel.raw.x
			g_mpu.v.accel.raw.y = be16_idx(gb_mpu.raw, 2); // Store y axis output in accel.raw.y
			g_mpu.v.accel.raw.z = be16_idx(gb_mpu.raw, 4); // Store z axis output in accel.raw.z

			sum_x += g_mpu.v.accel.raw.x; // Add x axis output to sum_x
			sum_y += g_mpu.v.accel.raw.y; // Add y axis output to sum_y
			sum_z += g_mpu.v.accel.raw.z; // Add z axis output to sum_z

			sleep_ms(5); // small delay between measurements
		}

		g_mpu_conf.offset_accel.x = sum_x / samples; // Store x axis average as offset_accel.x
		g_mpu_conf.offset_accel.y = sum_y / samples; // Store y axis average as offset_accel.y
		g_mpu_conf.offset_accel.z = sum_z / samples; // Store z axis average as offset_accel.z

		int32_t one_g = (int32_t)g_mpu_conf.fsr_div.accel;
		if((sensor & MPU_ACCEL_X) == MPU_ACCEL_X){
			if(g_mpu_conf.offset_accel.x > 0)
				g_mpu_conf.offset_accel.x -= one_g;
			else
				g_mpu_conf.offset_accel.x += one_g;
		}

		if((sensor & MPU_ACCEL_Y) == MPU_ACCEL_Y){
			if(g_mpu_conf.offset_accel.y > 0)
				g_mpu_conf.offset_accel.y -= one_g;
			else
				g_mpu_conf.offset_accel.y += one_g;
		}

		if((sensor & MPU_ACCEL_Z) == MPU_ACCEL_Z){
			if(g_mpu_conf.offset_accel.z > 0)
				g_mpu_conf.offset_accel.z -= one_g;
			else
				g_mpu_conf.offset_accel.z += one_g;
		}

		LOG_I("accel calibration done");
		LOG_D("accel offset x=%d y=%d z=%d", g_mpu_conf.offset_accel.x, g_mpu_conf.offset_accel.y, g_mpu_conf.offset_accel.z);
	}

	return true; // If everything goes right
}

/**
 * @brief Reads sensor data and optionally calculates scaled values.
 *
 * This function handles both single sensor reads and optimized burst reads.
 * If more than one sensor type is requested in the mask, it performs a single
 * 14-byte I2C transaction to reduce overhead.
 *
 * @param sensor Bitmask of sensors to read and/or scale (use @ref mpu_sensor_t).
 *               Example: (MPU_ACCEL | MPU_GYRO | MPU_SCALED)
 *
 * @return true  If I2C read and optional scaling were successful.
 * @return false If @ref g_mpu is NULL or an I2C communication error occurred.
 *
 * @note If @ref MPU_SCALED is set, the function uses the offsets and FSR
 *       divisors stored in the global configuration struct.
 * @note FSYNC state is automatically extracted from the Temperature LSB
 *       during burst or temperature reads.
 */
bool mpu_read(mpu_sensor_t sensor){
	uint8_t mask = (sensor & MPU_ALL); // A mask where only sensor bits are given

	if((mask & (mask - 1))){ // If two or more sensors are read it reads all for less overhead.

		if(!_mpu_read_reg(MPU_REG_ACCEL_XOUT_H, gb_mpu.raw, 14)){ // Read all output register
			LOG_E("register read failed reg=0x%02X len=14", MPU_REG_ACCEL_XOUT_H);
			return false;
		}

		g_mpu.v.accel.raw.x = be16_idx(gb_mpu.raw, 0) - g_mpu_conf.offset_accel.x; // Save raw accelerometer x axis
		g_mpu.v.accel.raw.y = be16_idx(gb_mpu.raw, 2) - g_mpu_conf.offset_accel.y; // Save raw accelerometer y axis
		g_mpu.v.accel.raw.z = be16_idx(gb_mpu.raw, 4) - g_mpu_conf.offset_accel.z; // Save raw accelerometer z axis
		g_mpu.v.temp.raw =    be16_idx(gb_mpu.raw, 6); // Save raw temperatur
		g_mpu.v.gyro.raw.x =  be16_idx(gb_mpu.raw, 8) - g_mpu_conf.offset_gyro.x; // Save raw gyro x axis
		g_mpu.v.gyro.raw.y =  be16_idx(gb_mpu.raw, 10) - g_mpu_conf.offset_gyro.y; // Save raw gyro y axis ;
		g_mpu.v.gyro.raw.z =  be16_idx(gb_mpu.raw, 12) - g_mpu_conf.offset_gyro.z; // Save raw gyro z axis ;

		LOG_D("raw burst accel_x=%d accel_y=%d accel_z=%d temp=%d gyro_x=%d gyro_y=%d gyro_z=%d",
				g_mpu.v.accel.raw.x, g_mpu.v.accel.raw.y, g_mpu.v.accel.raw.z,
				g_mpu.v.temp.raw,
				g_mpu.v.gyro.raw.x, g_mpu.v.gyro.raw.y, g_mpu.v.gyro.raw.z);
	}else{
		if(mask & MPU_ACCEL){ // Only accelerometer
			if(!_mpu_read_reg(MPU_REG_ACCEL_XOUT_H, gb_mpu.raw, 6)){ // Read accelerometer output register
				LOG_E("register read failed reg=0x%02X len=6", MPU_REG_ACCEL_XOUT_H);
				return false;
			}

			g_mpu.v.accel.raw.x = be16_idx(gb_mpu.raw, 0) - g_mpu_conf.offset_accel.x; // Save raw accelerometer x axis
			g_mpu.v.accel.raw.y = be16_idx(gb_mpu.raw, 2) - g_mpu_conf.offset_accel.y; // Save raw accelerometer y axis
			g_mpu.v.accel.raw.z = be16_idx(gb_mpu.raw, 4) - g_mpu_conf.offset_accel.z; // Save raw accelerometer z axis

			LOG_D("raw accel x=%d y=%d z=%d", g_mpu.v.accel.raw.x, g_mpu.v.accel.raw.y, g_mpu.v.accel.raw.z);
		}
		if(mask & MPU_TEMP){ // Only temperatur
			if(!_mpu_read_reg(MPU_REG_TEMP_OUT_H, gb_mpu.raw, 2)){ // Reads temperatur output register
				LOG_E("register read failed reg=0x%02X len=2", MPU_REG_TEMP_OUT_H);
				return false;
			}

			g_mpu.v.temp.raw = byte_make_u16_be(gb_mpu.raw[0], gb_mpu.raw[1]); // Save raw temperatur

			LOG_D("raw temp=%d", g_mpu.v.temp.raw);
		}
		if(mask & MPU_GYRO){ // Only gyroscope
			if(!_mpu_read_reg(MPU_REG_GYRO_XOUT_H, gb_mpu.raw, 6)){ // Read gyro output register
				LOG_E("register read failed reg=0x%02X len=6", MPU_REG_GYRO_XOUT_H);
				return false;
			}

			g_mpu.v.gyro.raw.x = be16_idx(gb_mpu.raw, 0) - g_mpu_conf.offset_gyro.x; // Save raw gyro x axis
			g_mpu.v.gyro.raw.y = be16_idx(gb_mpu.raw, 2) - g_mpu_conf.offset_gyro.y; // Save raw gyro y axis
			g_mpu.v.gyro.raw.z = be16_idx(gb_mpu.raw, 4) - g_mpu_conf.offset_gyro.z; // Save raw gyro z axis

			LOG_D("raw gyro x=%d y=%d z=%d", g_mpu.v.gyro.raw.x, g_mpu.v.gyro.raw.y, g_mpu.v.gyro.raw.z);
		}
	}

	if(sensor & MPU_SCALED){ // Optional: scale raw values
		if(mask & MPU_ACCEL){ // Raw -> G for accelerometer
			g_mpu.v.accel.g.x = g_mpu.v.accel.raw.x / g_mpu_conf.fsr_div.accel; // Calculate raw x axis to G
			g_mpu.v.accel.g.y = g_mpu.v.accel.raw.y / g_mpu_conf.fsr_div.accel; // Calculate raw y axis to G
			g_mpu.v.accel.g.z = g_mpu.v.accel.raw.z / g_mpu_conf.fsr_div.accel; // Calculate raw z axis to G

			LOG_D("scaled accel x=%0.3fg y=%0.3fg z=%0.3fg", g_mpu.v.accel.g.x, g_mpu.v.accel.g.y, g_mpu.v.accel.g.z);
		}
		if(mask & MPU_TEMP){ // Raw -> °C
			g_mpu.v.temp.celsius = (g_mpu.v.temp.raw / 340.0f) + 36.53f; // Calculate raw temperatur to °C

			LOG_D("scaled temp celsius=%02.2f", g_mpu.v.temp.celsius);
		}

		if(mask & MPU_GYRO){ // Raw -> °/s for gyroscope
			g_mpu.v.gyro.dps.x = g_mpu.v.gyro.raw.x / g_mpu_conf.fsr_div.gyro; // Calculate raw x axis to °/s
			g_mpu.v.gyro.dps.y = g_mpu.v.gyro.raw.y / g_mpu_conf.fsr_div.gyro; // Calculate raw y axis to °/s
			g_mpu.v.gyro.dps.z = g_mpu.v.gyro.raw.z / g_mpu_conf.fsr_div.gyro; // Calculate raw z axis to °/s

			LOG_D("scaled gyro x=%0.3f°/s y=%0.3f°/s z=%0.3f°/s", g_mpu.v.gyro.dps.x, g_mpu.v.gyro.dps.y, g_mpu.v.gyro.dps.z);
		}
	}

	return true;
}

#if MPU_INT_PIN // Checks if MPU_INT_PIN is greater than 0
/** @brief Global flag indicating an MPU interrupt has occurred. Hardware-level volatile. */
volatile bool g_mpu_int_flag;

/**
 * @brief Low-level IRQ handler for the Raspberry Pi Pico GPIO.
 *
 * This function is registered as a callback. It checks if the triggering
 * pin is @ref MPU_INT_PIN and sets the global @ref g_mpu_int_flag.
 *
 * @param gpio   The GPIO pin number that triggered the interrupt.
 * @param events The bitmask of event types (e.g., EDGE_RISE).
 */
void _mpu_irq_handler(uint gpio, uint32_t events){
	(void)events;
    if(gpio == MPU_INT_PIN){   // Checks if the called pin is MPU_INT_PIN
        g_mpu_int_flag = true; // if it is set `g_mpu_int_flag` true.
    }
}

/**
 * @brief Configures the physical behavior of the MPU's interrupt and FSYNC pins.
 *
 * @param cfg Bitmask of configuration flags using @ref mpu_int_pin_cfg_t.
 * @return true  If the configuration was written successfully.
 * @return false If I2C communication failed.
 */
bool mpu_int_pin_cfg(mpu_int_pin_cfg_t cfg){
	gb_mpu.tx.head = MPU_REG_INT_PIN_CFG;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){ // Reads the INT_PIN_CFG register and save it in gc_mpu
			LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
			return false;
		}

	gb_mpu.tx.data[0] &= ~MPU_INT_PIN_CFG_ALL; // Unsets all interrupt bits
	gb_mpu.tx.data[0] |= cfg; // Set the bits given in `cfg`

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){ // Write back to registers
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(2); // Little activation pause

	LOG_I("interrupt pin config applied reg=0x%02X value=0x%02X", gb_mpu.tx.head, cfg);

	return true; // If everything goes right
}

/**
 * @brief Configures the motion detection interrupt thresholds.
 *
 * This function sets the acceleration threshold (mg) and the required
 * duration (ms) for a motion event to trigger an interrupt.
 *
 * @param ms Duration in milliseconds (1-255). Values are clamped to range.
 * @param mg Acceleration threshold in milli-g (32-8160).
 *           Input is divided by 32 to fit MPU's 1-LSB = 32mg scale.
 *
 * @return true  If high-pass filter and thresholds were set correctly.
 * @return false If I2C communication failed.
 *
 * @note Automatically enables the Accel High Pass Filter (AHPF) at 5Hz
 *       to filter out constant gravity and detect only movement.
 */
bool mpu_int_motion_cfg(uint8_t ms, uint16_t mg){
	switch(g_mpu_conf.who_am_i){
		// =====================================================
		// MPU-6000 / MPU-6050
		// Classic motion threshold + duration + accel HPF
		// =====================================================
		case MPU60X0_WHO_AM_I:
			if(ms < 1)         ms = 1;
			else if(ms > 254)  ms = 255;

			if(mg < 32)        mg = 1;
			else if(mg > 8160) mg = 255;
			else               mg /= 32;  // 1 LSB = 32 mg

			if(!mpu_ahpf(MPU_AHPF_5HZ)){
				LOG_E("ahpf config failed value=0x%02X", MPU_AHPF_5HZ);
				return false;
			}

			gb_mpu.tx.head    = MPU_REG_MOT_THR;
			gb_mpu.tx.data[0] = (uint8_t)mg;
			gb_mpu.tx.data[1] = ms;

			if(!_mpu_write_reg(gb_mpu.raw, 3, false)){
				LOG_E("register write failed reg=0x%02X values=0x%02X,0x%02X",
					gb_mpu.tx.head, gb_mpu.tx.data[0], gb_mpu.tx.data[1]);
				return false;
			}

			sleep_ms(2);

			LOG_I("motion cfg applied type=MPU60X0 reg=0x%02X values=0x%02X,0x%02X",
				gb_mpu.tx.head, gb_mpu.tx.data[0], gb_mpu.tx.data[1]);
			return true;

		// =====================================================
		// MPU-6500 / MPU-9250 / MPU-9255
		// Hardware Wake-On-Motion (WOM)
		// ms is not used here like on the 6050 path
		// =====================================================
		case MPU6500_WHO_AM_I:
		case MPU9250_WHO_AM_I:
		case MPU9255_WHO_AM_I:
			(void)ms;

			/*
			 * WOM threshold granularity on modern parts is much finer than on
			 * the 6050 path. This mapping uses 1 LSB = 4 mg.
			 */
			if(mg < 4)         mg = 1;
			else if(mg > 1020) mg = 255;
			else               mg /= 4;

			// Enable accel intelligence / WOM hardware
			gb_mpu.tx.head = MPU6500_REG_ACCEL_INTEL_CTRL;
			if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
				LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
				return false;
			}

			gb_mpu.tx.data[0] |= MPU6500_ACCEL_INTEL_EN;
			gb_mpu.tx.data[0] |= MPU6500_ACCEL_INTEL_MODE;

			if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
				LOG_E("register write failed reg=0x%02X value=0x%02X",
					gb_mpu.tx.head, gb_mpu.tx.data[0]);
				return false;
			}
			sleep_ms(1);

			// Write WOM threshold
			gb_mpu.tx.head    = MPU6500_REG_WOM_THR;
			gb_mpu.tx.data[0] = (uint8_t)mg;

			if(!_mpu_write_reg(gb_mpu.raw, 2, false)){
				LOG_E("register write failed reg=0x%02X value=0x%02X",
					gb_mpu.tx.head, gb_mpu.tx.data[0]);
				return false;
			}

			sleep_ms(2);

			LOG_I("motion cfg applied type=MPU65X0/92XX reg=0x%02X value=0x%02X",
				gb_mpu.tx.head, gb_mpu.tx.data[0]);
			return true;

		default:
			LOG_E("unsupported who_am_i=0x%02X", g_mpu_conf.who_am_i);
			return false;
	}
}

/**
 * @brief Configures and enables specific interrupt sources on the MPU.
 *
 * This function sets up the Pico's GPIO hardware to listen for the MPU's
 * interrupt signal and simultaneously configures the MPU's internal
 * @c INT_ENABLE register.
 *
 * @param interrupt Bitmask of interrupts to enable (use @ref mpu_int_enable_t).
 *                  Common values: @ref MPU_DATA_RDY_EN, @ref MPU_INT_MOTION_EN.
 *
 * @return true  If the interrupt handler was attached and I2C write succeeded.
 * @return false If I2C communication failed.
 *
 * @warning This function overwrites previous interrupt settings in the
 *          @c INT_ENABLE register but keeps the state of non-mapped bits.
 * @note    Includes a 2ms sleep to allow the interrupt logic to stabilize.
 */
bool mpu_int_enable(mpu_int_enable_t interrupt){
	gpio_set_irq_enabled_with_callback(MPU_INT_PIN, GPIO_IRQ_EDGE_RISE, true, &_mpu_irq_handler); // Listen MPU_INT_PIN call `mpu_irq_handler` if pin HIGH
	LOG_I("IRQ enabled gpio=%d edge=rising", MPU_INT_PIN);

	gb_mpu.tx.head = MPU_REG_INT_ENABLE;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){ // Read the INT_ENABLE register
			LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
			return false;
		}

	gb_mpu.tx.data[0] &= ~MPU_INT_ENABLE_ALL; // Unsets all interrupt bits
	gb_mpu.tx.data[0] |= interrupt; // Sets with the bitmask given by argument

	if(!_mpu_write_reg(gb_mpu.raw, 2, false)){ // Write back to registers
		LOG_E("register write failed reg=0x%02X value=0x%02X", gb_mpu.tx.head, gb_mpu.tx.data[0]);
		return false;
	}

	sleep_ms(2); // Little activation pause

	LOG_I("interrupt activated");

	g_mpu_int_flag = 0;
	return true; // When nothing goes wrong
}

/**
 * @brief Checks and clears the internal interrupt status of the MPU.
 *
 * First checks the local @ref g_mpu_int_flag. If true, it performs an I2C
 * read of the @c INT_STATUS register to determine the exact source.
 *
 * @return true  If any enabled interrupt (Data Ready, Motion, FIFO, etc.) is active.
 * @return false If no interrupt occurred or I2C read failed.
 *
 * @note Resets @ref g_mpu_int_flag to false upon being called.
 */
bool mpu_int_status(void){
	if(!g_mpu_int_flag) return false;
	g_mpu_int_flag = false;

	gb_mpu.raw[0] = 0;
	gb_mpu.raw[1] = 0;
	gb_mpu.tx.head = MPU_REG_INT_STATUS;
	if(!_mpu_read_reg(gb_mpu.tx.head, gb_mpu.tx.data, 1)){
		LOG_E("register read failed reg=0x%02X len=1", gb_mpu.tx.head);
		return false;
	}

	LOG_D("int_status=0x%02X", gb_mpu.tx.data[0]);

#if DEBUG_ENABLED
	if(gb_mpu.tx.data[0] & MPU_DATA_RDY_INT)
		LOG_D("interrupt source=raw_data_ready");

	if(gb_mpu.tx.data[0] & MPU_MOTION_INT)
		LOG_D("interrupt source=motion/wom");

	if(gb_mpu.tx.data[0] & MPU_FIFO_OFLOW_INT)
		LOG_D("interrupt source=fifo_overflow");
#endif

	return (gb_mpu.tx.data[0] & (MPU_DATA_RDY_INT | MPU_I2C_MST_INT | MPU_MOTION_INT | MPU_FIFO_OFLOW_INT)) != 0;
}
#endif
