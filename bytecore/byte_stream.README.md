# byte_stream

A lightweight cursor-based byte stream helper for sequential read and write access.

This module provides a simple and safe abstraction for working with raw byte buffers without manually tracking indexes.

---

## Overview

`byte_stream` wraps a raw memory buffer and provides:

- sequential read/write access
- bounds-checked operations
- cursor management
- lookahead (peek)
- binary and text matching helpers for parsing

The stream does not allocate memory and does not own the buffer.

---

## Core concept

A byte stream is defined as:

```c
typedef struct {
    uint8_t *buf;
    size_t size;
    size_t pos;
} byte_stream_t;
```

- `buf`  → pointer to raw memory  
- `size` → total buffer size  
- `pos`  → current cursor position  

All operations are based on advancing or inspecting this cursor.

---

## Basic usage

### Initialize

```c
uint8_t buffer[32];

byte_stream_t s;
byte_stream_init(&s, buffer, sizeof(buffer));
```

---

### Write data

```c
byte_stream_write_u8(&s, 0xAA);
byte_stream_write_u16_le(&s, 0x1234);
```

---

### Read data

```c
uint8_t a;
uint16_t b;

byte_stream_read_u8(&s, &a);
byte_stream_read_u16_le(&s, &b);
```

---

## Cursor control

```c
byte_stream_seek(&s, 0);      // jump to position
byte_stream_skip(&s, 4);      // move forward
byte_stream_rewind(&s, 2);    // move backward
```

---

## State helpers

```c
size_t pos       = byte_stream_pos(&s);
size_t available = byte_stream_available(&s);

if (byte_stream_eof(&s)) {
    // reached end
}
```

---

## Peek (lookahead)

Peek allows inspecting data without moving the cursor.

```c
uint8_t v;

if (byte_stream_peek_u8(&s, &v)) {
    // v contains next byte, cursor unchanged
}
```

---

## Matching (parser helpers)

### Binary match

```c
if (byte_stream_match(&s, "\xAA\x55", 2)) {
    // match at current position
}
```

### Consume on match

```c
if (byte_stream_consume_if_match(&s, "\xAA\x55", 2)) {
    // matched and advanced by 2 bytes
}
```

---

### String match

```c
if (byte_stream_match_str(&s, "GET")) {
    ...
}
```

```c
byte_stream_consume_if_match_str(&s, "while");
```

---

### Match at arbitrary position (lookahead)

```c
if (byte_stream_match_str_at(&s, s.pos + 1, "else")) {
    ...
}
```

This allows inspecting future positions without modifying the stream state.

---

## Endianness helpers

The stream provides explicit endian-aware functions:

- `*_le` → little-endian  
- `*_be` → big-endian  

```c
byte_stream_write_u32_le(&s, value);
byte_stream_read_u16_be(&s, &value);
```

---

## Raw access

```c
uint8_t tmp[4];

byte_stream_read(&s, tmp, 4);
byte_stream_write(&s, tmp, 4);
```

---

## Design notes

- All functions are bounds-checked  
- No undefined behavior on overflow  
- No pointer arithmetic exposed  
- No dynamic allocation  
- Works with any buffer (stack, static, heap)  

---

## Philosophy

`byte_stream` is designed as:

> a safe cursor over raw memory

It provides structure without hiding how memory works.

---

## When to use

- parsing binary or text protocols  
- building structured byte buffers  
- avoiding manual index tracking  
- implementing parsers or tokenizers  

---

## When not to use

- heavy random access → use direct pointers  
- ultra-tight loops where every cycle matters  

---

## Example: simple parser

```c
if (byte_stream_consume_if_match_str(&s, "GET ")) {
    // parse HTTP GET
}
```

```c
if (byte_stream_match(&s, "\r\n", 2)) {
    // end of line
}
```

---

## Related modules

- `byte_ops.h` → bit and byte manipulation  
- `byte_cache.h` → multi-view byte buffers  

---

## Status

This module is intended to remain small, predictable, and stable.
