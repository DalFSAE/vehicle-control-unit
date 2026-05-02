
#include "cmsis_os2.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LOG_MODULE LOG_SRC_LOG
#include "log.h"
#include "sensor_types.h"

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

static bool               s_log_initialized = false;
static osMessageQueueId_t s_log_queue;

// ---------------------------------------------------------------------------
// log_putchar
// ---------------------------------------------------------------------------

#define LOG_PUTCHAR_LINE_LEN 240U
#define LOG_PUTCHAR_MAX_LINES 64U

static char    s_line_buf[LOG_PUTCHAR_LINE_LEN];
static uint8_t s_line_len = 0U;

// This is a simple way to capture early logs without dynamic memory allocation 
// or complex buffering logic. Used for tracking logs generated before log_init() 
static char    s_pre_init_lines[LOG_PUTCHAR_MAX_LINES][LOG_PUTCHAR_LINE_LEN];
static uint8_t s_pre_init_line_count = 0U;

// Flushes the current line buffer to the appropriate sink 
// (USB if initialized, otherwise pre-init buffer).
static void putchar_flush_line(void) {
    if (s_line_len == 0U) {
        return;
    }
    s_line_buf[s_line_len] = '\0';

    if (s_log_initialized) {
        log_printf("%s\r\n", s_line_buf);
    } else if (s_pre_init_line_count < LOG_PUTCHAR_MAX_LINES) {
        memcpy(s_pre_init_lines[s_pre_init_line_count], s_line_buf, s_line_len + 1U);
        s_pre_init_line_count++;
    }

    s_line_len = 0U;
}

// Used for line-buffered char sink (i.e. Unity)
void log_putchar(int c) {
    if (c == '\n') {
        putchar_flush_line();
    } else if (c != '\r') {
        if (s_line_len < LOG_PUTCHAR_LINE_LEN - 1U) {
            s_line_buf[s_line_len++] = (char)c;
        }
    }
}

// Writes a log message to the USB CDC queue.
static bool log_usb_write(const char *buf, size_t len) {
    if (!s_log_initialized) {
        return false;
    }

    LogMsg_t msg;

    if (len > sizeof(msg.data)) {
        len = sizeof(msg.data);
    }

    memcpy(msg.data, buf, len);
    msg.len = len;

    return (osMessageQueuePut(s_log_queue, &msg, 0, 0) == osOK);
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
        case LOG_SRC_MC:    return "MC";
        case LOG_SRC_LOG:   return "LOG";
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
        case EVT_SENSOR_DEBUG: return "SENSOR_DEBUG";
        case EVT_RTD_ENTERED: return "RTD_ENTERED";
        case EVT_COMMAND_REJECTED: return "CMD_REJECTED";
        case EVT_TASK_CREATED: return "TASK_CREATED";
        case EVT_HEARTBEAT: return "HEARTBEAT";
        default: return "UNKNOWN_EVENT";
    }
}

static const char *log_sensor_channel_str(uint32_t sensor_channel) {
    switch ((SensorType_t)sensor_channel) {
        case APPS1: return "APPS1";
        case APPS2: return "APPS2";
        case FBPS: return "FBPS ";
        case RBPS: return "RBPS ";
        case CUR: return "CUR  ";
        default: return "ERROR";
    }
}

// Keep in sync with FaultFlags_t in vehicle_state.h.
static const char *log_fault_flag_str(uint32_t flag) {
    switch (flag) {
        case (1u << 0): return "APPS_DISAGREE";
        case (1u << 1): return "PEDAL_PLAUS";
        case (1u << 2): return "SENSOR_RANGE";
        default: return "UNKNOWN_FAULT";
    }
}

// Keep in sync with FaultResponse_t in fsm.h.
static const char *log_fault_resp_str(uint32_t resp) {
    switch (resp) {
        case 0u: return "CUT_THROTTLE";
        case 1u: return "RETURN_NEUTRAL";
        default: return "UNKNOWN_RESP";
    }
}

static const char *log_fsm_state_str(uint32_t state) {
    // Keep in sync with FsmState_t in fsm.c.
    switch (state) {
        case 0u: return "ENTRY";
        case 1u: return "STANDBY";
        case 2u: return "NEUTRAL";
        case 3u: return "FORWARD";
        case 4u: return "REVERSE";
        default: return "UNKNOWN";
    }
}

// Keep in sync with Rinehart PM100 VSM state table.
static const char *log_mc_vsm_state_str(uint32_t state) {
    switch (state) {
        case 0u: return "START";
        case 1u: return "PRECHARGE_INIT";
        case 2u: return "PRECHARGE_ACTIVE";
        case 3u: return "PRECHARGE_COMPLETE";
        case 4u: return "WAIT";
        case 5u: return "READY";
        case 6u: return "RUNNING";
        case 7u: return "FAULT";
        default: return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Sinks: target for events
// ---------------------------------------------------------------------------

// Formats the event as human-readable text and sends over UART.
static void sink_uart_event(const LogEvent_t *event) {
    char buf[UART_BUF_SIZE];
    int  len;

    if (event->event_id == EVT_SENSOR_DEBUG) {
        len = snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s channel=%s value=%lu\r\n",
                       (unsigned long)event->time_ms, log_level_str(event->level), log_source_str(event->source),
                       log_event_str(event->event_id), log_sensor_channel_str(event->a0), (unsigned long)event->a1);
    } else if (event->event_id == EVT_FAULT_SET || event->event_id == EVT_FAULT_CLEAR) {
        if (event->source == LOG_SRC_MC) {
            len = snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s faults 0x%04lX -> 0x%04lX\r\n",
                           (unsigned long)event->time_ms, log_level_str(event->level),
                           log_source_str(event->source), log_event_str(event->event_id),
                           (unsigned long)event->a0, (unsigned long)event->a1);
        } else {
            len = snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s %s -> %s\r\n",
                           (unsigned long)event->time_ms, log_level_str(event->level),
                           log_source_str(event->source), log_event_str(event->event_id),
                           log_fault_flag_str(event->a0), log_fault_resp_str(event->a1));
        }
    } else if (event->event_id == EVT_STATE_CHANGE) {
        if (event->source == LOG_SRC_MC) {
            len = snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s %s -> %s\r\n",
                           (unsigned long)event->time_ms, log_level_str(event->level),
                           log_source_str(event->source), log_event_str(event->event_id),
                           log_mc_vsm_state_str(event->a0), log_mc_vsm_state_str(event->a1));
        } else {
            len = snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s %s -> %s\r\n",
                           (unsigned long)event->time_ms, log_level_str(event->level),
                           log_source_str(event->source), log_event_str(event->event_id),
                           log_fsm_state_str(event->a0), log_fsm_state_str(event->a1));
        }
    } else {
        len = snprintf(buf, sizeof(buf), "[%8lu] %-5s %-6s %-14s a0=%lu a1=%lu\r\n", (unsigned long)event->time_ms,
                       log_level_str(event->level), log_source_str(event->source), log_event_str(event->event_id),
                       (unsigned long)event->a0, (unsigned long)event->a1);
    }

    if (len <= 0) {
        return;
    }
    if ((size_t)len >= sizeof(buf)) {
        len = (int)(sizeof(buf) - 1u);
    }

    (void)log_usb_write(buf, (size_t)len);
}

// Encodes the event into a compact 8-byte CAN payload (no raw struct).
static void sink_can_event(const LogEvent_t *event) {
    // Only allow valid events to be sent over CAN
    switch (event->event_id) {
        case EVT_FAULT_SET:
        case EVT_STATE_CHANGE:
        case EVT_BOOT: break;
        default: return;
    }

    uint8_t payload[CAN_PAYLOAD_SIZE];
    payload[0] = (uint8_t)event->event_id;
    payload[1] = (uint8_t)event->source;
    payload[2] = (uint8_t)((event->a0 >> 8u) & 0xFFu);
    payload[3] = (uint8_t)(event->a0 & 0xFFu);
    payload[4] = (uint8_t)((event->a1 >> 8u) & 0xFFu);
    payload[5] = (uint8_t)(event->a1 & 0xFFu);
    payload[6] = 0u;
    payload[7] = 0u;

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

bool log_init(void) {
    const int pre_boot_msg_count = 24; // warning: large values will consume RAM due to static allocation
    s_log_queue = osMessageQueueNew(pre_boot_msg_count, sizeof(LogMsg_t), NULL);
    if (s_log_queue == NULL) {
        return false;
    }
    s_log_initialized = true;

    for (uint8_t i = 0; i < s_pre_init_line_count; i++) {
        log_printf("%s\r\n", s_pre_init_lines[i]);
    }
    s_pre_init_line_count = 0U;

    return true;
}

bool log_write(const LogEvent_t *event) {
    if ((event == NULL) || !s_log_initialized || (event->level < LOG_MIN_LEVEL)) {
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

void log_usb_task(void *argument) {
    (void)argument;
    osDelay(1000); // wait for USB host enumeration before transmitting
    LogMsg_t msg;
    for (;;) {
        if (osMessageQueueGet(s_log_queue, &msg, NULL, osWaitForever) == osOK) {
            // Wait until USB is free
            while (CDC_Transmit_FS((uint8_t *)msg.data, msg.len) == USBD_BUSY) {
                osDelay(1);
            }

            // Wait until TX complete
            while (CDC_IsTxBusy()) {
                osDelay(1);
            }
        }
    }
}
