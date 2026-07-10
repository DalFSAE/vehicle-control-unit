#pragma once

#include "stdbool.h"
#include <stdint.h>
#include "vcu_io.h"

typedef struct {
    bool can1_terminated;
    bool can2_terminated;
} CanTermConfig_t;

void buzzer_init(void);
void buzzer_beep(uint32_t duration_ms);
void buzzer_update(void);

// Set motor controller direction.
// Define MC_FORWARD_POLARITY_INVERTED=1 in CMakeLists to swap physical forward.
void mc_set_direction(MotorDir_t dir);


// Loads config from flash (or defaults if flash is blank/invalid) and
// drives the GPIOs to match. Call once at boot, before the CAN bus is used.
void can_term_init(void);

// Returns the current in-RAM config (already applied to GPIO).
CanTermConfig_t can_term_get(void);

// Writes new values to flash and updates GPIO immediately.
// Returns true on success, false on flash write failure.
bool can_term_set(const CanTermConfig_t *cfg);