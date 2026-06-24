#include "runtime_profiler.h"
#include "log.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

void profiler_start(profiler_t *p) {
    if (!p) {
        return;
    }

    p->start_ms = HAL_GetTick();
    p->running  = true;
}

void profiler_end(profiler_t *p, LogEventId_t event_id, uint32_t a0,
                  uint32_t a1) { // including a1 to match log.h architecture
    if (!p || !p->running) {
        return;
    }

    uint32_t end     = HAL_GetTick();
    uint32_t elapsed = end - p->start_ms;

    p->running = false;

    LOG_EVENT(LOG_LEVEL_INFO, event_id, elapsed, a0);
}