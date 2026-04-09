#pragma once

#include <stdbool.h>
#include "vehicle_state.h"

bool read_dash_switch_filtered(void);
bool read_ready_to_drive_button(void);
bool read_forward_switch(void);

void get_driver_inputs(VcuState_t *v);