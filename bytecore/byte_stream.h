#ifndef BYTE_STREAM_H
#define BYTE_STREAM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * byte_stream.h
 * ----------------------------------------------------------------------------
 * Lightweight byte stream helper for sequential access to an existing buffer.
 *
 * Features
 * --------
 * - cursor-based read/write access
 * - bounds-checked operations
 * - little-endian and big-endian helpers
 * - peek / lookahead helpers
 * - binary and text match helpers for parser use
 *
 * Notes
 * -----
 * - The stream does not own the buffer.
 * - No dynamic allocation is used.
 * - The API is intentionally byte-oriented.
 * - Absolute-position helpers are limited to matching/lookahead in order to
 *   keep the stream model clean.
 * ============================================================================ */

/**
 * @brief Cursor-based byte stream.
 *
 * @param buf  Pointer to external buffer memory.
 * @param size Total size of the buffer in bytes.
 * @param pos  Current cursor position in bytes.
 */
typedef struct {
    uint8_t *buf;
    size_t size;
    size_t pos;
} byte_stream_t;


/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * @brief Initialize a byte stream on an existing buffer.
 *
 * @param s    Pointer to stream object.
 * @param buf  Pointer to external buffer memory.
 * @param size Buffer size in bytes.
 */
static inline void byte_stream_init(byte_stream_t *s, void *buf, size_t size)
{
    s->buf  = (uint8_t *)buf;
    s->size = size;
    s->pos  = 0;
}

/**
 * @brief Reset the cursor to the beginning of the stream.
 *
 * @param s Pointer to stream object.
 */
static inline void byte_stream_reset(byte_stream_t *s)
{
    s->pos = 0;
}


/* ============================================================================
 * State
 * ============================================================================ */

/**
 * @brief Get the current cursor position.
 *
 * @param s Pointer to stream object.
 * @return Current position in bytes.
 */
static inline size_t byte_stream_pos(const byte_stream_t *s)
{
    return s->pos;
}

/**
 * @brief Get total stream size.
 *
 * @param s Pointer to stream object.
 * @return Total buffer size in bytes.
 */
static inline size_t byte_stream_size(const byte_stream_t *s)
{
    return s->size;
}

/**
 * @brief Get number of bytes available from the current cursor to the end.
 *
 * @param s Pointer to stream object.
 * @return Remaining byte count.
 */
static inline size_t byte_stream_available(const byte_stream_t *s)
{
    return (s->pos <= s->size) ? (s->size - s->pos) : 0;
}

/**
 * @brief Check whether the cursor reached the end of the stream.
 *
 * @param s Pointer to stream object.
 * @return true if at end of stream, otherwise false.
 */
static inline bool byte_stream_eof(const byte_stream_t *s)
{
    return s->pos >= s->size;
}


/* ============================================================================
 * Cursor movement
 * ============================================================================ */

/**
 * @brief Move the cursor to an absolute position.
 *
 * @param s   Pointer to stream object.
 * @param pos New absolute position.
 * @return true on success, false if out of bounds.
 */
static inline bool byte_stream_seek(byte_stream_t *s, size_t pos)
{
    if (pos > s->size) {
        return false;
    }

    s->pos = pos;
    return true;
}

/**
 * @brief Advance the cursor by a number of bytes.
 *
 * @param s Pointer to stream object.
 * @param n Number of bytes to skip.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_skip(byte_stream_t *s, size_t n)
{
    if (byte_stream_available(s) < n) {
        return false;
    }

    s->pos += n;
    return true;
}

/**
 * @brief Move the cursor backward by a number of bytes.
 *
 * @param s Pointer to stream object.
 * @param n Number of bytes to rewind.
 * @return true on success, false if rewinding would go before 0.
 */
static inline bool byte_stream_rewind(byte_stream_t *s, size_t n)
{
    if (n > s->pos) {
        return false;
    }

    s->pos -= n;
    return true;
}


/* ============================================================================
 * Raw byte access
 * ============================================================================ */

/**
 * @brief Read a single byte and advance the cursor.
 *
 * @param s   Pointer to stream object.
 * @param out Output byte pointer.
 * @return true on success, false if no byte is available.
 */
static inline bool byte_stream_read_u8(byte_stream_t *s, uint8_t *out)
{
    if (byte_stream_available(s) < 1) {
        return false;
    }

    *out = s->buf[s->pos++];
    return true;
}

/**
 * @brief Peek a single byte without advancing the cursor.
 *
 * @param s   Pointer to stream object.
 * @param out Output byte pointer.
 * @return true on success, false if no byte is available.
 */
static inline bool byte_stream_peek_u8(const byte_stream_t *s, uint8_t *out)
{
    if (byte_stream_available(s) < 1) {
        return false;
    }

    *out = s->buf[s->pos];
    return true;
}

/**
 * @brief Write a single byte and advance the cursor.
 *
 * @param s Pointer to stream object.
 * @param v Byte value to write.
 * @return true on success, false if no space remains.
 */
static inline bool byte_stream_write_u8(byte_stream_t *s, uint8_t v)
{
    if (byte_stream_available(s) < 1) {
        return false;
    }

    s->buf[s->pos++] = v;
    return true;
}

/**
 * @brief Read multiple raw bytes and advance the cursor.
 *
 * @param s   Pointer to stream object.
 * @param dst Destination buffer.
 * @param len Number of bytes to read.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_read(byte_stream_t *s, void *dst, size_t len)
{
    if (byte_stream_available(s) < len) {
        return false;
    }

    memcpy(dst, &s->buf[s->pos], len);
    s->pos += len;
    return true;
}

/**
 * @brief Peek multiple raw bytes without advancing the cursor.
 *
 * @param s   Pointer to stream object.
 * @param dst Destination buffer.
 * @param len Number of bytes to copy.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_peek(const byte_stream_t *s, void *dst, size_t len)
{
    if (byte_stream_available(s) < len) {
        return false;
    }

    memcpy(dst, &s->buf[s->pos], len);
    return true;
}

/**
 * @brief Write multiple raw bytes and advance the cursor.
 *
 * @param s   Pointer to stream object.
 * @param src Source buffer.
 * @param len Number of bytes to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_write(byte_stream_t *s, const void *src, size_t len)
{
    if (byte_stream_available(s) < len) {
        return false;
    }

    memcpy(&s->buf[s->pos], src, len);
    s->pos += len;
    return true;
}


/* ============================================================================
 * Comparison helpers
 * ============================================================================ */

/**
 * @brief Compare upcoming bytes with external data without advancing.
 *
 * @param s    Pointer to stream object.
 * @param data Data to compare against.
 * @param len  Number of bytes to compare.
 * @return true if equal, false otherwise.
 */
static inline bool byte_stream_match(const byte_stream_t *s, const void *data, size_t len)
{
    if (byte_stream_available(s) < len) {
        return false;
    }

    return memcmp(&s->buf[s->pos], data, len) == 0;
}

/**
 * @brief Compare upcoming bytes and advance on success.
 *
 * @param s    Pointer to stream object.
 * @param data Data to compare against.
 * @param len  Number of bytes to compare.
 * @return true if equal and consumed, false otherwise.
 */
static inline bool byte_stream_consume_if_match(byte_stream_t *s, const void *data, size_t len)
{
    if (!byte_stream_match(s, data, len)) {
        return false;
    }

    s->pos += len;
    return true;
}

/**
 * @brief Compare bytes at an absolute position without changing the cursor.
 *
 * @param s    Pointer to stream object.
 * @param pos  Absolute buffer position.
 * @param data Data to compare against.
 * @param len  Number of bytes to compare.
 * @return true if equal, false otherwise.
 */
static inline bool byte_stream_match_at(const byte_stream_t *s, size_t pos, const void *data, size_t len)
{
    if (pos > s->size || (s->size - pos) < len) {
        return false;
    }

    return memcmp(&s->buf[pos], data, len) == 0;
}

/**
 * @brief Compare upcoming bytes with a null-terminated string.
 *
 * @param s   Pointer to stream object.
 * @param str Null-terminated string to compare against.
 * @return true if equal, false otherwise.
 */
static inline bool byte_stream_match_str(const byte_stream_t *s, const char *str)
{
    size_t len = strlen(str);
    return byte_stream_match(s, str, len);
}

/**
 * @brief Compare upcoming bytes with a string and advance on success.
 *
 * @param s   Pointer to stream object.
 * @param str Null-terminated string to compare against.
 * @return true if equal and consumed, false otherwise.
 */
static inline bool byte_stream_consume_if_match_str(byte_stream_t *s, const char *str)
{
    size_t len = strlen(str);
    return byte_stream_consume_if_match(s, str, len);
}

/**
 * @brief Compare bytes at an absolute position with a null-terminated string.
 *
 * @param s   Pointer to stream object.
 * @param pos Absolute buffer position.
 * @param str Null-terminated string to compare against.
 * @return true if equal, false otherwise.
 */
static inline bool byte_stream_match_str_at(const byte_stream_t *s, size_t pos, const char *str)
{
    size_t len = strlen(str);
    return byte_stream_match_at(s, pos, str, len);
}


/* ============================================================================
 * Little-endian reads
 * ============================================================================ */

/**
 * @brief Read a 16-bit little-endian value.
 *
 * @param s   Pointer to stream object.
 * @param out Output value pointer.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_read_u16_le(byte_stream_t *s, uint16_t *out)
{
    if (byte_stream_available(s) < 2) {
        return false;
    }

    *out = (uint16_t)(
        ((uint16_t)s->buf[s->pos + 0] << 0) |
        ((uint16_t)s->buf[s->pos + 1] << 8)
    );

    s->pos += 2;
    return true;
}

/**
 * @brief Read a 32-bit little-endian value.
 *
 * @param s   Pointer to stream object.
 * @param out Output value pointer.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_read_u32_le(byte_stream_t *s, uint32_t *out)
{
    if (byte_stream_available(s) < 4) {
        return false;
    }

    *out = (uint32_t)(
        ((uint32_t)s->buf[s->pos + 0] << 0)  |
        ((uint32_t)s->buf[s->pos + 1] << 8)  |
        ((uint32_t)s->buf[s->pos + 2] << 16) |
        ((uint32_t)s->buf[s->pos + 3] << 24)
    );

    s->pos += 4;
    return true;
}


/* ============================================================================
 * Big-endian reads
 * ============================================================================ */

/**
 * @brief Read a 16-bit big-endian value.
 *
 * @param s   Pointer to stream object.
 * @param out Output value pointer.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_read_u16_be(byte_stream_t *s, uint16_t *out)
{
    if (byte_stream_available(s) < 2) {
        return false;
    }

    *out = (uint16_t)(
        ((uint16_t)s->buf[s->pos + 0] << 8) |
        ((uint16_t)s->buf[s->pos + 1] << 0)
    );

    s->pos += 2;
    return true;
}

/**
 * @brief Read a 32-bit big-endian value.
 *
 * @param s   Pointer to stream object.
 * @param out Output value pointer.
 * @return true on success, false if not enough bytes remain.
 */
static inline bool byte_stream_read_u32_be(byte_stream_t *s, uint32_t *out)
{
    if (byte_stream_available(s) < 4) {
        return false;
    }

    *out = (uint32_t)(
        ((uint32_t)s->buf[s->pos + 0] << 24) |
        ((uint32_t)s->buf[s->pos + 1] << 16) |
        ((uint32_t)s->buf[s->pos + 2] << 8)  |
        ((uint32_t)s->buf[s->pos + 3] << 0)
    );

    s->pos += 4;
    return true;
}


/* ============================================================================
 * Little-endian writes
 * ============================================================================ */

/**
 * @brief Write a 16-bit little-endian value.
 *
 * @param s Pointer to stream object.
 * @param v Value to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_write_u16_le(byte_stream_t *s, uint16_t v)
{
    if (byte_stream_available(s) < 2) {
        return false;
    }

    s->buf[s->pos + 0] = (uint8_t)((v >> 0) & 0xFFu);
    s->buf[s->pos + 1] = (uint8_t)((v >> 8) & 0xFFu);
    s->pos += 2;
    return true;
}

/**
 * @brief Write a 32-bit little-endian value.
 *
 * @param s Pointer to stream object.
 * @param v Value to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_write_u32_le(byte_stream_t *s, uint32_t v)
{
    if (byte_stream_available(s) < 4) {
        return false;
    }

    s->buf[s->pos + 0] = (uint8_t)((v >> 0) & 0xFFu);
    s->buf[s->pos + 1] = (uint8_t)((v >> 8) & 0xFFu);
    s->buf[s->pos + 2] = (uint8_t)((v >> 16) & 0xFFu);
    s->buf[s->pos + 3] = (uint8_t)((v >> 24) & 0xFFu);
    s->pos += 4;
    return true;
}


/* ============================================================================
 * Big-endian writes
 * ============================================================================ */

/**
 * @brief Write a 16-bit big-endian value.
 *
 * @param s Pointer to stream object.
 * @param v Value to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_write_u16_be(byte_stream_t *s, uint16_t v)
{
    if (byte_stream_available(s) < 2) {
        return false;
    }

    s->buf[s->pos + 0] = (uint8_t)((v >> 8) & 0xFFu);
    s->buf[s->pos + 1] = (uint8_t)((v >> 0) & 0xFFu);
    s->pos += 2;
    return true;
}

/**
 * @brief Write a 32-bit big-endian value.
 *
 * @param s Pointer to stream object.
 * @param v Value to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_write_u32_be(byte_stream_t *s, uint32_t v)
{
    if (byte_stream_available(s) < 4) {
        return false;
    }

    s->buf[s->pos + 0] = (uint8_t)((v >> 24) & 0xFFu);
    s->buf[s->pos + 1] = (uint8_t)((v >> 16) & 0xFFu);
    s->buf[s->pos + 2] = (uint8_t)((v >> 8) & 0xFFu);
    s->buf[s->pos + 3] = (uint8_t)((v >> 0) & 0xFFu);
    s->pos += 4;
    return true;
}


/* ============================================================================
 * Fill helpers
 * ============================================================================ */

/**
 * @brief Fill bytes at the current cursor position with one value.
 *
 * @param s     Pointer to stream object.
 * @param value Fill byte value.
 * @param len   Number of bytes to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_fill(byte_stream_t *s, uint8_t value, size_t len)
{
    if (byte_stream_available(s) < len) {
        return false;
    }

    memset(&s->buf[s->pos], value, len);
    s->pos += len;
    return true;
}

/**
 * @brief Fill bytes at the current cursor position with zero.
 *
 * @param s   Pointer to stream object.
 * @param len Number of bytes to write.
 * @return true on success, false if not enough space remains.
 */
static inline bool byte_stream_zero(byte_stream_t *s, size_t len)
{
    return byte_stream_fill(s, 0, len);
}

#ifdef __cplusplus
}
#endif

#endif /* BYTE_STREAM_H */
