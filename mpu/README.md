# MPU Driver for Raspberry Pi Pico

Low-level MPU driver for Raspberry Pi Pico with register-close configuration, sensor readout, calibration, interrupt support, timing control, and cycle mode handling.

---

## Features

- Low-level register access
- MPU initialization and device detection
- Gyroscope, accelerometer, and temperature readout
- Raw and scaled sensor values
- Gyroscope and accelerometer calibration
- Clock source configuration
- Sample rate and digital low-pass filter configuration
- Interrupt pin and interrupt source handling
- Motion interrupt configuration
- Low-power cycle mode support
- Device reset, sleep, and standby control

---

## Supported API

### Core / Setup

- `mpu_init(i2c_hw_t *i2c_hw, mpu_addr_t addr)`
- `mpu_who_am_i(void)`
- `mpu_bypass(bool active)`

### Power / Clock / Timing

- `mpu_reset(mpu_reset_t reset)`
- `mpu_sleep(mpu_sleep_t sleep)`
- `mpu_stby(mpu_stby_t stby)`
- `mpu_clk_sel(mpu_clk_sel_t clksel)`
- `mpu_dlpf_cfg(mpu_dlpf_cfg_t cfg)`
- `mpu_smplrt_div(mpu_smplrt_div_t smplrt_div)`
- `mpu_ahpf(mpu_ahpf_t ahpf)`
- `mpu_cycle_mode(mpu_cycle_t mode, mpu_lp_wake_t wake_up_rate)`

### Sensor / Data

- `mpu_fsr(mpu_fsr_t fsr, mpu_afsr_t afsr)`
- `mpu_calibrate(mpu_sensor_t sensor, uint8_t samples)`
- `mpu_read(mpu_sensor_t sensor)`

### Interrupts

- `mpu_int_pin_cfg(mpu_int_pin_cfg_t cfg)`
- `mpu_int_enable(mpu_int_enable_t enable)`
- `mpu_int_motion_cfg(uint8_t ms, uint16_t mg)`
- `mpu_int_status(void)`

---

## File Overview

### Public Headers

- `mpu.h`
  Main public API

- `mpu_reg_map.h`
  Register addresses, bit masks, and configuration enums

- `mpu_types.h`
  Public enums and data structures

### Source File

- `mpu.c`
  Driver implementation, internal helpers, IRQ handler, register access

---

## Important Public Types

### Sensor selection

- `mpu_sensor_t`
- `mpu_cycle_t`
- `mpu_sleep_t`
- `mpu_addr_t`
- `mpu_reset_t`

### Timing / filtering / scaling

- `mpu_smplrt_div_t`
- `mpu_dlpf_cfg_t`
- `mpu_ext_sync_set_t`
- `mpu_fsr_t`
- `mpu_afsr_t`
- `mpu_ahpf_t`
- `mpu_clk_sel_t`
- `mpu_lp_wake_t`

### Interrupts

- `mpu_int_pin_cfg_t`
- `mpu_int_enable_t`
- `mpu_mot_count_t`
- `mpu_mot_delay_t`

### Sensor output

- `mpu_value_t`

---

## Important Defines

### Pin / hardware config

- `MPU_I2C_HW`
- `MPU_SDA_PIN`
- `MPU_SCL_PIN`
- `MPU_USE_PULLUP`
- `MPU_INT_PIN`
- `MPU_INT_PULLUP`

### Register map

See `mpu_reg_map.h` for the full register and bit definition list.

---

## Minimal Example

    #include "mpu.h"

    int main(void) {
        mpu_value_t *mpu;

        mpu = mpu_init(MPU_I2C_HW, MPU_ADDR_AD0_GND);
        if (!mpu) {
            return 1;
        }

        mpu_reset(MPU_RESET_ALL);
        mpu_sleep(MPU_SLEEP_ALL_OFF);
        mpu_clk_sel(MPU_CLK_XGYRO);
        mpu_fsr(MPU_FSR_2000DPS, MPU_AFSR_8G);
        mpu_dlpf_cfg(MPU_DLPF_CFG_44HZ);
        mpu_smplrt_div(MPU_SMPLRT_1KHZ);

        mpu_calibrate(MPU_ACCEL_Z | MPU_GYRO, 100);
        mpu_read(MPU_ALL | MPU_SCALED);

        return 0;
    }

---

## Documentation

Detailed topics are split into separate files:

- [Interrupts](readme/interrupts.md)
- [Timing and Filters](readme/timing.md)
- [Cycle Mode](readme/cycle.md)

---

## Typical Setup Order

1. Initialize the driver
2. Detect the device
3. Reset and wake up the chip
4. Select clock source
5. Configure filter and sample rate
6. Configure full-scale ranges
7. Calibrate sensors if needed
8. Enable interrupts or cycle mode if required
9. Read sensor data

---

## Notes

- This driver is intentionally close to the register layout.
- Most enums directly map to register bits or bit fields.
- Recommended clock source is usually gyro PLL instead of the internal oscillator.
- Timing, interrupts, and cycle mode differ between legacy MPU60X0 and newer MPU6500/925X variants.

---

## Status

- Public declarations and implementations are currently in sync.
- No missing public declarations.
- No missing public implementations.
