#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "log.h" //until integrating into log.h itself

typedef struct {
    uint32_t start_ms;
    bool running;
} profiler_t;

void profiler_start(profiler_t *p);
void profiler_end(profiler_t *p, LogEventId_t event_id);