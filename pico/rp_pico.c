#include <stdarg.h>
#include "rp_pico.h"

/**
 * @brief Reads a single character from UART/USB and translates ANSI escape sequences.
 *
 * This function handles multi-byte sequences for navigation and editing keys
 * like Arrows, Home, End, and Delete. It uses a short timeout to differentiate
 * between a standalone ESC key and the start of a sequence.
 *
 * @return The detected key as @ref key_t.
 */
key_t get_key(void) {
    int c = getchar_timeout_us(1);
    if (c == PICO_ERROR_TIMEOUT) return KEY_NONE;

    /* Handle standard Backspace (ASCII 127 = DEL, ASCII 8 = BS) */
    if (c == 127 || c == 8) return KEY_BACKSPACE;

    /* Handle ANSI Escape Sequences (starting with ESC [ ...) */
    if (c == 27) {
        /* Short wait to see if more bytes follow the ESC */
        c = getchar_timeout_us(1);
        if (c == '[') {
            c = getchar_timeout_us(1);
            switch (c) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case '3':
                    /* Special handling for DELETE (ESC [ 3 ~) */
                    if (getchar_timeout_us(1) == '~') return KEY_DELETE;
                    break;
                default:
                    /* Unknown sequence: consume and return NONE or ESC */
                    return KEY_ESC;
            }
        }
        /* If only ESC was pressed (no '[' followed) */
        return KEY_ESC;
    }

    /* Return standard ASCII character */
    return (key_t)c;
}
