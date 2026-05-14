#pragma once

#include <stdint.h>
#include <stddef.h>

// A CAN bus participant. Each device on the bus (INV, BMS, ...) defines one.
// rx() is called from ISR context for each message routed to that node.
typedef struct {
    const char *name;
    void (*rx)(uint32_t id, const uint8_t *data, size_t len);
} CanNode_t;
