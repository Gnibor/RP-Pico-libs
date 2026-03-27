#ifndef BYTE_OPS_H
#define BYTE_OPS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * byte_ops.h
 * ----------------------------------------------------------------------------
 * Low-level helpers for byte and bit manipulation.
 *
 * This module provides small, reusable primitives for working with raw byte
 * data in a predictable and efficient way.
 *
 * Focus
 * -----
 * - direct bit manipulation (no abstraction layers)
 * - multi-bit field extraction and insertion
 * - byte composition and decomposition
 * - endian-safe load/store operations
 *
 * Design philosophy
 * -----------------
 * - header-only (static inline)
 * - no hidden overhead
 * - no dynamic allocation
 * - explicit, readable operations
 *
 * Typical use cases
 * -----------------
 * - register manipulation
 * - binary protocol parsing
 * - packing/unpacking data structures
 * - embedded / driver development
 * ============================================================================ */


/* ============================================================================
 * Single-bit operations
 * ============================================================================ */

/**
 * @brief Test whether a specific bit is set.
 *
 * @param v   Input byte.
 * @param bit Bit index [0..7].
 * @return true if the bit is set, false otherwise.
 */
static inline bool byte_test_bit(uint8_t v, uint8_t bit)
{
    return (v & (1u << bit)) != 0;
}

/**
 * @brief Set a specific bit.
 *
 * @param v   Input byte.
 * @param bit Bit index [0..7].
 * @return Updated byte.
 */
static inline uint8_t byte_set_bit(uint8_t v, uint8_t bit)
{
    return (uint8_t)(v | (1u << bit));
}

/**
 * @brief Clear a specific bit.
 *
 * @param v   Input byte.
 * @param bit Bit index [0..7].
 * @return Updated byte.
 */
static inline uint8_t byte_clear_bit(uint8_t v, uint8_t bit)
{
    return (uint8_t)(v & ~(1u << bit));
}

/**
 * @brief Toggle (invert) a specific bit.
 *
 * @param v   Input byte.
 * @param bit Bit index [0..7].
 * @return Updated byte.
 */
static inline uint8_t byte_toggle_bit(uint8_t v, uint8_t bit)
{
    return (uint8_t)(v ^ (1u << bit));
}

/**
 * @brief Write a bit to a defined state (0 or 1).
 *
 * @param v     Input byte.
 * @param bit   Bit index [0..7].
 * @param state Desired bit state.
 * @return Updated byte.
 */
static inline uint8_t byte_write_bit(uint8_t v, uint8_t bit, bool state)
{
    return state ? byte_set_bit(v, bit) : byte_clear_bit(v, bit);
}


/* ============================================================================
 * Multi-bit field operations
 * ============================================================================ */

/**
 * @brief Extract a bit-field from a byte.
 *
 * Extracts `width` bits starting at bit position `shift`.
 * The result is right-aligned.
 *
 * Example:
 *   v     = 0b10110100
 *   shift = 2
 *   width = 3
 *   result = 0b101
 *
 * @param v     Input byte.
 * @param shift Start bit position.
 * @param width Number of bits to extract.
 * @return Extracted field (right-aligned).
 *
 * @note width should be <= 8.
 */
static inline uint8_t byte_get_bits(uint8_t v, uint8_t shift, uint8_t width)
{
    return (uint8_t)((v >> shift) & ((1u << width) - 1u));
}

/**
 * @brief Insert a bit-field into a byte.
 *
 * Replaces `width` bits starting at `shift` with `field`.
 * Bits outside the field remain unchanged.
 *
 * Example:
 *   v     = 0b00000000
 *   shift = 2
 *   width = 3
 *   field = 0b101
 *   result = 0b00010100
 *
 * @param v     Original byte.
 * @param shift Start bit position.
 * @param width Number of bits.
 * @param field New field value (lower `width` bits are used).
 * @return Updated byte.
 */
static inline uint8_t byte_set_bits(uint8_t v, uint8_t shift, uint8_t width, uint8_t field)
{
    uint8_t mask = (uint8_t)(((1u << width) - 1u) << shift);

    v &= (uint8_t)~mask;
    v |= (uint8_t)((field << shift) & mask);

    return v;
}


/* ============================================================================
 * Combine / split helpers
 * ============================================================================ */

/**
 * @brief Combine two bytes into a 16-bit little-endian value.
 *
 * @param lo Low byte.
 * @param hi High byte.
 * @return Combined 16-bit value.
 */
static inline uint16_t byte_make_u16_le(uint8_t lo, uint8_t hi)
{
    return (uint16_t)(((uint16_t)lo << 0) | ((uint16_t)hi << 8));
}

/**
 * @brief Combine two bytes into a 16-bit big-endian value.
 *
 * @param hi High byte.
 * @param lo Low byte.
 * @return Combined 16-bit value.
 */
static inline uint16_t byte_make_u16_be(uint8_t hi, uint8_t lo)
{
    return (uint16_t)(((uint16_t)hi << 8) | ((uint16_t)lo << 0));
}

/**
 * @brief Extract the low byte from a 16-bit value.
 */
static inline uint8_t byte_lo_u16(uint16_t v)
{
    return (uint8_t)(v & 0xFFu);
}

/**
 * @brief Extract the high byte from a 16-bit value.
 */
static inline uint8_t byte_hi_u16(uint16_t v)
{
    return (uint8_t)((v >> 8) & 0xFFu);
}


/* ============================================================================
 * Byte swap (endianness conversion)
 * ============================================================================ */

/**
 * @brief Swap byte order of a 16-bit value.
 */
static inline uint16_t byte_swap16(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}

/**
 * @brief Swap byte order of a 32-bit value.
 */
static inline uint32_t byte_swap32(uint32_t v)
{
    return ((v << 24) |
           ((v << 8) & 0x00FF0000u) |
           ((v >> 8) & 0x0000FF00u) |
            (v >> 24));
}

/**
 * @brief Swap byte order of a 64-bit value.
 */
static inline uint64_t byte_swap64(uint64_t v)
{
    return ((v << 56) |
           ((v << 40) & 0x00FF000000000000ull) |
           ((v << 24) & 0x0000FF0000000000ull) |
           ((v << 8)  & 0x000000FF00000000ull) |
           ((v >> 8)  & 0x00000000FF000000ull) |
           ((v >> 24) & 0x0000000000FF0000ull) |
           ((v >> 40) & 0x000000000000FF00ull) |
            (v >> 56));
}


/* ============================================================================
 * Raw loads (unaligned-safe)
 * ============================================================================ */

/**
 * @brief Load a 16-bit little-endian value from raw bytes.
 */
static inline uint16_t byte_load_u16_le(const void *src)
{
    const uint8_t *p = (const uint8_t *)src;
    return (uint16_t)(p[0] | (p[1] << 8));
}

/**
 * @brief Load a 32-bit little-endian value from raw bytes.
 */
static inline uint32_t byte_load_u32_le(const void *src)
{
    const uint8_t *p = (const uint8_t *)src;
    return ((uint32_t)p[0]       |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16)|
           ((uint32_t)p[3] << 24));
}

/**
 * @brief Load a 64-bit little-endian value from raw bytes.
 */
static inline uint64_t byte_load_u64_le(const void *src)
{
    const uint8_t *p = (const uint8_t *)src;
    return ((uint64_t)p[0]        |
           ((uint64_t)p[1] << 8)  |
           ((uint64_t)p[2] << 16) |
           ((uint64_t)p[3] << 24) |
           ((uint64_t)p[4] << 32) |
           ((uint64_t)p[5] << 40) |
           ((uint64_t)p[6] << 48) |
           ((uint64_t)p[7] << 56));
}


/* ============================================================================
 * Raw stores (unaligned-safe)
 * ============================================================================ */

/**
 * @brief Store a 16-bit little-endian value into raw bytes.
 */
static inline void byte_store_u16_le(void *dst, uint16_t v)
{
    uint8_t *p = (uint8_t *)dst;
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}

/**
 * @brief Store a 32-bit little-endian value into raw bytes.
 */
static inline void byte_store_u32_le(void *dst, uint32_t v)
{
    uint8_t *p = (uint8_t *)dst;
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

#ifdef __cplusplus
}
#endif

#endif /* BYTE_OPS_H */
