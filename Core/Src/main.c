/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  *
  ******************************************************************************
  */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "main.h"

#include "cli.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* Used in the run time stats calculations. */
static unsigned long ulClocksPer10thOfAMilliSecond = 0UL;

void vMainConfigureTimerForRunTimeStats( void )
{
    /* Used by the optional run-time stats gathering functionality. */
    /* How many clocks are there per tenth of a millisecond? */
    ulClocksPer10thOfAMilliSecond = configCPU_CLOCK_HZ / 10000UL;
}

unsigned long ulMainGetRunTimeCounterValue( void )
{
    unsigned long ulSysTickCounts, ulTickCount, ulReturn;
    const unsigned long ulSysTickReloadValue = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    volatile unsigned long * const pulCurrentSysTickCount = ( ( volatile unsigned long *) 0xe000e018 );
    volatile unsigned long * const pulInterruptCTRLState = ( ( volatile unsigned long *) 0xe000ed04 );
    const unsigned long ulSysTickPendingBit = 0x04000000UL;

    /* Used by the optional run-time stats gathering functionality. */


    /* NOTE: There are potentially race conditions here.  However, it is used
    anyway to keep the examples simple, and to avoid reliance on a separate
    timer peripheral. */


    /* The SysTick is a down counter.  How many clocks have passed since it was
    last reloaded? */
    ulSysTickCounts = ulSysTickReloadValue - *pulCurrentSysTickCount;

    /* How many times has it overflowed? */
    ulTickCount = xTaskGetTickCountFromISR();

    /* This is called from the context switch, so will be called from a
    critical section.  xTaskGetTickCountFromISR() contains its own critical
    section, and the ISR safe critical sections are not designed to nest,
    so reset the critical section. */
    portSET_INTERRUPT_MASK_FROM_ISR();

    /* Is there a SysTick interrupt pending? */
    if( ( *pulInterruptCTRLState & ulSysTickPendingBit ) != 0UL ) {
        /* There is a SysTick interrupt pending, so the SysTick has overflowed
        but the tick count not yet incremented. */
        ulTickCount++;

        /* Read the SysTick again, as the overflow might have occurred since
        it was read last. */
        ulSysTickCounts = ulSysTickReloadValue - *pulCurrentSysTickCount;
    }

    /* Convert the tick count into tenths of a millisecond.  THIS ASSUMES
    configTICK_RATE_HZ is 1000! */
    ulReturn = ( ulTickCount * 10UL ) ;

    /* Add on the number of tenths of a millisecond that have passed since the
    tick count last got updated. */
    ulReturn += ( ulSysTickCounts / ulClocksPer10thOfAMilliSecond );

    return ulReturn;
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    cli_init();
    vTaskStartScheduler();

    while (1)
    {

    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure. */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 4;
    RCC_OscInitStruct.PLL.PLLN       = 168;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK   |\
                                       RCC_CLOCKTYPE_SYSCLK |\
                                       RCC_CLOCKTYPE_PCLK1  |\
                                       RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin   = B1_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : BOOT1_Pin */
    GPIO_InitStruct.Pin   = BOOT1_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin */
    GPIO_InitStruct.Pin   = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7) {
      HAL_IncTick();
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */


