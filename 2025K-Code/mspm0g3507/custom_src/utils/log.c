#include "log.h"

static char buffer[MAX_LOG_SIZE];

static void log_print(int level, const char *format, va_list args) {
    if (level <= MODULE_LOG_LEVEL) {
        const char *level_tag = "";
        switch (level) {
            case LOG_LEVEL_DEBUG:
                level_tag = LOG_TAG_DEBUG;
                break;
            case LOG_LEVEL_INFO:
                level_tag = LOG_TAG_INFO;
                break;
            case LOG_LEVEL_WARN:
                level_tag = LOG_TAG_WARN;
                break;
            case LOG_LEVEL_ERROR:
                level_tag = LOG_TAG_ERROR;
                break;
            default:
                level_tag = "[UNKNOWN]";
                break;
        }
        vsnprintf(buffer, sizeof(buffer), format, args);
        usart_printf(UART_0_INST, "%s %s\r\n", level_tag, buffer);
    }
}

void LOG_D(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void LOG_I(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void LOG_W(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void LOG_E(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}