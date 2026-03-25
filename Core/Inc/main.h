/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define INV_NEUTRAL_Pin GPIO_PIN_2
#define INV_NEUTRAL_GPIO_Port GPIOE
#define RTD_BUTTON_Pin GPIO_PIN_3
#define RTD_BUTTON_GPIO_Port GPIOE
#define INV_FORWARD_Pin GPIO_PIN_4
#define INV_FORWARD_GPIO_Port GPIOE
#define RTD_Buzzer_Pin GPIO_PIN_5
#define RTD_Buzzer_GPIO_Port GPIOE
#define PC14_OSC32_IN_Pin GPIO_PIN_14
#define PC14_OSC32_IN_GPIO_Port GPIOC
#define PC15_OSC32_OUT_Pin GPIO_PIN_15
#define PC15_OSC32_OUT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define OTG_FS_PowerSwitchOn_Pin GPIO_PIN_0
#define OTG_FS_PowerSwitchOn_GPIO_Port GPIOC
#define B1_Pin GPIO_PIN_0
#define B1_GPIO_Port GPIOA
#define BOOT1_Pin GPIO_PIN_2
#define BOOT1_GPIO_Port GPIOB
#define CAN1_TERMINATION_Pin GPIO_PIN_7
#define CAN1_TERMINATION_GPIO_Port GPIOE
#define CAN2_TERMINATION_Pin GPIO_PIN_8
#define CAN2_TERMINATION_GPIO_Port GPIOE
#define MOSFET1_Pin GPIO_PIN_11
#define MOSFET1_GPIO_Port GPIOE
#define MOSFET2_Pin GPIO_PIN_13
#define MOSFET2_GPIO_Port GPIOE
#define MOSFET3_Pin GPIO_PIN_15
#define MOSFET3_GPIO_Port GPIOE
#define CLK_IN_Pin GPIO_PIN_10
#define CLK_IN_GPIO_Port GPIOB
#define CAN2_FLT_Pin GPIO_PIN_11
#define CAN2_FLT_GPIO_Port GPIOB
#define LD4_Pin GPIO_PIN_12
#define LD4_GPIO_Port GPIOD
#define LD3_Pin GPIO_PIN_13
#define LD3_GPIO_Port GPIOD
#define LD5_Pin GPIO_PIN_14
#define LD5_GPIO_Port GPIOD
#define LD6_Pin GPIO_PIN_15
#define LD6_GPIO_Port GPIOD
#define I2S3_MCK_Pin GPIO_PIN_7
#define I2S3_MCK_GPIO_Port GPIOC
#define TSSI_EN_Pin GPIO_PIN_8
#define TSSI_EN_GPIO_Port GPIOA
#define VBUS_FS_Pin GPIO_PIN_9
#define VBUS_FS_GPIO_Port GPIOA
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define CAN1_FLT_Pin GPIO_PIN_11
#define CAN1_FLT_GPIO_Port GPIOC
#define PWR_INV_Pin GPIO_PIN_2
#define PWR_INV_GPIO_Port GPIOD
#define Audio_RST_Pin GPIO_PIN_4
#define Audio_RST_GPIO_Port GPIOD
#define OTG_FS_OverCurrent_Pin GPIO_PIN_5
#define OTG_FS_OverCurrent_GPIO_Port GPIOD
#define PWR_FANS_Pin GPIO_PIN_6
#define PWR_FANS_GPIO_Port GPIOD
#define SDC_RELAY_Pin GPIO_PIN_5
#define SDC_RELAY_GPIO_Port GPIOB
#define PWR_AUX_Pin GPIO_PIN_6
#define PWR_AUX_GPIO_Port GPIOB
#define PWR_ALWAYSON_Pin GPIO_PIN_7
#define PWR_ALWAYSON_GPIO_Port GPIOB
#define PWR_BL_Pin GPIO_PIN_9
#define PWR_BL_GPIO_Port GPIOB
#define TSSI_FLT_Pin GPIO_PIN_0
#define TSSI_FLT_GPIO_Port GPIOE
#define MEMS_INT2_Pin GPIO_PIN_1
#define MEMS_INT2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
