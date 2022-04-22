/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  *
  *
  ******************************************************************************
  */
#include "rtc.h"
#include <stdlib.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "main.h"

RTC_HandleTypeDef hrtc;

/* RTC init function */
void RTC_Init(void)
{
	char *time     = __TIME__;
	char hours[]   = { time[0], time[1] };
	char minutes[] = { time[3], time[4] };
	char seconds[] = { time[6], time[7] };

	RTC_TimeTypeDef sTime    = {0};
	RTC_DateTypeDef sDate    = {0};

	/** Initialize RTC Only	*/
	hrtc.Instance            = RTC;
	hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv   = 127;
	hrtc.Init.SynchPrediv    = 255;
	hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}

	/** Initialize RTC and set the Time and Date */
	sTime.Hours              = (uint8_t)strtoul(hours,   NULL, 10);
	sTime.Minutes            = (uint8_t)strtoul(minutes, NULL, 10);
	sTime.Seconds            = (uint8_t)strtoul(seconds, NULL, 10);
	sTime.DayLightSaving     = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation     = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}

	sDate.WeekDay            = RTC_WEEKDAY_SATURDAY;
	sDate.Month              = RTC_MONTH_JANUARY;
	sDate.Date               = 1;
	sDate.Year               = 22;

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}
}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	if (rtcHandle->Instance == RTC) {

		/** Initializes the peripherals clock */
		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
		PeriphClkInitStruct.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;

		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
			Error_Handler();
		}

		/* RTC clock enable */
		__HAL_RCC_RTC_ENABLE();
	}
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{
	if (rtcHandle->Instance == RTC) {
		/* Peripheral clock disable */
		__HAL_RCC_RTC_DISABLE();
	}
}

void RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
	RTC_TimeTypeDef stime = {0};
	RTC_DateTypeDef sdate = {0};

	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);

	*hours   = stime.Hours;
	*minutes = stime.Minutes;
	*seconds = stime.Seconds;
}

void RTC_GetDate(uint8_t *day, uint8_t *month, uint8_t *year)
{
	RTC_TimeTypeDef stime = {0};
	RTC_DateTypeDef sdate = {0};

	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);

	*day   = sdate.Date;
	*month = sdate.Month;
	*year  = sdate.Year;
}

bool RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	RTC_TimeTypeDef stime = {0};
	RTC_DateTypeDef sdate = {0};
	bool retv = true;

	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);

	stime.Hours    = hours;
	stime.Minutes  = minutes;
	stime.Seconds  = seconds;

	if (HAL_OK != HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN)) {
		retv = false;
	}

	return retv;
}

bool RTC_SetDate(uint8_t day, uint8_t month, uint8_t year)
{
	RTC_TimeTypeDef stime = {0};
	RTC_DateTypeDef sdate = {0};
	bool retv = true;

	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);

	sdate.Date   = day;
	sdate.Month  = month;
	sdate.Year   = year;

	if (HAL_OK != HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN)) {
		retv = false;
	}

	return retv;
}

