#pragma once

#include "stdbool.h"
#include <stdint.h>
#include "vcu_io.h"


void buzzer_init(void);
void buzzer_beep(uint32_t duration_ms);
void buzzer_update(void);

// Set motor controller direction.
// Define MC_FORWARD_POLARITY_INVERTED=1 in CMakeLists to swap physical forward.
void mc_set_direction(MotorDir_t dir);
