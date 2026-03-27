# bytecore

A lightweight collection of reusable byte-oriented utilities for low-level C development.

`bytecore` is a small header-focused library built around raw byte access, bit manipulation, byte streams, and compact helper primitives for binary data handling.

It is designed for projects that need direct, explicit, and efficient control over memory and byte-level data without unnecessary abstraction.

---

## Features

- header-only style modules
- byte- and bit-oriented helpers
- stream-style parsing and writing
- endian-safe load/store helpers
- fixed-size cache/view types
- suitable for drivers, parsers, binary protocols, and embedded projects

---

## Design goals

- keep the API small and explicit
- avoid hidden allocation and runtime overhead
- make raw memory handling easier to read and reuse
- stay useful for both embedded and general low-level C projects

---

## Modules

### Core modules

- [`byte_ops.h`](./byte_ops.h)  
  Low-level bit, byte, field, swap, load/store, and packing helpers.  
  Documentation: [`byte_ops.README.md`](./byte_ops.README.md)

- [`byte_stream.h`](./byte_stream.h)  
  Cursor-based byte stream helper for sequential read/write access and parser-style lookahead.  
  Documentation: [`byte_stream.README.md`](./byte_stream.README.md)

- [`byte_cache.h`](./byte_cache.h)  
  Fixed-size multi-view byte cache types for raw access, structured access, and debugging convenience.  
  Documentation: [`byte_cache.README.md`](./byte_cache.README.md)

---

## Intended use cases

- binary protocol parsing
- low-level driver development
- register and bitfield manipulation
- building and decoding raw byte frames
- embedded systems
- internal utility libraries used as submodules

---

## Repository layout

```text
bytecore/
├── README.md
├── byte_ops.h
├── byte_ops.README.md
├── byte_stream.h
├── byte_stream.README.md
├── byte_cache.h
└── byte_cache.README.md
