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
