#ifndef BYTE_CACHE_H
#define BYTE_CACHE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * byte_cache.h
 * ----------------------------------------------------------------------------
 * Generic internal byte cache helpers.
 *
 * Purpose
 * -------
 * This header provides small reusable cache/container types that expose the
 * same memory through multiple views:
 *
 * - raw byte array access
 * - header/payload access
 * - per-byte bit inspection access
 * - optional 16-bit / 32-bit word access
 *
 * Typical use cases
 * -----------------
 * - preparing small binary messages
 * - reusing a fixed internal scratch buffer
 * - inspecting individual bits while debugging
 * - interpreting the same bytes as words/dwords when convenient
 *
 * Design notes
 * ------------
 * - These types are intended for internal/private use.
 * - Bitfield layout is compiler-dependent.
 * - For actual bit manipulation, prefer `.byte` with explicit bit masks.
 * - The `.bit.bX` fields are mainly for convenience, debugging, and quick
 *   inspection.
 * - Multi-byte views (`u16[]`, `u32[]`) depend on platform endianness and
 *   alignment expectations of the target environment.
 * ============================================================================ */


/* ============================================================================
 * byte_u8_t
 * ----------------------------------------------------------------------------
 * Single-byte helper with two views:
 *
 * - `.byte` : raw 8-bit value
 * - `.bit`  : optional bitfield access
 *
 * Example:
 *   byte_u8_t v;
 *   v.byte = 0xA5;
 *
 *   if (v.bit.b0) {
 *       ...
 *   }
 *
 * Recommended usage:
 *   - use `v.byte` for normal reads/writes
 *   - use masks for portable register/flag handling
 *   - use `v.bit.bX` only as a convenience/debug view
 * ============================================================================ */
typedef union {
    uint8_t byte;

    struct {
        uint8_t b0 : 1;
        uint8_t b1 : 1;
        uint8_t b2 : 1;
        uint8_t b3 : 1;
        uint8_t b4 : 1;
        uint8_t b5 : 1;
        uint8_t b6 : 1;
        uint8_t b7 : 1;
    } bit;
} byte_u8_t;


/* ============================================================================
 * byte_cache8_t
 * ----------------------------------------------------------------------------
 * Fixed-size 8-byte cache with multiple views on the same memory.
 *
 * Layout:
 *   raw[0]     = header / command / tag / first byte
 *   raw[1..7]  = payload / data
 *
 * Views:
 * - raw[]      : direct byte access
 * - tx.head    : first byte
 * - tx.data[]  : remaining payload bytes
 * - bits.head  : bit inspection view of first byte
 * - bits.data  : bit inspection view of payload bytes
 * - u16[]      : optional 16-bit interpretation
 * - u32[]      : optional 32-bit interpretation
 * ============================================================================ */
typedef union {
    uint8_t raw[8];

    struct {
        uint8_t head;
        uint8_t data[7];
    } tx;

    struct {
        byte_u8_t head;
        byte_u8_t data[7];
    } bits;

    uint16_t u16[4];
    uint32_t u32[2];
} byte_cache8_t;


/* ============================================================================
 * byte_cache16_t
 * ----------------------------------------------------------------------------
 * Fixed-size 16-byte cache with multiple views on the same memory.
 *
 * Layout:
 *   raw[0]      = header / command / tag / first byte
 *   raw[1..15]  = payload / data
 *
 * This is a practical default size for many small internal buffers.
 * ============================================================================ */
typedef union {
    uint8_t raw[16];

    struct {
        uint8_t head;
        uint8_t data[15];
    } tx;

    struct {
        byte_u8_t head;
        byte_u8_t data[15];
    } bits;

    uint16_t u16[8];
    uint32_t u32[4];
} byte_cache16_t;


/* ============================================================================
 * byte_cache32_t
 * ----------------------------------------------------------------------------
 * Fixed-size 32-byte cache with multiple views on the same memory.
 * ============================================================================ */
typedef union {
    uint8_t raw[32];

    struct {
        uint8_t head;
        uint8_t data[31];
    } tx;

    struct {
        byte_u8_t head;
        byte_u8_t data[31];
    } bits;

    uint16_t u16[16];
    uint32_t u32[8];
} byte_cache32_t;


/* ============================================================================
 * byte_cache64_t
 * ----------------------------------------------------------------------------
 * Fixed-size 64-byte cache with multiple views on the same memory.
 * ============================================================================ */
typedef union {
    uint8_t raw[64];

    struct {
        uint8_t head;
        uint8_t data[63];
    } tx;

    struct {
        byte_u8_t head;
        byte_u8_t data[63];
    } bits;

    uint16_t u16[32];
    uint32_t u32[16];
} byte_cache64_t;


#ifdef __cplusplus
}
#endif

#endif /* BYTE_CACHE_H */
