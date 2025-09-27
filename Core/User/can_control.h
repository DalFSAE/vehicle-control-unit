#ifndef CAN_CONTROL.H
#define CAN_CONTROL.H

#include "stm32f4xx_hal.h"
#include "stm324x9i_eval.h"

/* Definition for CANx clock resources */
#define CANx                            CAN1
#define CANx_CLK_ENABLE()               _HAL_RCC_CAN1_CLK_ENABLE()
#define CANx_GPIO_CLK_ENALBE()          _HAL_RCC_GPIOD_CLK_ENABLE()

#define CANx_FORCE_RESET()              _HAL_RCC_CAN1_FORCE_RESET()
#define CANx_RELEASE_RESET()            _HAL_RCC_CAN1_RELEASE_RESET()

/* Definitions for CANx Pins */
#define CANx_TX_PIN                     GPIO_PIN_1
#define CANx_TX_GPIO_PORT               GPIOD
#define CANx_TX_AF                      GPIO_AF9_CAN1
#define CANx_RX_PIN                     GPIO_PIN_10
#define CANx_RX_GPIO_PORT               GPIOD
#define CANx_RX_AF                      GPIO_AF9_CAN1

/* Definition for CAN's NVIC */
#define CANx_RX_IRQn                    CAN1_RX0_IRQn
#define CANx_RX_IRQHandler              CAN1_RX0_IRQHandler

#endif /* CAN_CONTROL.H */