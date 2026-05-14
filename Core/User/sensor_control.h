#pragma once

#include "sensor_types.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdint.h>

#define MOCK_ADC true

void sensor_control_register_thread(osThreadId_t thread_id);
void sensorInputTask(void *argument);
void sensor_init(void);
void set_dac_out(uint32_t dacOut);

osThreadId_t sensor_task_get_handle(void);

// Getters for processed sensor state (thread safe).
float    sensor_get_throttle(void);
bool     sensor_get_brake(void);
uint32_t sensor_get_fault_flags(void);
