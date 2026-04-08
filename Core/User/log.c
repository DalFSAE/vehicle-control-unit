
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LOG_MODULE LOG_SRC_LOG
#include "log.h"

#define UART_BUF_SIZE 256
#define LOG_USB_TX_RETRY_COUNT 5U

// CAN payload is 8 bytes:
// [event_id, level, source, a0_hi, a0_lo, a1_hi, a1_lo, reserved]
#define CAN_PAYLOAD_SIZE 8U

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

static bool    s_log_initialized = false;
static uint8_t s_usb_tx_buffer[UART_BUF_SIZE];

static bool log_usb_write(const char *buf, size_t len) {
    if ((buf == NULL) || (len == 0u)) {
        return false;
    }

    if (osKernelGetState() != osKernelRunning) {
        return false;
    }

    if (len > sizeof(s_usb_tx_buffer)) {
        len = sizeof(s_usb_tx_buffer);
    }

    memcpy(s_usb_tx_buffer, buf, len);

    for (uint32_t attempt = 0u; attempt < LOG_USB_TX_RETRY_COUNT; ++attempt) {
        uint8_t status = CDC_Transmit_FS(s_usb_tx_buffer, (uint16_t)len);
        if (status == USBD_OK) {
            return true;
        }
        if (status != USBD_BUSY) {
            return false;
        }
        osDelay(1U);
    }

    return false;
}

static uint32_t log_get_time_ms(void) {
    return HAL_GetTick();
}

static const char *log_level_str(LogLevel_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO ";
        case LOG_LEVEL_WARN: return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNK  ";
    }
}

static const char *log_source_str(LogSource_t source) {
    switch (source) {
        case LOG_SRC_UNKNOWN: return "UNKNOWN";
        case LOG_SRC_APP: return "APP";
        case LOG_SRC_FSM: return "FSM";
        case LOG_SRC_SENSOR: return "SENSOR";
        case LOG_SRC_PEDAL: return "PEDAL";
        case LOG_SRC_IO: return "IO";
        case LOG_SRC_CAN: return "CAN";
        case LOG_SRC_FAULT: return "FAULT";
        case LOG_SRC_TORQUE: return "TORQUE";
        case LOG_SRC_LOG: return "LOG";
        default: return "UNK";
    }
}

static const char *log_event_str(LogEventId_t event_id) {
    switch (event_id) {
        case EVT_BOOT: return "BOOT";
        case EVT_STATE_CHANGE: return "STATE_CHANGE";
        case EVT_FAULT_SET: return "FAULT_SET";
        case EVT_FAULT_CLEAR: return "FAULT_CLEAR";
        case EVT_IO_CHANGE: return "IO_CHANGE";
        case EVT_RTD_ENTERED: return "RTD_ENTERED";
        case EVT_COMMAND_REJECTED: return "CMD_REJECTED";
        case EVT_TASK_CREATED: return "TASK_CREATED";
        case EVT_HEARTBEAT: return "HEARTBEAT";
        default: return "UNKNOWN_EVENT";
    }
}

// ---------------------------------------------------------------------------
// Sinks: target for events
// ---------------------------------------------------------------------------

// Formats the event as human-readable text and sends over UART.
static void sink_uart_event(const LogEvent_t *event) {
    char buf[UART_BUF_SIZE];
    int  len =
        snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s a0=%lu a1=%lu\r\n",
                 (unsigned long)event->time_ms, log_level_str(event->level),
                 log_source_str(event->source), log_event_str(event->event_id),
                 (unsigned long)event->a0, (unsigned long)event->a1);
    if (len <= 0) {
        return;
    }
    if ((size_t)len >= sizeof(buf)) {
        len = (int)(sizeof(buf) - 1u);
    }

    (void)log_usb_write(buf, (size_t)len);
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
    (void)log_usb_write(buf, len);
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
    LogEvent_t stamped = *event;
    stamped.time_ms = log_get_time_ms();

    sink_uart_event(&stamped);
    sink_can_event(&stamped);
    return true;
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
