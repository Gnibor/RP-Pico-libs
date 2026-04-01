# Cycle Mode

This file documents low-power cycle mode behavior and related wake-up timing options.

---

## Overview

Relevant public function:

- `mpu_cycle_mode(mpu_cycle_t mode, mpu_lp_wake_t wake_up_rate)`

Related public functions:

- `mpu_sleep(mpu_sleep_t sleep)`
- `mpu_stby(mpu_stby_t stby)`
- `mpu_clk_sel(mpu_clk_sel_t clksel)`

Relevant enums:

- `mpu_cycle_t`
- `mpu_lp_wake_t`
- `mpu_sleep_t`
- `mpu_stby_t`

---

## What Cycle Mode Does

Cycle mode is used for low-power operation.

In this mode, the device does not run the full measurement chain continuously.
Instead, it wakes up periodically, performs limited sensing work, and returns to a reduced-power state.

Typical use cases:

- wake-on-motion
- battery-powered sensor nodes
- periodic low-power motion checks
- idle state monitoring

---

## Public API

### Enable / disable

- `MPU_CYCLE_ON`
- `MPU_CYCLE_OFF`

### Low-power marker enum

- `mpu_lp_wake_t`

This enum contains both:

- legacy MPU60X0 wake-up rates
- newer MPU6500/925X low-power accel ODR values

---

## Wake-Up Rates

### Legacy MPU (60X0)

Available rates:

- `MPU_LP_WAKE_1_25HZ`
- `MPU_LP_WAKE_5HZ`
- `MPU_LP_WAKE_20HZ`
- `MPU_LP_WAKE_40HZ`

### Modern MPU (6500 / 925X)

Available rates:

- `MPU6500_LP_WAKE_0_24HZ`
- `MPU6500_LP_WAKE_0_49HZ`
- `MPU6500_LP_WAKE_0_98HZ`
- `MPU6500_LP_WAKE_1_95HZ`
- `MPU6500_LP_WAKE_3_91HZ`
- `MPU6500_LP_WAKE_7_81HZ`
- `MPU6500_LP_WAKE_15_63HZ`
- `MPU6500_LP_WAKE_31_25HZ`
- `MPU6500_LP_WAKE_62_5HZ`
- `MPU6500_LP_WAKE_125HZ`
- `MPU6500_LP_WAKE_250HZ`
- `MPU6500_LP_WAKE_500HZ`

---

## Legacy vs Modern Devices

The enum already contains both families in one type.
This is convenient for the public API, but it also means the implementation must distinguish between:

- old wake-up bits in `PWR_MGMT_2`
- newer low-power accel ODR register handling

This section should later document exactly how the driver detects and applies the correct mode.

---

## Relationship to Sleep and Standby

Cycle mode is closely related to:

- `mpu_sleep()`
- `mpu_stby()`

### Sleep

Sleep mode is a stronger power reduction state.

### Standby

Standby disables selected axes or sensor groups.

### Cycle mode

Cycle mode is periodic wake-up behavior, not simply full sleep.

This section should later explain which combinations are valid and which register bits conflict.

---

## Typical Use Case

Example low-power configuration:

    mpu_sleep(MPU_SLEEP_ALL_OFF);
    mpu_stby(MPU_STBY_GYRO);
    mpu_cycle_mode(MPU_CYCLE_ON, MPU_LP_WAKE_5HZ);

Example meaning:

- device awake
- gyro disabled
- periodic low-power wake-up enabled
- wake-up rate set to 5 Hz

---

## Motion + Cycle Mode

Cycle mode is often paired with:

- accelerometer-only operation
- motion threshold detection
- interrupt-based wake-up

This section should later describe:

- recommended interrupt settings
- recommended threshold values
- expected latency
- current consumption tradeoffs

---

## Things to Verify Per Device

This section is a placeholder for later test notes.

### Verify on MPU60X0

- valid wake rates
- effect on data-ready behavior
- interrupt behavior in cycle mode

### Verify on MPU6500 / 925X

- LP accel ODR programming
- wake-on-motion behavior
- hardware WOM interaction
- accel intelligence interaction

---

## Practical Recommendations

### For simple low-power polling

- disable unnecessary axes
- use a moderate wake-up rate
- avoid very high wake-up rates unless needed

### For wake-on-motion

- combine cycle mode with motion interrupt configuration
- tune threshold and filter carefully
- verify actual trigger behavior on the target chip

---

## Recommended Future Additions

- register-level explanation of cycle mode bits
- per-chip truth table
- tested example setups
- current consumption notes
- wake latency measurements
