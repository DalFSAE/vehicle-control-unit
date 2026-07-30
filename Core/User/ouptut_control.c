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

// ---------------------------------------------------------------------------
// CAN termination status
// ---------------------------------------------------------------------------

static CanTermConfig_t s_cfg; // defaults to {false, false} via static zero-init

static void apply_gpio(const CanTermConfig_t *cfg) {
    dio_write(CAN1_TERMINATION, cfg->can1_terminated);
    dio_write(CAN2_TERMINATION, cfg->can2_terminated);
}

void can_term_init(void) {
    apply_gpio(&s_cfg);
}

CanTermConfig_t can_term_get(void) {
    return s_cfg;
}

bool can_term_set(const CanTermConfig_t *cfg) {
    if (cfg == NULL) return false;

    s_cfg = *cfg;
    apply_gpio(&s_cfg);
    return true;
}