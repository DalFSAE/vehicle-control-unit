#include "pedal_logic.h"
#include "vcu_io.h"
#include <math.h>

// Math helpers

float pedal_normalize(uint16_t value, uint16_t min, uint16_t max) {
    return (float)(value - min) / (float)(max - min);
}

uint32_t pedal_denormalize(float normalized, uint16_t min, uint16_t max) {
    return (uint32_t)(normalized * (max - min) + min);
}

float pedal_percent_difference(float a, float b) {
    if (a == b) return 0.0f;
    float avg = (a + b) / 2.0f;
    return fabsf(a - b) / avg;
}

float pedal_adc_to_normalized(int adcValue, float minVoltage, float maxVoltage, int adcMax) {
    const float adcRefVoltage = 3.3f; // todo: add calibration step
    float adc_voltage = (adcValue / (float)adcMax) * adcRefVoltage;
    return (adc_voltage - minVoltage) / (maxVoltage - minVoltage);
}

// Fault checks

PDP_StatusTypeDef apps_offset_check(float apps1, float apps2, float thresh) {
    return (pedal_percent_difference(apps1, apps2) >= thresh) ? PDP_ERROR : PDP_OKAY;
}

PDP_StatusTypeDef pedal_plausibility_check(pedalStatus_t *pedal,
                                           float apps, float bps,
                                           float appsLatchThresh,
                                           float bpsLatchThresh,
                                           float appsRestThresh) {
    if (apps > appsLatchThresh && bps > bpsLatchThresh) {
        return PDP_ERROR;
    } else if (pedal->latchStatus != PDP_OKAY && apps < appsRestThresh) {
        return PDP_OKAY; // latch resets
    } else if (pedal->latchStatus != PDP_OKAY && apps > appsLatchThresh) {
        return PDP_ERROR; // waiting for latch to reset
    }
    return pedal->latchStatus;
}

PDP_StatusTypeDef sensor_out_of_range(float normalizedValue, float minRange, float maxRange) {
    return (normalizedValue < minRange || normalizedValue > maxRange) ? PDP_ERROR : PDP_OKAY;
}

// Orchestrator

uint32_t pedal_check_faults(pedalStatus_t *status, SensorInfo_t *sensors, int numSensors) {
    static const float appsLatchThresh = 0.4f;
    static const float bpsLatchThresh  = 0.1f;
    static const float appsResetThresh = 0.3f;
    static const float minRange        = -0.1f;
    static const float maxRange        =  1.1f;

    status->offsetStatus = apps_offset_check(
        sensors[APPS1].normalizedValue,
        sensors[APPS2].normalizedValue,
        0.2f);

    status->latchStatus = pedal_plausibility_check(
        status,
        sensors[APPS1].normalizedValue,
        sensors[FBPS].normalizedValue,
        appsLatchThresh, bpsLatchThresh, appsResetThresh);

    status->sensorStatus = PDP_OKAY;
    for (int i = 0; i < numSensors; i++) {
        if (sensor_out_of_range(sensors[i].normalizedValue, minRange, maxRange) == PDP_ERROR) {
            status->sensorStatus = PDP_ERROR;
            break;
        }
    }

    uint32_t flags = FAULT_NONE;
    if (status->offsetStatus != PDP_OKAY) flags |= FAULT_APPS_DISAGREE;
    if (status->latchStatus  != PDP_OKAY) flags |= FAULT_PEDAL_PLAUS;
    if (status->sensorStatus != PDP_OKAY) flags |= FAULT_SENSOR_RANGE;
    return flags;
}
