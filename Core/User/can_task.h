#pragma once

#include "cmsis_os2.h"

#define CAN_TASK_PERIOD_MS 5U

void can_task(void *arg);
osThreadId_t can_task_get_handle(void);
