#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PDP_OKAY    = 0x00U,
    PDP_ERROR   = 0x01U,
    PDP_TIMEOUT = 0x03U,
    PDP_LATCH   = 0x04U
} PDP_StatusTypeDef;

typedef struct {
    const char *name;       // sensor name
    float       voltageMin; // minimum nominal voltage
    float       voltageMax; // maximum nominal voltage
    int         currentAdcValue;   // current raw ADC reading
    float       normalizedValue;   // normalized [0.0, 1.0]
} SensorInfo_t;

typedef enum {
    APPS1,
    APPS2,
    FBPS,
    RBPS,
    CUR,
    NUM_SENSORS
} SensorType_t;
