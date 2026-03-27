#ifndef _LOG_H_
#define _LOG_H_
#include "ansi.h"
#include "log_cfg.h"

#ifndef INFO_ENABLED
#define INFO_ENABLED 1
#endif

#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0
#endif

#ifndef LOG_NEW_LINE
#define LOG_NEW_LINE 1
#endif

#ifndef LOG_PREFIX
#define LOG_PREFIX(fmt) fmt
#endif

#ifndef LOG_TIME
#define LOG_TIME() ((void)0)
#endif


// Log Levels
typedef enum {
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_DEBUG
} log_level_t;

// The core push_log function
void push_log(log_level_t level, const char *fmt, ...);

// Handy macros for shorter calls
// #define LOG_W(...) pico_log(LOG_WARN,  __VA_ARGS__)
//#define LOG_E(...) pico_log(LOG_ERROR, __VA_ARGS__)
#define LOG_E(fmt, ...) do{ push_log(LOG_ERROR, LOG_PREFIX(fmt), ##__VA_ARGS__); } while(0)
#define LOG_W(fmt, ...) do{ push_log(LOG_WARN, LOG_PREFIX(fmt), ##__VA_ARGS__); } while(0)

#if INFO_ENABLED
// #define LOG_I(...) pico_log(LOG_INFO, __VA_ARGS__)
#define LOG_I(fmt, ...) do{ push_log(LOG_INFO, LOG_PREFIX(fmt), ##__VA_ARGS__); } while(0)
#else // INFO_ENABLED
#define LOG_I(...) ((void)0) // Completely ignored by the compiler
#endif // INFO_ENABLED

// The Magic: If disabled, LOG_D does absolutely nothing
#if DEBUG_ENABLED
// #define LOG_D(...) pico_log(LOG_DEBUG, __VA_ARGS__)
#define LOG_D(fmt, ...) do{ push_log(LOG_DEBUG, LOG_PREFIX(fmt), ##__VA_ARGS__); } while(0)
#else // DEBUG_ENABLED
#define LOG_D(...) ((void)0) // Completely ignored by the compiler
#endif // DEBUG_ENABLED

#endif // _LOG_H_
