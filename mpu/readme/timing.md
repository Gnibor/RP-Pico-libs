# Timing and Filters

This file documents clock selection, sample timing, low-pass filtering, and related timing behavior of the MPU driver.

---

## Overview

Relevant public functions:

- `mpu_clk_sel(mpu_clk_sel_t clksel)`
- `mpu_dlpf_cfg(mpu_dlpf_cfg_t cfg)`
- `mpu_smplrt_div(mpu_smplrt_div_t smplrt_div)`
- `mpu_ahpf(mpu_ahpf_t ahpf)`

Relevant enums:

- `mpu_clk_sel_t`
- `mpu_dlpf_cfg_t`
- `mpu_smplrt_div_t`
- `mpu_ahpf_t`
- `mpu_ext_sync_set_t`

---

## Clock Source

Use `mpu_clk_sel()` to select the internal or external timing reference.

Available options:

- `MPU_CLK_INTERNAL`
- `MPU_CLK_XGYRO`
- `MPU_CLK_YGYRO`
- `MPU_CLK_ZGYRO`
- `MPU_CLK_EXT32KHZ`
- `MPU_CLK_EXT19MHZ`
- `MPU_CLK_STOP`

### Recommendation

In normal operation, gyro PLL clock is usually preferred over the internal oscillator.

Typical choice:

    mpu_clk_sel(MPU_CLK_XGYRO);

---

## Digital Low-Pass Filter (DLPF)

Use `mpu_dlpf_cfg()` to set the digital low-pass filter bandwidth.

Available options:

- `MPU_DLPF_CFG_260HZ`
- `MPU_DLPF_CFG_184HZ`
- `MPU_DLPF_CFG_94HZ`
- `MPU_DLPF_CFG_44HZ`
- `MPU_DLPF_CFG_21HZ`
- `MPU_DLPF_CFG_10HZ`
- `MPU_DLPF_CFG_5HZ`
- `MPU_DLPF_CFG_3600HZ`

### Tradeoff

Higher bandwidth:

- less delay
- more noise
- better for fast motion

Lower bandwidth:

- more delay
- less noise
- better for stable measurements

---

## Sample Rate Divider

Use `mpu_smplrt_div()` to control effective output rate.

Available options:

- `MPU_SMPLRT_8KHZ`
- `MPU_SMPLRT_1KHZ`
- `MPU_SMPLRT_500HZ`
- `MPU_SMPLRT_200HZ`
- `MPU_SMPLRT_100HZ`

### Important note

Actual sample timing depends on:

- selected DLPF mode
- internal gyro output rate
- divider value

This section should later explain the exact formula per device family.

---

## High-Pass Filter (AHPF)

Use `mpu_ahpf()` to configure the accelerometer high-pass filter.

Available options:

- `MPU_AHPF_RESET`
- `MPU_AHPF_5HZ`
- `MPU_AHPF_2_5HZ`
- `MPU_AHPF_1_25HZ`
- `MPU_AHPF_0_63HZ`
- `MPU_AHPF_HOLD`

### Typical use

AHPF is relevant for:

- motion detection
- removing static gravity offset from some detection paths
- threshold-based event logic

---

## External Sync

Relevant enum:

- `mpu_ext_sync_set_t`

Possible sources:

- temperature output
- gyro axes
- accel axes
- disabled

This section can later explain:

- FSYNC behavior
- when external sync is useful
- how sync affects sampled data bits

---

## Typical Timing Setup

Example:

    mpu_reset(MPU_RESET_ALL);
    mpu_sleep(MPU_SLEEP_ALL_OFF);
    mpu_clk_sel(MPU_CLK_XGYRO);
    mpu_dlpf_cfg(MPU_DLPF_CFG_44HZ);
    mpu_smplrt_div(MPU_SMPLRT_1KHZ);

This is a good baseline for many general-purpose motion applications.

---

## Interaction Between Filter and Sample Rate

| DLPF_CFG | Gyro BW | Accel BW | Delay (ms) | Internal Rate | Notes                          |
|----------|---------|----------|------------|---------------|--------------------------------|
| 0        | 250 Hz  | 260 Hz   | ~0.97      | 8 kHz         | No/weak filtering, very fast   |
| 1        | 184 Hz  | 184 Hz   | ~2.0       | 1 kHz         | Good general purpose           |
| 2        | 92 Hz   | 94 Hz    | ~3.0       | 1 kHz         | Balanced noise/latency         |
| 3        | 41 Hz   | 44 Hz    | ~5.0       | 1 kHz         | Stable, less noise             |
| 4        | 20 Hz   | 21 Hz    | ~8.5       | 1 kHz         | Slow but clean                 |
| 5        | 10 Hz   | 10 Hz    | ~13.8      | 1 kHz         | Very smooth                    |
| 6        | 5 Hz    | 5 Hz     | ~19.0      | 1 kHz         | Extreme filtering              |

### Internal Sample Rate Behavior

- DLPF_CFG = 0 → internal gyro rate = 8 kHz  
- DLPF_CFG ≥ 1 → internal rate = 1 kHz  

Effective sample rate:

```
Sample Rate = Internal Rate / (1 + SMPLRT_DIV)
```

### Examples

```
DLPF = 0
SMPLRT_DIV = 7

→ 8000 / (1 + 7) = 1000 Hz
```

```
DLPF = 2
SMPLRT_DIV = 0

→ 1000 / (1 + 0) = 1000 Hz
```

### Notes

- Lower bandwidth → less noise, more delay  
- Higher bandwidth → faster response, more noise  

- DLPF = 0 is a special high-speed mode (8 kHz gyro only)  
- Accelerometer always runs at 1 kHz internally  
- Data Ready interrupt frequency follows the configured sample rate  
- Very high rates (e.g. 8 kHz) may overload I²C or CPU

---

## Practical Recommendations

### Fast motion / short latency

- use PLL clock
- use higher DLPF bandwidth
- use higher sample rate

### Stable measurement / low noise

- use PLL clock
- use lower DLPF bandwidth
- reduce sample rate if application allows

### Interrupt-based readout

- match data-ready frequency to your processing loop
- avoid setting sample rates higher than your application can consume

---

## Device Family Differences

This section summarizes key behavioral differences between MPU device families.

| Feature                  | MPU60X0 (6050/6000)        | MPU6500                  | MPU9250 / 9255              |
|--------------------------|----------------------------|--------------------------|-----------------------------|
| Process / Noise          | Older, higher noise        | Improved, lower noise    | Same as MPU6500             |
| Gyro / Accel             | Yes                        | Yes                      | Yes                         |
| Magnetometer             | No                         | No                       | Yes (AK8963 internal)       |
| Internal Rate (gyro)     | 8 kHz / 1 kHz (DLPF dep.)  | Same                     | Same                        |
| Accel-only mode          | Limited                    | Full support             | Full support                |
| Low-power accel mode     | Basic                      | Advanced (LP accel)      | Advanced (LP accel)         |
| Motion detection         | Basic                      | Improved (LP + WOM)      | Improved (LP + WOM)         |
| Wake-on-motion (WOM)     | No                         | Yes                      | Yes                         |
| FIFO behavior            | Basic                      | More stable              | More stable                 |
| Power consumption        | Higher                     | Lower                    | Lower                       |


### Low-Power Timing

- **MPU60X0**
  - No true low-power accel mode  
  - Uses normal sampling + sleep cycles  

- **MPU6500 / 9250**
  - Dedicated low-power accel mode  
  - Configurable wake-up frequencies:
    - ~0.24 Hz, 0.49 Hz, 0.98 Hz, 1.95 Hz, 3.91 Hz, 7.81 Hz, 15.63 Hz, 31.25 Hz, 62.5 Hz, 125 Hz, 250 Hz, 500 Hz  
  - Internal duty cycling (sensor sleeps between samples)

### Motion Detection

- **MPU60X0**
  - Motion threshold + duration  
  - Always tied to full-power operation  

- **MPU6500 / 9250**
  - Hardware motion detection in low-power mode  
  - Wake-on-Motion (WOM):
    - triggers interrupt without full sensor running  
    - uses accel-only path  
  - Lower power + faster reaction for embedded use  

### Wake-Up Timing

- **MPU60X0**
  - Wake-up requires full sensor startup  
  - Slower response (~tens of ms depending on config)

- **MPU6500 / 9250**
  - Fast wake via WOM interrupt  
  - Sensor can remain mostly off until motion detected  
  - Much lower latency for event-driven systems  

### Accel-Only Operating Mode

- **MPU60X0**
  - Gyro can be disabled, but no true optimized accel-only pipeline  

- **MPU6500 / 9250**
  - Dedicated accel-only low-power mode  
  - Gyro fully powered down  
  - Reduced noise path for motion detection  
  - Used for:
    - step detection  
    - tilt detection  
    - wake triggers  

### Practical Implications

- Use **MPU60X0** for:
  - simple projects
  - continuous sampling
  - no strict power constraints  

- Use **MPU6500 / 9250** for:
  - low-power systems
  - interrupt-driven designs
  - motion-triggered wake-up
  - battery-powered devices  

- Use **MPU9250 / 9255** when:
  - magnetometer (compass) is required  
  - 9-DOF sensor fusion is needed  

---

## Timing, Filtering and Sample Rate

The timing behavior of the MPU is mainly controlled by two configuration functions:

- `mpu_dlpf_cfg(mpu_dlpf_cfg_t cfg)`
- `mpu_smplrt_div(mpu_smplrt_div_t smplrt_div)`

These two settings must always be considered together.

`mpu_dlpf_cfg(...)` does not only select the digital low-pass filter bandwidth. It also changes the internal timing base used by the sensor. This means it affects:

- signal bandwidth
- signal delay
- noise level
- internal gyro sample clock
- effective output sample rate
- interrupt timing behavior

`mpu_smplrt_div(...)` then divides that internal base rate down to the final output rate.

In practice, this means that sample rate is never defined by `mpu_smplrt_div_t` alone. The real output rate always depends on the combination of:

```text
effective_rate = f(dlpf_cfg, smplrt_div)
```

---

### DLPF Characteristics

The selected DLPF mode determines the tradeoff between bandwidth, delay and internal gyro rate.

| `mpu_dlpf_cfg_t`        | Gyro BW | Delay (ms) | Internal Gyro Rate | Notes                              |
|-------------------------|---------|------------|--------------------|------------------------------------|
| `MPU_DLPF_CFG_260HZ`    | 260 Hz  | 0.6 ms     | 8 kHz              | fastest filtered mode              |
| `MPU_DLPF_CFG_184HZ`    | 184 Hz  | 2.0 ms     | 1 kHz              | good low-latency default           |
| `MPU_DLPF_CFG_94HZ`     | 94 Hz   | 3.0 ms     | 1 kHz              | balanced                           |
| `MPU_DLPF_CFG_44HZ`     | 44 Hz   | 4.9 ms     | 1 kHz              | smoother                           |
| `MPU_DLPF_CFG_21HZ`     | 21 Hz   | 8.5 ms     | 1 kHz              | strong smoothing                   |
| `MPU_DLPF_CFG_10HZ`     | 10 Hz   | 13.8 ms    | 1 kHz              | high delay                         |
| `MPU_DLPF_CFG_5HZ`      | 5 Hz    | 19.0 ms    | 1 kHz              | maximum smoothing                  |
| `MPU_DLPF_CFG_3600HZ`   | 3600 Hz | ~0 ms      | 8 kHz              | unfiltered gyro path               |

Lower bandwidth reduces noise, but increases signal delay.  
Higher bandwidth gives faster response, but also passes more noise and vibration.

This matters directly for:

- control loop stability
- motion detection timing
- interrupt frequency
- perceived responsiveness
- software fusion quality

As a rough rule of thumb:

| Delay Range | Meaning                         |
|-------------|---------------------------------|
| < 1 ms      | very fast, minimal filtering    |
| 2–5 ms      | good control / tracking         |
| 8–19 ms     | strong smoothing, higher lag    |

For fast control systems, low delay is usually more important than absolute smoothness.  
For logging or slow event detection, stronger filtering is often more useful than fast response.

---

### Internal Base Rate

The internal sample base depends on the selected DLPF mode.

| DLPF Mode                | Internal Gyro Rate | Internal Accel Rate |
|--------------------------|--------------------|---------------------|
| `*_260HZ`, `*_3600HZ`    | 8 kHz              | 1 kHz               |
| all others               | 1 kHz              | 1 kHz               |

This is the key detail that often causes confusion.

Even though the divider register is always configured through `mpu_smplrt_div(...)`, the result changes depending on the active DLPF mode. Two configurations using the same divider can produce completely different output rates if the internal base clock is different.

---

### Sample Rate Formula

The effective sample rate is calculated as:

```c
sample_rate = internal_rate / (1 + smplrt_div)
```

Where:

- `internal_rate` is either `8000` or `1000`
- `smplrt_div` is the raw divider written to `MPU_REG_SMPLRT_DIV`

The `mpu_smplrt_div_t` enum provides predefined divider values:

| Enum                  | Raw Divider | Rate @1 kHz base | Rate @8 kHz base |
|-----------------------|-------------|------------------|------------------|
| `MPU_SMPLRT_8KHZ`     | `0x00`      | 1000 Hz          | 8000 Hz          |
| `MPU_SMPLRT_1KHZ`     | `0x07`      | 125 Hz           | 1000 Hz          |
| `MPU_SMPLRT_500HZ`    | `0x0E`      | ~66.7 Hz         | ~533.3 Hz        |
| `MPU_SMPLRT_200HZ`    | `0x27`      | 25 Hz            | 200 Hz           |
| `MPU_SMPLRT_100HZ`    | `0x5C`      | ~10.6 Hz         | ~84.2 Hz         |

This means the enum names should be understood as intended presets, not as absolute guaranteed output rates in every filter mode.

Example:

- `MPU_SMPLRT_1KHZ`
  - with 8 kHz internal base → 1000 Hz
  - with 1 kHz internal base → 125 Hz

So the following is wrong:

```text
MPU_SMPLRT_1KHZ == always 1000 Hz
```

The correct view is:

```text
MPU_SMPLRT_1KHZ == divider preset 0x07
```

and the final output rate depends on the selected DLPF mode.

---

### DLPF Selection Guide

Use `mpu_dlpf_cfg(...)` to choose the tradeoff between:

- noise
- latency
- bandwidth
- interrupt behavior

In most applications, DLPF selection should be done first.  
Only after that should the output rate be adjusted with `mpu_smplrt_div(...)`.

Recommended starting points:

| Use Case                   | Recommended `mpu_dlpf_cfg_t`       | Why                                      |
|----------------------------|------------------------------------|------------------------------------------|
| balancing robot            | `MPU_DLPF_CFG_184HZ`               | low delay, still filtered                |
| fast PID loop              | `MPU_DLPF_CFG_184HZ` or `MPU_DLPF_CFG_260HZ` | fast response                    |
| gesture sensing            | `MPU_DLPF_CFG_94HZ` or `MPU_DLPF_CFG_44HZ`  | smoother motion data             |
| general-purpose IMU        | `MPU_DLPF_CFG_94HZ`                | balanced default                         |
| slow logging               | `MPU_DLPF_CFG_21HZ` or `MPU_DLPF_CFG_10HZ`  | low noise, delay uncritical      |
| wake-up / motion trigger   | `MPU_DLPF_CFG_44HZ` or `MPU_DLPF_CFG_21HZ`  | suppresses spurious triggers     |
| vibration capture          | `MPU_DLPF_CFG_3600HZ`              | maximum bandwidth                        |

Quick selection rules:

- choose `MPU_DLPF_CFG_260HZ`
  - when minimum delay matters most
  - and more noise is acceptable

- choose `MPU_DLPF_CFG_184HZ`
  - for most control applications
  - usually the best first test configuration

- choose `MPU_DLPF_CFG_94HZ`
  - for a balanced default setup
  - useful for readout, calibration and fusion

- choose `MPU_DLPF_CFG_44HZ` or lower
  - when stable, smooth output matters more than fast response

- choose `MPU_DLPF_CFG_3600HZ`
  - only when the unfiltered high-bandwidth gyro path is explicitly needed

Because DLPF also changes the internal timing base, filter selection affects both:

- output quality
- effective sample rate

So DLPF is not just a signal-quality setting. It is also part of timing configuration.

---

### Practical Implications

In real applications, filter and sample-rate setup influence much more than raw sensor values.

A higher output rate increases:

- interrupt frequency
- bus traffic
- CPU load
- FIFO pressure

A lower bandwidth increases:

- signal smoothness
- stability for slow measurements

but also increases:

- phase lag
- control-loop latency
- motion-to-interrupt delay

This is why the "best" filter setting depends heavily on the application.

For example:

- a balancing robot needs low latency first
- a logger often prefers smoother data
- a wake-up detector benefits from reduced noise to avoid false triggers
- vibration analysis may require the widest possible bandwidth even if the signal becomes noisy

---

### Practical Examples

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_3600HZ);
mpu_smplrt_div(MPU_SMPLRT_1KHZ);
```

Result:

```text
internal_rate = 8000 Hz
sample_rate   = 1000 Hz
```

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_94HZ);
mpu_smplrt_div(MPU_SMPLRT_1KHZ);
```

Result:

```text
internal_rate = 1000 Hz
sample_rate   = 125 Hz
```

These two examples use the same divider preset, but produce different output rates because the DLPF mode changes the internal base clock.

---

### Key Takeaway

`mpu_dlpf_cfg(...)` controls:

- bandwidth
- delay
- noise behavior
- internal timing base

`mpu_smplrt_div(...)` divides that timing base into the final output rate.

Both settings always belong together and should never be documented or tuned independently.

---

## Filter Setup Examples

The following examples use the public API:

- `mpu_dlpf_cfg(...)`
- `mpu_smplrt_div(...)`
- `mpu_cycle_mode(...)`
- `mpu_int_enable(...)`
- `mpu_int_motion_cfg(...)`

### Balancing Robot

Goal:

- low delay
- stable gyro feedback
- high control loop rate

Suggested setup:

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_184HZ);
mpu_smplrt_div(MPU_SMPLRT_1KHZ);
```

Why:

- `MPU_DLPF_CFG_184HZ` gives good latency
- with 8 kHz base mode this divider gives 1 kHz output
- good starting point for complementary or PID-based balancing

Notes:

- if noise is too high, try `MPU_DLPF_CFG_94HZ`
- if timing is too slow, use a smaller divider value than `MPU_SMPLRT_1KHZ`

### Gesture Sensing

Goal:

- smoother motion curves
- less spike noise
- moderate output rate

Suggested setup:

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_94HZ);
mpu_smplrt_div(MPU_SMPLRT_100HZ);
```

Why:

- gestures usually do not require very high bandwidth
- stronger filtering improves recognition quality
- 100 Hz class output is often sufficient

Notes:

- `MPU_SMPLRT_100HZ` only gives exactly that rate in the matching base-rate mode
- always verify the real resulting output rate for the active DLPF mode

### Data Logging

Goal:

- clean sensor data
- stable long-term capture
- low interrupt pressure

Suggested setup:

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_21HZ);
mpu_smplrt_div(MPU_SMPLRT_100HZ);
```

Why:

- low noise is more important than low latency
- lower rate reduces bus load
- good for plotting, offline analysis, and storage

Notes:

- for very smooth logs, try `MPU_DLPF_CFG_10HZ`
- for more detail, move up to `MPU_DLPF_CFG_44HZ`

### Wake-Up / Motion Interrupt Applications

Goal:

- low-power waiting
- motion-triggered wake-up
- fewer false interrupts

#### MPU6000 / MPU6050 Style

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_44HZ);
mpu_int_motion_cfg(ms, mg);
mpu_int_enable(MPU_INT_MOTION_EN);
mpu_cycle_mode(MPU_CYCLE_ON, MPU_LP_WAKE_5HZ);
```

Why:

- moderate filtering helps reject small spikes
- low-power cycle mode periodically checks for motion
- `MPU_LP_WAKE_5HZ` is a reasonable low-power default

#### MPU6500 / MPU9250 / MPU9255 Style

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_44HZ);
mpu_int_motion_cfg(ms, mg);
mpu_int_enable(MPU_INT_MOTION_EN);
mpu_cycle_mode(MPU_CYCLE_LP, MPU6500_LP_WAKE_3_91HZ);
```

Why:

- modern devices support dedicated low-power accel wake rates
- lower wake-up ODR reduces power consumption
- useful for battery systems and event-triggered designs

Notes:

- increase wake frequency for faster reaction
- decrease wake frequency for lower power
- threshold tuning is usually more important than exact DLPF tuning here

### High-Frequency Motion / Vibration Capture

Goal:

- maximum gyro bandwidth
- minimum filtering
- highest useful output rate

Suggested setup:

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_3600HZ);
mpu_smplrt_div(MPU_SMPLRT_8KHZ);
```

Why:

- unfiltered gyro path
- maximum internal gyro rate
- useful for vibration or fast transient analysis

Notes:

- this mode is noisy
- bus load and interrupt rate can become very high
- usually not suitable for normal control applications

### General-Purpose Default

If no application-specific tuning is known yet, start with:

```c
mpu_dlpf_cfg(MPU_DLPF_CFG_94HZ);
mpu_smplrt_div(MPU_SMPLRT_1KHZ);
```

This gives a good baseline for:

- driver bring-up
- debugging
- calibration
- initial sensor fusion experiments

---

## Recommended Future Additions

- exact sample rate formulas
- effective delay table
- DLPF selection guide
- filter setup examples for balancing robots, gesture sensing, logging, and wake-up applications
