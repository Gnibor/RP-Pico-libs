# byte_ops

Low-level helpers for byte and bit manipulation.

This module provides small, reusable primitives for working directly with raw byte data, focusing on clarity, performance, and explicit control.

---

## Overview

`byte_ops` contains:

- single-bit operations
- multi-bit field extraction and insertion
- byte composition and decomposition helpers
- endian-safe load/store helpers
- byte swap utilities

All functions are header-only (`static inline`) and have no dependencies beyond the standard integer types.

---

## Design goals

- no abstraction overhead
- explicit and predictable behavior
- suitable for embedded and low-level systems
- works directly on raw memory
- easy to combine with `byte_stream` and `byte_cache`

---

## Bit operations

### Test / set / clear

```c
uint8_t v = 0;

v = byte_set_bit(v, 3);       // set bit 3
v = byte_clear_bit(v, 3);     // clear bit 3

if (byte_test_bit(v, 3)) {
    ...
}
```

---

### Toggle and write

```c
v = byte_toggle_bit(v, 2);         // flip bit
v = byte_write_bit(v, 1, true);   // set bit to 1
v = byte_write_bit(v, 1, false);  // set bit to 0
```

---

## Multi-bit fields

Useful for registers and packed data.

### Extract bits

```c
uint8_t v = 0b10110100;

uint8_t field = byte_get_bits(v, 2, 3);  // -> 0b101
```

---

### Set bits

```c
uint8_t v = 0;

v = byte_set_bits(v, 2, 3, 0b101);  // -> 0b00010100
```

---

## Combine / split helpers

### Combine bytes

```c
uint16_t v = byte_make_u16_le(0x34, 0x12);  // -> 0x1234
```

```c
uint16_t v = byte_make_u16_be(0x12, 0x34);  // -> 0x1234
```

---

### Split values

```c
uint16_t v = 0x1234;

uint8_t lo = byte_lo_u16(v);  // 0x34
uint8_t hi = byte_hi_u16(v);  // 0x12
```

---

## Byte swap (endianness conversion)

```c
uint16_t a = byte_swap16(0x1234);      // -> 0x3412
uint32_t b = byte_swap32(0x12345678);  // -> 0x78563412
```

---

## Raw loads (unaligned safe)

Load values from byte buffers without alignment requirements.

### Little-endian

```c
uint8_t data[] = {0x34, 0x12};

uint16_t v = byte_load_u16_le(data);  // -> 0x1234
```

---

### Big-endian

```c
uint8_t data[] = {0x12, 0x34};

uint16_t v = byte_load_u16_be(data);  // -> 0x1234
```

---

## Raw stores

Write values into byte buffers.

### Little-endian

```c
uint8_t buf[2];

byte_store_u16_le(buf, 0x1234);
// buf = {0x34, 0x12}
```

---

### Big-endian

```c
uint8_t buf[2];

byte_store_u16_be(buf, 0x1234);
// buf = {0x12, 0x34}
```

---

## Typical use cases

- hardware register manipulation
- binary protocol parsing
- packing/unpacking structures
- bitfield encoding/decoding
- working with raw byte buffers

---

## Design notes

- no bounds checks are performed (caller responsibility)
- no hidden masking or clamping
- assumes valid input ranges (e.g. bit < 8)
- optimized for clarity and inline performance

---

## Philosophy

`byte_ops` is designed as:

> small, explicit building blocks for byte-level logic

It does not try to hide how bits work — it makes them easier to use correctly.

---

## Related modules

- `byte_stream.h` → sequential access and parsing
- `byte_cache.h` → structured multi-view buffers

---

## Status

Stable core functionality.

Additional helpers may be added if they:
- are broadly useful
- do not complicate the API
- remain consistent with the byte-oriented design
