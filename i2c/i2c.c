#include "hardware/resets.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "rp_pico.h"
#include "i2c.h"
#include "log.h"

typedef union {
	uint8_t raw;

	struct {
		uint8_t addr_noack    : 1; // no ACK on address
		uint8_t data_noack    : 1; // no ACK on data
		uint8_t arb_lost      : 1; // arbitration lost
		uint8_t master_dis    : 1; // master disabled
		uint8_t gcall_noack   : 1; // general call no ack
		uint8_t reserved      : 3;
	} bits;

} i2c_abort_flags_t;

#define I2C_HANDLE_TX_ABRT(_cfg, _addr, _ctx) do{ \
	if((_cfg)->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS){ \
		i2c_abort_flags_t f = _i2c_get_abort_flags((_cfg)); \
		LOG_W("I2C ABRT: %s addr=0x%02X raw=0x%02X", (_ctx), (_addr), f.raw); \
		if(f.bits.addr_noack) LOG_W("  addr noack"); \
		if(f.bits.data_noack) LOG_W("  data noack"); \
		if(f.bits.arb_lost) LOG_W("  arb lost"); \
		if(f.bits.master_dis) LOG_W("  master disabled"); \
		if(f.bits.gcall_noack) LOG_W("  gcall noack"); \
		(void)(_cfg)->hw->clr_tx_abrt; \
		return false; \
	} \
}while(0)

/**
 * @brief Internal helper to wait for a specific hardware status bit.
 * @param cfg Config struct for timeout value.
 * @param reg Pointer to the hardware register to poll.
 * @param mask Bitmask for the status bit.
 * @param state Target state (true = bit set, false = bit cleared).
 * @param context String for error logging.
 * @return true if state reached, false on timeout.
 */
static bool _i2c_wait_for_status(const i2c_hw_config *cfg, volatile uint32_t *reg, uint32_t mask, bool state, const char* context) {
	absolute_time_t timeout_time = make_timeout_time_us(cfg->timeout_us);

	// Polling loop with safety timeout
	while (((*reg & mask) != 0) != state) {
		if (time_reached(timeout_time)) {
			LOG_E("I2C Timeout: %s (Reg: 0x%08X, Mask: 0x%08X): ", context, (uint32_t)reg, mask);
			return false;
		}
	}
	return true;
}

static i2c_abort_flags_t _i2c_get_abort_flags(const i2c_hw_config *cfg){
	i2c_abort_flags_t f;
	uint32_t abrt = cfg->hw->tx_abrt_source;

	f.raw = 0;

	if(abrt & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) f.bits.addr_noack = 1;
	if(abrt & I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS)  f.bits.data_noack = 1;
	if(abrt & I2C_IC_TX_ABRT_SOURCE_ARB_LOST_BITS)           f.bits.arb_lost = 1;
	if(abrt & I2C_IC_TX_ABRT_SOURCE_ABRT_MASTER_DIS_BITS)    f.bits.master_dis = 1;
	if(abrt & I2C_IC_TX_ABRT_SOURCE_ABRT_GCALL_NOACK_BITS)   f.bits.gcall_noack = 1;

	return f;
}

/**
 * @brief Helper to update the target address (TAR).
 */
static void _i2c_set_addr(const i2c_hw_config *cfg, uint8_t addr) {
	if ((cfg->hw->tar & I2C_IC_TAR_IC_TAR_BITS) != addr) {
		cfg->hw->enable = 0; // Must be disabled to change TAR
		cfg->hw->tar = addr;
		cfg->hw->enable = 1;
	}
}

/**
 * @brief Internal check to verify if the I2C peripheral is fully initialized.
 *
 * Validates the hardware state by checking if the block is out of reset,
 * enabled at the hardware level, and currently configured in master mode.
 *
 * @param cfg Pointer to the custom I2C hardware configuration structure.
 * @return true  If the peripheral is out of reset, enabled, and in master mode.
 * @return false If any of the required hardware states are not met.
 */
bool i2c_is_initialized(const i2c_hw_config *cfg) {
    // 1. Check if the peripheral has been released from the system reset state
    uint reset_bit = (cfg->hw == i2c0_hw) ? RESETS_RESET_I2C0_BITS : RESETS_RESET_I2C1_BITS;
    bool reset = !(resets_hw->reset & reset_bit);

    // 2. Check if the ENABLE bit and MASTER_MODE bit are both set in hardware
    // cfg->hw provides direct access to the register structure (e.g., IC_ENABLE and IC_CON)
    bool enabled = (cfg->hw->enable & I2C_IC_ENABLE_ENABLE_BITS) != 0;
    bool master = (cfg->hw->con & I2C_IC_CON_MASTER_MODE_BITS) != 0;

    return (reset && enabled && master);
}

/**
 * @brief Checks if the I2C Bus is currently executing a command.
 */
bool i2c_is_busy(const i2c_hw_config *cfg) {
	// Activity bit 0 in IC_STATUS is high if the controller is not idle
	return (cfg->hw->status & I2C_IC_STATUS_ACTIVITY_BITS) != 0;
}

/**
 * @brief Manually toggles SCL as GPIO to force a slave to release SDA.
 */
void i2c_recover_bus(const i2c_hw_config *cfg) {
	LOG_D("Checking bus health on SDA:%d SCL:%d", cfg->sda_pin, cfg->scl_pin);

	// Set pins to GPIO mode for manual bit-banging
	gpio_init(cfg->sda_pin);
	gpio_init(cfg->scl_pin);
	gpio_set_dir(cfg->sda_pin, GPIO_IN);
	gpio_set_dir(cfg->scl_pin, GPIO_OUT);

	// If SDA is stuck LOW, a slave might be waiting for clock pulses
	if (gpio_get(cfg->sda_pin) == 0) {
		LOG_W("I2C Bus hung (SDA LOW). Attempting recovery...");
		for (int i = 0; i < 9; i++) {
			gpio_put(cfg->scl_pin, 0); sleep_us(5);
			gpio_put(cfg->scl_pin, 1); sleep_us(5);
			if (gpio_get(cfg->sda_pin) == 1) {
				LOG_I("I2C Bus recovered after %d pulses.", i + 1);
				break;
			}
		}
	}

	// Re-assign pins to the I2C hardware block
	gpio_set_function(cfg->sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(cfg->scl_pin, GPIO_FUNC_I2C);
}

/**
 * @brief Force SDA into a defined state after I2C hot-plug conditions.
 *
 * @details
 * When the I2C peripheral is already initialized but no device is connected,
 * the controller can observe an undefined bus state (e.g. SDA low or a partial
 * transfer condition). In this situation, attaching a device later does not
 * guarantee a valid idle state on the bus.
 *
 * As a result, initial probe operations (read/write) may fail or time out,
 * even though the device is physically present and functional.
 *
 * This helper toggles the SDA pin direction multiple times to release and
 * re-drive the line, forcing a clean electrical state and re-synchronizing
 * the controller with the actual bus level before performing detection.
 *
 * @note
 * - Required only for hot-plug scenarios.
 * - Should not be needed for stable, permanently connected I2C devices.
 * - Acts as a lightweight bus resynchronization, not a full I2C recovery.
 */
void i2c_hotplug_recover_sda(uint8_t sda_pin){
	for(size_t i = 0; i < 6; i++){
		gpio_set_dir(sda_pin, GPIO_IN);
		gpio_set_dir(sda_pin, GPIO_OUT);
		sleep_us(10);
	}
	gpio_set_dir(sda_pin, GPIO_IN);
}

/**
 * @brief Performs Low-Level hardware initialization.
 */
void i2c_initialize(i2c_hw_config *cfg) {
	if (i2c_is_initialized(cfg)) {
		LOG_D("I2C hardware already active. Skipping init.");
		return;
	}

	LOG_I("Initializing I2C at %d Hz", cfg->baudrate);
	i2c_recover_bus(cfg);

	// Hard reset the I2C peripheral block
	uint32_t reset_bit = (cfg->hw == i2c0_hw) ? RESETS_RESET_I2C0_BITS : RESETS_RESET_I2C1_BITS;
	reset_block(reset_bit);
	unreset_block_wait(reset_bit);

	// Disable controller to allow configuration
	cfg->hw->enable = 0;

	// Configure: Master Mode, Slave Disabled, Restart Enabled, Fast Mode (0x2)
	cfg->hw->con = I2C_IC_CON_MASTER_MODE_BITS |
		I2C_IC_CON_IC_SLAVE_DISABLE_BITS |
		I2C_IC_CON_IC_RESTART_EN_BITS |
		(0x2 << I2C_IC_CON_SPEED_LSB);

	// Calculate High/Low Count for SCL clock cycles
	uint32_t freq_in = clock_get_hz(clk_sys);
	uint32_t period = (freq_in + cfg->baudrate / 2) / cfg->baudrate;
	cfg->hw->fs_scl_hcnt = period * 3 / 8; // 37.5% High
	cfg->hw->fs_scl_lcnt = period * 5 / 8; // 62.5% Low

	// Set pins (standard SDK function used for safety)
	gpio_set_function(cfg->sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(cfg->scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(cfg->sda_pin);
	gpio_pull_up(cfg->scl_pin);

	// Enable the controller
	cfg->hw->enable = 1;
	LOG_D("I2C controller is now online.");
}

/**
 * @brief Fully featured write buffer with optional STOP suppression.
 */
bool i2c_write_buffer(const i2c_hw_config *cfg, uint8_t addr, const uint8_t *src, size_t len, bool nostop){
	if(len == 0) return false;

	(void)cfg->hw->clr_tx_abrt;
	_i2c_set_addr(cfg, addr);

	for(size_t i = 0; i < len; i++){
		if(!_i2c_wait_for_status(cfg, (volatile uint32_t*)&cfg->hw->status, I2C_IC_STATUS_TFNF_BITS, true, "TX_FIFO_WAIT")) return false;

		uint32_t cmd = src[i];
		if((i == len - 1) && !nostop) cmd |= I2C_IC_DATA_CMD_STOP_BITS;

		cfg->hw->data_cmd = cmd;
	}

	absolute_time_t timeout_time = make_timeout_time_us(cfg->timeout_us);

	while(1){
		I2C_HANDLE_TX_ABRT(cfg, addr, "write");

		if(!nostop){
			if(!i2c_is_busy(cfg) && (cfg->hw->status & I2C_IC_STATUS_TFE_BITS)) break;
		}else{
			if(cfg->hw->status & I2C_IC_STATUS_TFE_BITS) break;
		}

		if(time_reached(timeout_time)){
			LOG_E("I2C Timeout (Write) at addr 0x%02X", addr);
			return false;
		}
	}

	I2C_HANDLE_TX_ABRT(cfg, addr, "write late");

	return true;
}


/**
 * @brief Low-level read implementation. Triggers CMD bits in FIFO.
 */
bool i2c_read_buffer(const i2c_hw_config *cfg, uint8_t addr, uint8_t *dst, size_t len, bool nostop){
	if(len == 0 || !dst) return false;

	(void)cfg->hw->clr_tx_abrt;
	_i2c_set_addr(cfg, addr);

	for(size_t i = 0; i < len; i++){
		if(!_i2c_wait_for_status(cfg, (volatile uint32_t*)&cfg->hw->status, I2C_IC_STATUS_TFNF_BITS, true, "RX_CMD_WAIT")) return false;

		uint32_t cmd = I2C_IC_DATA_CMD_CMD_BITS;
		if((i == len - 1) && !nostop) cmd |= I2C_IC_DATA_CMD_STOP_BITS;
		cfg->hw->data_cmd = cmd;
	}

	for(size_t i = 0; i < len; i++){
		absolute_time_t timeout_time = make_timeout_time_us(cfg->timeout_us);

		while(1){
			I2C_HANDLE_TX_ABRT(cfg, addr, "read");

			if(cfg->hw->status & I2C_IC_STATUS_RFNE_BITS){
				dst[i] = (uint8_t)cfg->hw->data_cmd;
				break;
			}

			if(time_reached(timeout_time)){
				LOG_E("I2C Timeout: RX_DATA_TIMEOUT addr=0x%02X", addr);
				return false;
			}
		}
	}

	if(!nostop){
		absolute_time_t timeout_time = make_timeout_time_us(cfg->timeout_us);

		while(i2c_is_busy(cfg)){
			I2C_HANDLE_TX_ABRT(cfg, addr, "read late");

			if(time_reached(timeout_time)){
				LOG_E("I2C Busy Timeout (Read) at addr 0x%02X", addr);
				return false;
			}
		}
	}

	I2C_HANDLE_TX_ABRT(cfg, addr, "read final");

	return true;
}

/**
 * @brief Scan the I2C bus for devices using a 1-byte read probe.
 *
 * @details
 * This checks whether a device acknowledges its 7-bit address.
 * The returned byte value is ignored. Only the ACK/NACK matters.
 */
bool i2c_scan_bus(const i2c_hw_config *cfg, bool print_empty) {
    bool found_any = false;
    uint8_t dummy = 0;

    LOG_I("Scanning I2C bus...");

    for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
        int ret = i2c_read_buffer(cfg, addr, &dummy, 1, false);

        if (ret == 1) {
            LOG_I("I2C device found at 0x%02X", addr);
            found_any = true;
        } else if (print_empty) {
            LOG_I("No response at 0x%02X", addr);
        }
    }

    if (!found_any) {
        LOG_W("No I2C devices found.");
    }

    return found_any;
}
