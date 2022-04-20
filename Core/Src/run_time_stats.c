/*
 * run_time_stats.c
 *
 *  Created on: 2022. apr. 20.
 *      Author: Balint
 */
#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim2;
volatile uint32_t runtime_stats_timer;

void TIM2_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

void vConfigureTimerForRunTimeStats( void )
{
	RCC_ClkInitTypeDef    clkconfig;
	uint32_t              uwTimclock = 0;
	uint32_t              uwPrescalerValue = 0;
	uint32_t              pFLatency;

	runtime_stats_timer = 0;

	/*Configure the TIM7 IRQ priority */
	HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);

	/* Enable the TIM7 global Interrupt */
	HAL_NVIC_EnableIRQ(TIM2_IRQn);

	/* Enable TIM2 clock */
	__HAL_RCC_TIM2_CLK_ENABLE();

	/* Get clock configuration */
	HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

	/* Compute TIM2 clock */
	uwTimclock = 2*HAL_RCC_GetPCLK1Freq();
	/* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
	uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

	/* Initialize TIM2 */
	htim2.Instance = TIM2;

	/* Initialize TIMx peripheral as follow:
	 *
	+ Period = [(TIM2CLK/10000) - 1]. to have a (1/10000) s time base.

	+ Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
	+ ClockDivision = 0
	+ Counter direction = Up
	*/
	htim2.Init.Period = (1000000U / 10000U) - 1U;
	htim2.Init.Prescaler = uwPrescalerValue;
	htim2.Init.ClockDivision = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;

	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		while (1);

	}

	HAL_TIM_RegisterCallback(&htim2, HAL_TIM_PERIOD_ELAPSED_CB_ID, TIM2_PeriodElapsedCallback);

	/* Start the TIM time Base generation in interrupt mode */
	HAL_TIM_Base_Start_IT(&htim2);
}

void TIM2_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	runtime_stats_timer++;
}




