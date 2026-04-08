#pragma once

#include <stdbool.h>
#include <stdint.h>

// used to filter log verbosity level
#define LOG_LEVEL_DEBUG 0u
#define LOG_LEVEL_INFO 1u
#define LOG_LEVEL_WARN 2u
#define LOG_LEVEL_ERROR 3u

// Provides a common interface for logging events
typedef struct {
    uint32_t time_ms;  // time of event
    uint16_t event_id; // event id
    uint8_t  level;    // verbosity level
    uint8_t  source;   // event source
    uint32_t a0;       // logging payload
    uint32_t a1;       // logging payload
} LogEvent_t;          // 16 bytes total
_Static_assert(sizeof(LogEvent_t) == 16, "LogEvent_t must be 16 bytes");

// Starts the logging subsystem
void log_init(void);

// Primary event logger
bool log_write(const LogEvent_t *event);

// Helper for creating the struct
bool log_write_fields(uint8_t level, uint8_t source, uint16_t event_id,
                      uint32_t a0, uint32_t a1);

// Optinal plain text interface for printf style debugging
void log_printf(const char *format, ...);