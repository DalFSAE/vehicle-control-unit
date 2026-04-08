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

// LOG_MODULE must be defined before importing `log.h`:
//  #define LOG_MODULE LOG_SRC_APP
#define LOG_EVENT(level, event_id, a0, a1)                                     \
    log_event_emit(level, LOG_MODULE, event_id, a0, a1)

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
    EVT_COUNT
} LogEventId_t;

// List of firmware module that produced a log event.
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

// Internal target of LOG_EVENT(); fills timestamp and routes to all sinks
bool log_event_emit(LogLevel_t level, LogSource_t source,
                    LogEventId_t event_id, uint32_t a0, uint32_t a1);

// Optional plain text interface for printf style debugging
void log_printf(const char *format, ...);