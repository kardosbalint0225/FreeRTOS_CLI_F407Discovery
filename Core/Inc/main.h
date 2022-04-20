/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  *
  *
  ******************************************************************************
  */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);


/* Private defines -----------------------------------------------------------*/
#define PH0_OSC_IN_Pin 			GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port 	GPIOH
#define PH1_OSC_OUT_Pin 		GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port 	GPIOH
#define B1_Pin 					GPIO_PIN_0
#define B1_GPIO_Port 			GPIOA
#define B1_EXTI_IRQn 			EXTI0_IRQn
#define BOOT1_Pin 				GPIO_PIN_2
#define BOOT1_GPIO_Port 		GPIOB
#define LD4_Pin 				GPIO_PIN_12
#define LD4_GPIO_Port 			GPIOD
#define LD3_Pin 				GPIO_PIN_13
#define LD3_GPIO_Port 			GPIOD
#define LD5_Pin 				GPIO_PIN_14
#define LD5_GPIO_Port 			GPIOD
#define LD6_Pin 				GPIO_PIN_15
#define LD6_GPIO_Port 			GPIOD
#define SWDIO_Pin 				GPIO_PIN_13
#define SWDIO_GPIO_Port 		GPIOA
#define SWCLK_Pin 				GPIO_PIN_14
#define SWCLK_GPIO_Port 		GPIOA


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
