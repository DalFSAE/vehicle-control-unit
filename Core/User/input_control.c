#include "input_control.h"
#include "main.h"
#include "dio.h"

// ---------------------------------------------------------------------------
// Simple 1-bit software debounce + asymmetric OFF delay
// ---------------------------------------------------------------------------

#define SWITCH_ON_LEVEL 0
#define SWITCH_OFF_LEVEL 1
#define DEBOUNCE_SAMPLES 4 // 4 x 10 ms = 40 ms
#define OFF_HOLDOFF_MS 200 // must stay OFF this long

static bool     sw_stable = false;
static uint8_t  sw_cnt = 0;
static uint32_t off_start_ms = 0;

bool read_dash_switch_filtered(void) {
    bool raw = dio_read(DASH_SWITCH);

    if (raw == sw_stable) {
        sw_cnt = 0;
    } else if (++sw_cnt >= DEBOUNCE_SAMPLES) {
        sw_stable = raw;
        sw_cnt = 0;
        if (sw_stable == SWITCH_OFF_LEVEL)
            off_start_ms = HAL_GetTick();
    }

    if (sw_stable == SWITCH_OFF_LEVEL) {
        if (HAL_GetTick() - off_start_ms < OFF_HOLDOFF_MS)
            return SWITCH_ON_LEVEL;
    }

    return sw_stable;
}

bool read_ready_to_drive_button(void) {
    return dio_read(DASH_RTD_BUTTON);
}

bool read_forward_switch(void) {
    return dio_read(DASH_SWITCH);
}

bool read_pcb_user_button(void) {
    return dio_read(PCB_USER_BUTTON);
}
