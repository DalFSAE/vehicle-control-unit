#include "output_control.h"

#include "dio.h"

#include "stdbool.h"
#include "main.h"

// ---------------------------------------------------------------------------
// Motor controls
// ---------------------------------------------------------------------------

void mc_set_direction(MotorDir_t dir) {
#if MC_FORWARD_POLARITY_INVERTED
    bool want_forward = (dir == MOTOR_DIR_REVERSE);
#else
    bool want_forward = (dir == MOTOR_DIR_FORWARD);
#endif
    // MC forward switch is active-low: false = forward
    dio_write(MC_FORWARD_SW, !want_forward);
}

// ---------------------------------------------------------------------------
// Buzzer Controls
// ---------------------------------------------------------------------------

static uint32_t _beep_start;
static uint32_t _beep_duration;
static bool     _beep_active;

void buzzer_init(void) {
    dio_write(BUZZER, false);
    _beep_active = false;
}

void buzzer_beep(uint32_t duration_ms) {
    _beep_start = HAL_GetTick();
    _beep_duration = duration_ms;
    _beep_active = true;
    dio_write(BUZZER, true);
}

void buzzer_update(void) {
    if (_beep_active && (HAL_GetTick() - _beep_start >= _beep_duration)) {
        dio_write(BUZZER, false);
        _beep_active = false;
    }
}
