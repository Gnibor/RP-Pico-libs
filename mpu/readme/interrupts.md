# Interrupts

This file documents interrupt-related configuration and usage of the MPU driver.

---

## Overview

The driver provides helpers for:

- interrupt pin behavior
- interrupt source enable bits
- motion interrupt configuration
- interrupt status readout

Relevant public functions:

- `mpu_int_pin_cfg(mpu_int_pin_cfg_t cfg)`
- `mpu_int_enable(mpu_int_enable_t enable)`
- `mpu_int_motion_cfg(uint8_t ms, uint16_t mg)`
- `mpu_int_status(void)`

Relevant internal function:

- `_mpu_irq_handler(uint gpio, uint32_t events)`

---

## Interrupt Pin Configuration

Use `mpu_int_pin_cfg()` to configure the electrical and logical behavior of the INT pin.

Relevant flags from `mpu_int_pin_cfg_t`:

- `MPU_I2C_BYPASS_EN`
- `MPU_FSYNC_INT_EN`
- `MPU_FSYNC_INT_LEVEL`
- `MPU_INT_RD_CLEAR`
- `MPU_LATCH_INT_EN`
- `MPU_INT_OPEN_DRAIN`
- `MPU_INT_LEVEL_LOW`
- `MPU_INT_PIN_CFG_ALL`

### Typical options

#### Active high vs active low

- default is usually active high
- use `MPU_INT_LEVEL_LOW` if your board expects active-low signaling

#### Push-pull vs open-drain

- default is usually push-pull
- use `MPU_INT_OPEN_DRAIN` if the signal is shared or externally pulled up

#### Latched vs pulse interrupt

- use `MPU_LATCH_INT_EN` to hold the interrupt until status is read
- without latch, the signal behaves more like a short pulse

#### Clear-on-read behavior

- use `MPU_INT_RD_CLEAR` if reading interrupt-related registers should clear the event flags

---

## Interrupt Source Enable

Use `mpu_int_enable()` to enable selected interrupt sources.

Relevant flags from `mpu_int_enable_t`:

- `MPU_DATA_RDY_EN`
- `MPU_I2C_MST_INT_EN`
- `MPU_FIFO_OFLOW_EN`
- `MPU_INT_MOTION_EN`
- `MPU_INT_ENABLE_ALL`

### Common interrupt types

#### Data ready

`MPU_DATA_RDY_EN`

Triggers when a new sensor sample is ready.

#### I2C master interrupt

`MPU_I2C_MST_INT_EN`

Triggers when an internal auxiliary I2C master transaction finishes.

#### FIFO overflow

`MPU_FIFO_OFLOW_EN`

Triggers when FIFO data is lost due to overflow.

#### Motion interrupt

`MPU_INT_MOTION_EN`

Triggers when motion detection logic fires.

---

## Motion Interrupt Configuration

Use `mpu_int_motion_cfg(uint8_t ms, uint16_t mg)` to configure motion detection.

### Parameters

- `ms`
  Motion duration / debounce style timing parameter

- `mg`
  Motion threshold in mg

### Related enums / registers

- `mpu_mot_count_t`
- `mpu_mot_delay_t`
- `MPU_REG_MOT_THR`
- `MPU_REG_MOT_DUR`
- `MPU_REG_MOT_DETECT_CTRL`

### Notes

- Motion detection depends strongly on filter settings and sample rate.
- Legacy MPU60X0 and newer MPU6500/925X families differ in low-power motion behavior.
- False triggers are common if thresholds are too low or filtering is too weak.

---

## Interrupt Status

Use `mpu_int_status()` to read and evaluate the current interrupt state.

Relevant status bits:

- `MPU_DATA_RDY_INT`
- `MPU_I2C_MST_INT`
- `MPU_FIFO_OFLOW_INT`
- `MPU_MOTION_INT`

### Typical flow

1. Configure INT pin behavior
2. Enable needed sources
3. Wait for GPIO interrupt or poll status
4. Read `mpu_int_status()`
5. Read sensor data or handle the event
6. Clear interrupt flags depending on pin/status mode

---

## Example: Data Ready Interrupt

    mpu_int_pin_cfg(MPU_LATCH_INT_EN | MPU_INT_RD_CLEAR);
    mpu_int_enable(MPU_DATA_RDY_EN);

Typical behavior:

- INT goes active when new data is ready
- application reads status or sensor values
- interrupt clears on read if configured that way

---

## Example: Motion Interrupt

    mpu_int_pin_cfg(MPU_LATCH_INT_EN | MPU_INT_RD_CLEAR);
    mpu_int_motion_cfg(40, 100);
    mpu_int_enable(MPU_INT_MOTION_EN);

Typical behavior:

- device monitors motion threshold internally
- interrupt fires on sufficient movement
- useful for wake-up logic or low-power monitoring

---

## GPIO / Pico Side

Typical board-related defines:

- `MPU_INT_PIN`
- `MPU_INT_PULLUP`

Things to document here later:

- GPIO init
- edge polarity
- pull-up / pull-down setup
- ISR registration
- interaction with `_mpu_irq_handler()`

---

## Internal Notes

Private implementation currently contains:

- `_mpu_irq_handler(uint gpio, uint32_t events)`

This section can later document:

- IRQ acknowledgment flow
- global state updates
- deferred processing
- polling vs ISR-driven usage

---

## Recommended Future Additions

- full INT pin truth table
- latch vs pulse behavior examples
- motion threshold tuning guide
- differences between MPU6050 and MPU6500 motion logic
- example ISR integration for Raspberry Pi Pico
