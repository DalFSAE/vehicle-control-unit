#pragma once

#include "cmsis_os2.h"

#define CAN_TASK_PERIOD_MS 5U

void can_task(void *arg);
void can_task_register_handle(osThreadId_t handle);
osThreadId_t can_task_get_handle(void);
