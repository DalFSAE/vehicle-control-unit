
#include "stm32f4xx_hal.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LOG_MODULE LOG_SRC_LOG
#include "log.h"

#define UART_BUF_SIZE 256

// CAN payload is 8 bytes: 
// [event_id, level, source, a0_hi, a0_lo, a1_hi, a1_lo, reserved]
#define CAN_PAYLOAD_SIZE 8U

static bool s_log_initialized = false;

static uint32_t log_get_time_ms(void) {
    return HAL_GetTick();
}

// ---------------------------------------------------------------------------
// Sinks
// ---------------------------------------------------------------------------

// Formats the event as human-readable text and sends over UART.
static void sink_uart_event(const LogEvent_t *event) {
    char buf[UART_BUF_SIZE];
    int  len = snprintf(buf, sizeof(buf),
                        "[%lu] src=%u evt=%u lvl=%u a0=%lu a1=%lu\r\n",
                        (unsigned long)event->time_ms,
                        (unsigned)event->source,
                        (unsigned)event->event_id,
                        (unsigned)event->level,
                        (unsigned long)event->a0,
                        (unsigned long)event->a1);
    if (len <= 0) {
        return;
    }
    if ((size_t)len >= sizeof(buf)) {
        len = (int)(sizeof(buf) - 1u);
    }

    // TODO: transmit buf[0..len] over UART/USB CDC
    (void)buf;
    (void)len;
}

// Encodes the event into a compact 8-byte CAN payload (no raw struct).
// Layout: [event_id(1), level(1), source(1), a0[15:8](1), a0[7:0](1),
//          a1[15:8](1), a1[7:0](1), reserved(1)]
static void sink_can_event(const LogEvent_t *event) {
    uint8_t payload[CAN_PAYLOAD_SIZE];
    payload[0] = (uint8_t)event->event_id;
    payload[1] = (uint8_t)event->level;
    payload[2] = (uint8_t)event->source;
    payload[3] = (uint8_t)((event->a0 >> 8u) & 0xFFu);
    payload[4] = (uint8_t)(event->a0 & 0xFFu);
    payload[5] = (uint8_t)((event->a1 >> 8u) & 0xFFu);
    payload[6] = (uint8_t)(event->a1 & 0xFFu);
    payload[7] = 0u; // reserved

    // TODO: transmit payload over CAN (e.g. CAN ID = LOG_EVENT_ID, DLC = 8)
    (void)payload;
}

// Sends a pre-formatted string buffer over UART only.
static void sink_uart_printf(const char *buf, size_t len) {
    // TODO: transmit buf[0..len] over UART/USB CDC
    (void)buf;
    (void)len;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void log_init(void) {
    s_log_initialized = true;
}

bool log_write(const LogEvent_t *event) {
    if ((event == NULL) || !s_log_initialized) {
        return false;
    }

    // Stamp time on a local copy so the caller's struct is not mutated.
    LogEvent_t stamped  = *event;
    stamped.time_ms     = log_get_time_ms();

    sink_uart_event(&stamped);
    sink_can_event(&stamped);
    return true;
}

bool log_event_emit(LogLevel_t level, LogSource_t source,
                    LogEventId_t event_id, uint32_t a0, uint32_t a1) {
    LogEvent_t event = {
        .time_ms  = 0u, // filled by log_write
        .event_id = event_id,
        .level    = level,
        .source   = source,
        .a0       = a0,
        .a1       = a1,
    };
    return log_write(&event);
}

void log_printf(const char *format, ...) {
    if ((format == NULL) || !s_log_initialized) {
        return;
    }

    va_list args;
    char    buf[UART_BUF_SIZE];
    int     len;

    va_start(args, format);
    len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len <= 0) {
        return;
    }
    if ((size_t)len >= sizeof(buf)) {
        len = (int)(sizeof(buf) - 1u);
    }

    sink_uart_printf(buf, (size_t)len);
}
