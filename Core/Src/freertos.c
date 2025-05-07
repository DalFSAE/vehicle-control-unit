/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for status_leds */
osThreadId_t status_ledsHandle;
const osThreadAttr_t status_leds_attributes = {
  .name = "status_leds",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensor_input */
osThreadId_t sensor_inputHandle;
const osThreadAttr_t sensor_input_attributes = {
  .name = "sensor_input",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};
/* Definitions for state_machine */
osThreadId_t state_machineHandle;
const osThreadAttr_t state_machine_attributes = {
  .name = "state_machine",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void statusLedsTask(void *argument);
void sensorInputTask(void *argument);
void stateMachineTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of status_leds */
  status_ledsHandle = osThreadNew(statusLedsTask, NULL, &status_leds_attributes);

  /* creation of sensor_input */
  sensor_inputHandle = osThreadNew(sensorInputTask, NULL, &sensor_input_attributes);

  /* creation of state_machine */
  state_machineHandle = osThreadNew(stateMachineTask, NULL, &state_machine_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_statusLedsTask */
/**
  * @brief  Function implementing the status_leds thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_statusLedsTask */
__weak void statusLedsTask(void *argument)
{
  /* USER CODE BEGIN statusLedsTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END statusLedsTask */
}

/* USER CODE BEGIN Header_sensorInputTask */
/**
* @brief Function implementing the sensor_input thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_sensorInputTask */
__weak void sensorInputTask(void *argument)
{
  /* USER CODE BEGIN sensorInputTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END sensorInputTask */
}

/* USER CODE BEGIN Header_stateMachineTask */
/**
* @brief Function implementing the state_machine thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_stateMachineTask */
__weak void stateMachineTask(void *argument)
{
  /* USER CODE BEGIN stateMachineTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END stateMachineTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

