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
#define VBAT_EN_SW2_Pin GPIO_PIN_2
#define VBAT_EN_SW2_GPIO_Port GPIOE
#define VBAT_EN_SW3_Pin GPIO_PIN_3
#define VBAT_EN_SW3_GPIO_Port GPIOE
#define VBAT_EN_SW4_Pin GPIO_PIN_4
#define VBAT_EN_SW4_GPIO_Port GPIOE
#define CAN1_FAULT_EXTI5_Pin GPIO_PIN_5
#define CAN1_FAULT_EXTI5_GPIO_Port GPIOE
#define CAN2_FAULT_EXTI5_Pin GPIO_PIN_6
#define CAN2_FAULT_EXTI5_GPIO_Port GPIOE
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
#define PDM_OUT_Pin GPIO_PIN_3
#define PDM_OUT_GPIO_Port GPIOC
#define B1_Pin GPIO_PIN_0
#define B1_GPIO_Port GPIOA
#define ADC_VSUP_Pin GPIO_PIN_0
#define ADC_VSUP_GPIO_Port GPIOB
#define ADC_GLV_CUR_Pin GPIO_PIN_1
#define ADC_GLV_CUR_GPIO_Port GPIOB
#define BOOT1_Pin GPIO_PIN_2
#define BOOT1_GPIO_Port GPIOB
#define PWR_SW_ST2_Pin GPIO_PIN_7
#define PWR_SW_ST2_GPIO_Port GPIOE
#define PWR_SW_SW1_Pin GPIO_PIN_8
#define PWR_SW_SW1_GPIO_Port GPIOE
#define PWR_SW_EN1_Pin GPIO_PIN_9
#define PWR_SW_EN1_GPIO_Port GPIOE
#define PWR_SW_EN2_Pin GPIO_PIN_10
#define PWR_SW_EN2_GPIO_Port GPIOE
#define NMOS1_Pin GPIO_PIN_11
#define NMOS1_GPIO_Port GPIOE
#define NMOS2_Pin GPIO_PIN_12
#define NMOS2_GPIO_Port GPIOE
#define NMOS3_Pin GPIO_PIN_13
#define NMOS3_GPIO_Port GPIOE
#define NMOS4_Pin GPIO_PIN_14
#define NMOS4_GPIO_Port GPIOE
#define CLK_IN_Pin GPIO_PIN_10
#define CLK_IN_GPIO_Port GPIOB
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
#define VBUS_FS_Pin GPIO_PIN_9
#define VBUS_FS_GPIO_Port GPIOA
#define OTG_FS_ID_Pin GPIO_PIN_10
#define OTG_FS_ID_GPIO_Port GPIOA
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define DIO_EN_Pin GPIO_PIN_12
#define DIO_EN_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define CAN_SDC_RELAY_Pin GPIO_PIN_7
#define CAN_SDC_RELAY_GPIO_Port GPIOB
#define VBAT_EN_SW0_Pin GPIO_PIN_0
#define VBAT_EN_SW0_GPIO_Port GPIOE
#define VBAT_EN_SW1_Pin GPIO_PIN_1
#define VBAT_EN_SW1_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
