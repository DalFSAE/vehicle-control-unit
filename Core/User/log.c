#include "log.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define USB_BUF_SIZE 256

static bool s_log_initialized = false;

static bool log_backend_write(const uint8_t *data, size_t len);

void log_init(void)
{
    s_log_initialized = true;
}

bool log_write(const LogEvent_t *event)
{
    if ((event == NULL) || !s_log_initialized) {
        return false;
    }

    return log_backend_write((const uint8_t *)event, sizeof(*event));
}

bool log_write_fields(uint8_t level, uint8_t source, uint16_t event_id,
                      uint32_t a0, uint32_t a1)
{
    LogEvent_t event = {
        .time_ms  = 0u,
        .event_id = event_id,
        .level    = level,
        .source   = source,
        .a0       = a0,
        .a1       = a1,
    };

    return log_write(&event);
}

void log_printf(const char *format, ...)
{
    if ((format == NULL) || !s_log_initialized) {
        return;
    }

    va_list args;
    char    buffer[USB_BUF_SIZE];
    int     len;

    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len <= 0) {
        return;
    }

    if ((size_t)len >= sizeof(buffer)) {
        len = (int)(sizeof(buffer) - 1u);
    }

    (void)log_backend_write((const uint8_t *)buffer, (size_t)len);
}

static bool log_backend_write(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;

    // Transport hook goes here once USB/CAN logging is selected.
    return true;
}
