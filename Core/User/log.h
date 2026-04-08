#pragma once

#include <stdbool.h>
#include <stdint.h>

// used to filter log verbosity level
typedef enum {
    LOG_LEVEL_DEBUG = 0u,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel_t;

// List of possible firmware events
typedef enum {
    EVT_BOOT = 0u,
    EVT_STATE_CHANGE,
    EVT_FAULT_SET,
    EVT_FAULT_CLEAR,
    EVT_IO_CHANGE,
    EVT_RTD_ENTERED,
    EVT_COMMAND_REJECTED,
    EVT_TASK_CREATED,
    EVT_HEARTBEAT,
    EVT_COUNT // must be last
} LogEventId_t;

// List of firmware modules that can produce log events.
// Define LOG_MODULE in each .c file before including log.h:
//   #define LOG_MODULE LOG_SRC_FSM
typedef enum {
    LOG_SRC_UNKNOWN = 0u, // fallback
    LOG_SRC_APP,
    LOG_SRC_FSM,
    LOG_SRC_SENSOR,
    LOG_SRC_PEDAL,
    LOG_SRC_IO,
    LOG_SRC_CAN,
    LOG_SRC_FAULT,
    LOG_SRC_TORQUE,
    LOG_SRC_LOG,  // log.c itself
    LOG_SRC_COUNT // keep last
} LogSource_t;

// An event is a compact, structured record of something that happened at a
// specific point in time within the firmware.
// Provides a common interface for logging events
typedef struct {
    uint32_t     time_ms;  // time of event
    LogEventId_t event_id; // event id
    LogLevel_t   level;    // verbosity level
    LogSource_t  source;   // event source
    uint32_t     a0;       // logging payload
    uint32_t     a1;       // logging payload
} LogEvent_t;

// Starts the logging subsystem
void log_init(void);

// Primary event logger — auto-fills timestamp, routes to all sinks
bool log_write(const LogEvent_t *event);

// Optional plain-text printf-style interface for debug output
void log_printf(const char *format, ...);

// Convenience macro — requires LOG_MODULE to be defined before including this header.
// Falls back to LOG_SRC_UNKNOWN if LOG_MODULE is not defined.
#ifndef LOG_MODULE
#define LOG_MODULE LOG_SRC_UNKNOWN
#endif

#define LOG_EVENT(log_level_value, log_event_id_value, arg0_value, arg1_value) \
    log_write(&(LogEvent_t){                                                   \
        .event_id = (log_event_id_value),                                      \
        .level    = (log_level_value),                                         \
        .source   = LOG_MODULE,                                                \
        .a0       = (arg0_value),                                              \
        .a1       = (arg1_value),                                              \
    })
