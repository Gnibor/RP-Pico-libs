#ifndef _LOG_CFG_H_
#define _LOG_CFG_H_
#include "rp_pico.h"

// Color for function names
#define ANSI_COLOR_FN "\033[38;2;254;172;51m"
#define ANSI_COLOR_LINE "\x1b[38;2;160;160;170m"

#define LOG_TIME() do { \
    printf(ANSI_DIM); \
    pico_tsprintf("[%h:%m:%s:%S] "); \
    printf(ANSI_RESET); \
} while (0)

#define LOG_PREFIX(fmt) ANSI_COLOR_FN "%s" ANSI_RESET "():" ANSI_BOLD ANSI_COLOR_LINE "%d" ANSI_RESET ": " fmt, __func__, __LINE__

#endif
