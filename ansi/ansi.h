#ifndef ANSI_ESC_H
#define ANSI_ESC_H
#include <stdio.h>  /**< Standard I/O for printf, snprintf, and fflush. */
#include <stddef.h> /**< Defines size_t for safe buffer handling. */

/**
 * @file ansi-esc.h
 * @brief THE ULTIMATE TERMINAL CONTROL & TUI LIBRARY (v6.0 - Final Master)
 *
 * This header is designed to be the "Single Source of Truth" for terminal
 * manipulation on both Linux (for your TUI) and RP Pico (for serial feedback).
 *
 * ARCHITECTURE:
 * 1. C0 CONTROLS: Single ASCII bytes for fundamental movements (Fastest).
 * 2. STATIC MACROS: Compile-time strings for fixed UI elements (Zero overhead).
 * 3. RUNTIME FORMATS: Format strings for printf() with dynamic variables.
 * 4. BUILDER FUNCTIONS: Safe snprintf wrappers to store sequences in variables.
 */

/* --- 1. ASCII C0 CONTROL CHARACTERS ---
 * These are the most basic commands. Every terminal emulator supports them.
 */
/** @{ */
#define ANSI_LF          "\n"   /**< Line Feed: Move to the next line. */
#define ANSI_CR          "\r"   /**< Carriage Return: Move to the BEGINNING of the current line. */
#define ANSI_TAB         "\t"   /**< Horizontal Tab: Move to the next tab stop (usually 8 chars). */
#define ANSI_BS          "\b"   /**< Backspace: Move cursor left by one. Note: Does not erase! */
#define ANSI_BELL        "\a"   /**< Alert/Bell: Triggers a system beep or visual flash. */
#define ANSI_VT          "\v"   /**< Vertical Tab: Move to the next vertical tab stop. */
#define ANSI_FF          "\f"   /**< Form Feed: Used for "New Page" or clearing old printers. */
#define ANSI_NEWLINE     "\r\n" /**< Standard CRLF: Essential for Serial/UART stability. */
/** @} */

/* --- 2. STRINGIFICATION HELPERS --- */
#define ANSI_STR_HELPER(x) #x
#define ANSI_STR(x) ANSI_STR_HELPER(x)
#define ANSI_ESC            "\033["     /**< The Control Sequence Introducer (CSI). */

/**
 * @name 3. TEXT STYLES & DECORATIONS
 * @brief Attributes to change the visual appearance of text.
 * @{
 */
#define ANSI_RESET          "\033[0m"   /**< Reset ALL colors and styles to default. */
#define ANSI_BOLD           "\033[1m"   /**< High intensity / Thick text. */
#define ANSI_DIM            "\033[2m"   /**< Faint / Low intensity text. */
#define ANSI_ITALIC         "\033[3m"   /**< Slanted text (Support varies). */
#define ANSI_UNDERLINE      "\033[4m"   /**< Standard underline. */
#define ANSI_BLINK          "\033[5m"   /**< Flashing text (Slow blink). */
#define ANSI_REVERSE        "\033[7m"   /**< Inverse: Swaps Foreground and Background colors. */
#define ANSI_HIDDEN         "\033[8m"   /**< Invisible text (Still occupies space). */
#define ANSI_STRIKE         "\033[9m"   /**< Crossed-out text. */

/* Individual Attribute Resets (To turn off ONLY one specific style) */
#define ANSI_BOLD_OFF       "\033[22m"  /**< Back to normal intensity (Turns off Bold & Dim). */
#define ANSI_ITALIC_OFF     "\033[23m"
#define ANSI_UNDER_OFF      "\033[24m"
#define ANSI_BLINK_OFF      "\033[25m"
#define ANSI_REV_OFF        "\033[27m"
#define ANSI_STRIKE_OFF     "\033[29m"
/** @} */

/**
 * @name 4. SCREEN & VIEWPORT MANAGEMENT
 * @brief Manipulation of the visible area and screen buffers.
 * @{
 */
#define ANSI_CLR_ALL        "\033[2J"   /**< Clear the entire visible screen area. */
#define ANSI_CLR_BUF        "\033[3J"   /**< Clear the scrollback history (The area above the screen). */
#define ANSI_HOME           "\033[H"    /**< Move cursor to Row 1, Column 1. */
#define ANSI_CLR_LINE       "\033[2K"   /**< Clear the entire line where the cursor is. */

/* Erase Functions (Relative to cursor) */
#define ANSI_CLR_TO_END     "\033[0J"   /**< Clear from cursor down to screen bottom. */
#define ANSI_CLR_TO_STRT    "\033[1J"   /**< Clear from cursor up to screen top. */
#define ANSI_CLR_L_END      "\033[0K"   /**< Clear from cursor to the END of the current line. */
#define ANSI_CLR_L_STRT     "\033[1K"   /**< Clear from cursor back to the START of the current line. */

/* Buffer Management (Crucial for TUI Apps) */
#define ANSI_BUF_APP        "\033[?1049h" /**< Switch to Alt Buffer (Hides your shell history - for TUI). */
#define ANSI_BUF_MAIN       "\033[?1049l" /**< Return to Main Buffer (Restores previous bash screen). */
/** @} */

/**
 * @name 5. CURSOR & LINE CONTROL
 * @brief Manage cursor visibility, shapes, and behavior.
 * @{
 */
#define ANSI_HIDE_CUR       "\033[?25l" /**< Make the cursor invisible (Standard for TUI drawing). */
#define ANSI_SHOW_CUR       "\033[?25h" /**< Restore cursor visibility. */
#define ANSI_SAVE_CUR       "\033[s"    /**< Save current X/Y position in terminal memory. */
#define ANSI_RESTORE_CUR    "\033[u"    /**< Move cursor back to the last saved position. */

/* Cursor Shapes (Supported by PuTTY, VSCode, iTerm2, Windows Terminal) */
#define ANSI_CUR_BLOCK      "\033[1 q"  /**< Blinking Block. */
#define ANSI_CUR_STEADY     "\033[2 q"  /**< Static Block. */
#define ANSI_CUR_BAR        "\033[5 q"  /**< Blinking Vertical Line. */

/* Automatic Line Wrapping */
#define ANSI_WRAP_ON        "\033[7h"   /**< Text wraps to next line at screen edge. */
#define ANSI_WRAP_OFF       "\033[7l"   /**< Text stays on one line (is cut off). */
/** @} */

/**
 * @name 6. RUNTIME FORMATS (For Variable Support)
 * @brief These strings are for printf() when using dynamic variables.
 * @{
 */
#define ANSI_FMT_GOTO       "\033[%d;%dH"       /**< Target: Row, Col. */
#define ANSI_FMT_CUR_UP     "\033[%dA"          /**< Target: N steps up. */
#define ANSI_FMT_CUR_DOWN   "\033[%dB"          /**< Target: N steps down. */
#define ANSI_FMT_CUR_RIGHT  "\033[%dC"          /**< Target: N steps right. */
#define ANSI_FMT_CUR_LEFT   "\033[%dD"          /**< Target: N steps left. */
#define ANSI_FMT_SET_SCROLL "\033[%d;%dr"       /**< Target: TopRow, BottomRow. */
#define ANSI_FMT_FG_RGB     "\033[38;2;%d;%d;%dm" /**< Target: Red, Green, Blue (0-255). */
#define ANSI_FMT_BG_RGB     "\033[48;2;%d;%d;%dm" /**< Target: Red, Green, Blue (0-255). */
#define ANSI_FMT_FG_256     "\033[38;5;%dm"     /**< Target: 256-Color Index. */
#define ANSI_FMT_BG_256     "\033[48;5;%dm"     /**< Target: 256-Color Index. */
/** @} */

/**
 * @name 7. BUILDER FUNCTIONS (Store Sequences in Strings)
 * @brief Safe snprintf wrappers for storing codes in char arrays (Variables).
 * @{
 */

/** @brief Stores a Move-To sequence. Row/Col are 1-indexed. */
static inline int ansi_build_goto(char* b, size_t s, int r, int c)		{ return snprintf(b, s, ANSI_FMT_GOTO, r, c); }
/** @brief Stores a Move-Up sequence by N rows. */
static inline int ansi_build_up(char* b, size_t s, int n)			{ return snprintf(b, s, ANSI_FMT_CUR_UP, n); }
/** @brief Stores a Move-Down sequence by N rows. */
static inline int ansi_build_down(char* b, size_t s, int n)			{ return snprintf(b, s, ANSI_FMT_CUR_DOWN, n); }
/** @brief Stores a Move-Right sequence by N cols. */
static inline int ansi_build_right(char* b, size_t s, int n)			{ return snprintf(b, s, ANSI_FMT_CUR_RIGHT, n); }
/** @brief Stores a Move-Left sequence by N cols. */
static inline int ansi_build_left(char* b, size_t s, int n)			{ return snprintf(b, s, ANSI_FMT_CUR_LEFT, n); }

/* Color Builders (Variable Colors) */
static inline int ansi_build_fg_rgb(char* b, size_t s, int r, int g, int v)	{ return snprintf(b, s, ANSI_FMT_FG_RGB, r, g, v); }
static inline int ansi_build_bg_rgb(char* b, size_t s, int r, int g, int v)	{ return snprintf(b, s, ANSI_FMT_BG_RGB, r, g, v); }
static inline int ansi_build_fg_256(char* b, size_t s, int n)			{ return snprintf(b, s, ANSI_FMT_FG_256, n); }
static inline int ansi_build_bg_256(char* b, size_t s, int n)			{ return snprintf(b, s, ANSI_FMT_BG_256, n); }

/* Layout Builders */
/** @brief Define scrolling region (Margins). Text outside remains fixed. */
static inline int ansi_build_scroll(char* b, size_t s, int t, int bot)		{ return snprintf(b, s, ANSI_FMT_SET_SCROLL, t, bot); }
/** @} */

/**
 * @name 8. COMPILE-TIME MACROS (For Literal Constants)
 * @brief Use these if you know the numbers while coding. Fastest execution.
 * @{
 */
#define ANSI_GOTO(y, x)      ANSI_ESC ANSI_STR(y) ";" ANSI_STR(x) "H"
#define ANSI_SET_SCROLL(t,b) ANSI_ESC ANSI_STR(t) ";" ANSI_STR(b) "r"
#define ANSI_FG_256(n)       ANSI_ESC "38;5;" ANSI_STR(n) "m"
#define ANSI_BG_256(n)       ANSI_ESC "48;5;" ANSI_STR(n) "m"
/** @} */

/**
 * @name 9. TABULATOR MANAGEMENT
 * @brief Control horizontal alignment stops.
 * @{
 */
#define ANSI_SET_TAB         "\033H"     /**< Set a tab stop at the current cursor column. */
#define ANSI_CLR_TAB         "\033[g"    /**< Clear the tab stop at the current column. */
#define ANSI_CLR_TABS_ALL    "\033[3g"   /**< Clear ALL tab stops in the terminal. */
/** @} */

/**
 * @name 10. ADVANCED UTILITIES & BATCHING
 * @{
 */

/** @brief Set the terminal's window or tab title (OSC command). */
static inline void ansi_set_title(const char* t)	{ printf("\033]0;%s\007", t); fflush(stdout); }

/** @brief Request current cursor position. Terminal writes answer to STDIN as ESC[row;colR. */
static inline void ansi_req_cursor_pos(void)		{ printf("\033[6n"); fflush(stdout); }

/**
 * @brief START BATCH UPDATE: Terminal stops rendering until BATCH_END.
 * Eliminates flickering in high-speed TUIs or animations.
 */
#define ANSI_BATCH_START    "\033[?2026h"
/** @brief END BATCH UPDATE: Force render all changes at once. */
#define ANSI_BATCH_END      "\033[?2026l"

#define ANSI_RESET_SCROLL   "\033[r"    /**< Reset scrolling region to full screen. */
/** @} */

/**
 * @name 11. BOX DRAWING (UTF-8 / Unicode)
 * @brief Standard box characters. Requires UTF-8 support in Terminal.
 * @{
 */
#define ANSI_BOX_H          "─" /**< Horizontal line */
#define ANSI_BOX_V          "│" /**< Vertical line */
#define ANSI_BOX_TL         "┌" /**< Top-left corner */
#define ANSI_BOX_TR         "┐" /**< Top-right corner */
#define ANSI_BOX_BL         "└" /**< Bottom-left corner */
#define ANSI_BOX_BR         "┘" /**< Bottom-right corner */
#define ANSI_BOX_T_UP       "┴" /**< T-junction pointing up */
#define ANSI_BOX_T_DN       "┬" /**< T-junction pointing down */
#define ANSI_BOX_DIV_L      "┤" /**< Divider / T-junction left */
#define ANSI_BOX_DIV_R      "├" /**< Divider / T-junction right */
#define ANSI_BOX_CROSS      "┼" /**< Cross junction */

/* Double line versions (Optional) */
#define ANSI_DBOX_H         "═"
#define ANSI_DBOX_V         "║"
#define ANSI_DBOX_TL        "╔"
#define ANSI_DBOX_TR        "╗"
#define ANSI_DBOX_BL        "╚"
#define ANSI_DBOX_BR        "╝"
/** @} */

/**
 * 12. Color codes
 */
/**
 * @name Standard Foreground Colors (3x)
 * @{
 */
#define ANSI_BLACK          "\033[30m"          /**< Standard Black text. */
#define ANSI_RED            "\033[31m"          /**< Standard Red text. */
#define ANSI_GREEN          "\033[32m"          /**< Standard Green text. */
#define ANSI_YELLOW         "\033[33m"          /**< Standard Yellow text. */
#define ANSI_BLUE           "\033[34m"          /**< Standard Blue text. */
#define ANSI_MAGENTA        "\033[35m"          /**< Standard Magenta text. */
#define ANSI_CYAN           "\033[36m"          /**< Standard Cyan text. */
#define ANSI_WHITE          "\033[37m"          /**< Standard White text. */
/** @} */

/**
 * @name Bright Foreground Colors (9x)
 * @{
 */
#define ANSI_BRIGHT_BLACK   "\033[90m"          /**< Bright Black (Dark Gray). */
#define ANSI_BRIGHT_RED     "\033[91m"          /**< Bright Red text. */
#define ANSI_BRIGHT_GREEN   "\033[92m"          /**< Bright Green text. */
#define ANSI_BRIGHT_YELLOW  "\033[93m"          /**< Bright Yellow text. */
#define ANSI_BRIGHT_BLUE    "\033[94m"          /**< Bright Blue text. */
#define ANSI_BRIGHT_MAGENTA "\033[95m"          /**< Bright Magenta text. */
#define ANSI_BRIGHT_CYAN    "\033[96m"          /**< Bright Cyan text. */
#define ANSI_BRIGHT_WHITE   "\033[97m"          /**< Bright White text. */
/** @} */

/**
 * @name Background Colors (4x)
 * @{
 */
#define ANSI_BG_BLACK       "\033[40m"          /**< Black background. */
#define ANSI_BG_RED         "\033[41m"          /**< Red background. */
#define ANSI_BG_GREEN       "\033[42m"          /**< Green background. */
#define ANSI_BG_YELLOW      "\033[43m"          /**< Yellow background. */
#define ANSI_BG_BLUE        "\033[44m"          /**< Blue background. */
#define ANSI_BG_MAGENTA     "\033[45m"          /**< Magenta background. */
#define ANSI_BG_CYAN        "\033[46m"          /**< Cyan background. */
#define ANSI_BG_WHITE       "\033[47m"          /**< White background. */
/** @} */

#endif /* ANSI_ESC_H */
