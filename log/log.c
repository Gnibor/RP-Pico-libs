#include <stdarg.h>
#include <stdio.h>
#include "log.h"
// Neogit test
void push_log(log_level_t level, const char *fmt, ...){
	LOG_TIME();

	// 4. Set color and prefix based on level
	switch (level) {
		case LOG_INFO:  printf(ANSI_GREEN  "[INFO] " ANSI_RESET); break;
		case LOG_WARN:  printf(ANSI_YELLOW "[WARN] " ANSI_RESET); break;
		case LOG_ERROR: printf(ANSI_RED    "[ERROR] " ANSI_RESET); break;
		case LOG_DEBUG: printf(ANSI_ITALIC ANSI_CYAN "[DEBUG] " ANSI_RESET); break;
		default: printf(ANSI_BG_MAGENTA ANSI_BLUE "[UNKNOWN] " ANSI_RESET); break;
	}

	// 5. Process the actual message (like printf)
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf(ANSI_RESET);
#if LOG_NEW_LINE
	// 6. Always end with a newline and reset
	printf("\n");
#endif
}
