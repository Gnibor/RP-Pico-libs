# byte_cache

Fixed-size multi-view byte cache types for low-level C development.

This module provides small reusable cache/container types that expose the same memory through multiple views, making it easier to work with raw bytes, structured payloads, and bit-level inspection without extra copies.

---

## Overview

`byte_cache` is designed for small internal buffers that need multiple interpretations of the same memory.

It allows you to treat the same memory as:

- raw byte buffer
- structured header + payload
- per-byte bit access
- optional word/dword view

All views reference the same memory — no copies.

---

## Core idea

Instead of writing code like:

```c
uint8_t buf[16];

buf[0] = cmd;
buf[1] = data0;
buf[2] = data1;
```

You can structure it:

```c
byte_cache16_t c = {0};

c.tx.head = cmd;
c.tx.data[0] = data0;
c.tx.data[1] = data1;
```

Same memory — clearer intent.

---

## Real usage examples

### 1. Build a command + payload buffer

```c
byte_cache16_t c = {0};

c.tx.head = 0xA1;       // command
c.tx.data[0] = 0x10;
c.tx.data[1] = 0x20;

// send raw buffer
send_bytes(c.raw, 3);
```

👉 Vorteil:
- klar strukturierter Zugriff
- trotzdem direkt als raw sendbar

---

### 2. Parse incoming data

```c
byte_cache16_t c = {0};

read_bytes(c.raw, 3);

uint8_t cmd = c.tx.head;

if (cmd == 0xA1) {
    uint8_t a = c.tx.data[0];
    uint8_t b = c.tx.data[1];
}
```

👉 kein manuelles Index-Gefrickel

---

### 3. Debugging / Bit inspection

```c
byte_cache16_t c = {0};

c.raw[0] = 0b10110001;

if (c.bits.head.bit.b0) {
    // flag set
}

if (c.bits.head.bit.b7) {
    // highest bit set
}
```

👉 super praktisch beim Debuggen von Flags

---

### 4. Modify specific bits quickly

```c
byte_cache16_t c = {0};

c.raw[0] = 0x00;

// enable flag at bit 3
c.bits.head.bit.b3 = 1;

// disable flag at bit 0
c.bits.head.bit.b0 = 0;
```

👉 schnell, ohne Masken schreiben zu müssen

---

### 5. Reinterpret as words (optional)

```c
byte_cache16_t c = {0};

c.raw[0] = 0x34;
c.raw[1] = 0x12;

uint16_t v = c.u16[0];  // often 0x1234 on little-endian
```

👉 praktisch für:
- Debug
- schnelle Checks
- interne Verarbeitung

---

## Why this is useful

Without `byte_cache`:

```c
buf[0]
buf[1]
buf[2]
```

With `byte_cache`:

```c
c.tx.head
c.tx.data[0]
```

👉 gleiche Performance, bessere Lesbarkeit

---

## Important notes

### Bitfields

Bitfield layout is compiler-dependent.

Use:

- `bits.*` → debugging / convenience
- `byte_ops.h` → real bit manipulation

---

### Endianness

`u16[]` and `u32[]` depend on system endianness.

Use them only when:
- you know what you're doing
- or for debugging

---

## Philosophy

`byte_cache` is:

> one buffer, multiple meanings

It helps you write cleaner low-level code without hiding what the memory actually is.

---

## When to use

- small fixed-size buffers
- command + payload structures
- parsing binary data
- debugging byte streams
- internal driver caches

---

## When not to use

- dynamic buffers
- strict cross-platform serialized layouts
- large data processing

---

## Related modules

- `byte_ops.h` → bit manipulation
- `byte_stream.h` → parsing and streaming

---

## Status

Stable core functionality.

Future changes will keep the design simple and predictable.
