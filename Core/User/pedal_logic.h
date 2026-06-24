#pragma once

#include "sensor_types.h"
#include <stdbool.h>
#include <stdint.h>

// Stateful latch for pedal fault checks. Caller owns storage (lives in sensor task).
typedef struct {
    PDP_StatusTypeDef offsetStatus; // APPS agreement
    PDP_StatusTypeDef latchStatus;  // pedal plausibility latch
    PDP_StatusTypeDef sensorStatus; // out-of-range
} pedalStatus_t;


// Math helpers
float    pedal_normalize(uint16_t value, uint16_t min, uint16_t max);
uint32_t pedal_denormalize(float normalized, uint16_t min, uint16_t max);
float    pedal_percent_difference(float a, float b);
float    pedal_adc_to_normalized(int adcValue, float minVoltage, float maxVoltage, int adcMax);

// Fault checks
PDP_StatusTypeDef apps_offset_check(float apps1, float apps2, float thresh);
PDP_StatusTypeDef pedal_plausibility_check(pedalStatus_t *pedal,
                                           float apps, float bps,
                                           float appsLatchThresh,
                                           float bpsLatchThresh,
                                           float appsRestThresh);
PDP_StatusTypeDef sensor_out_of_range(float normalizedValue, float minRange, float maxRange);

// Orchestrator: runs all checks, updates *status, returns FaultFlags_t bitmask.
uint32_t pedal_check_faults(pedalStatus_t *status, SensorInfo_t *sensors, int numSensors);
