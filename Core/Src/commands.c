/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

 /******************************************************************************
 *
 * See the following URL for information on the commands defined in this file:
 * http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/Embedded_Ethernet_Examples/Ethernet_Related_CLI_Commands.shtml
 *
 ******************************************************************************/


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

#include "rtc.h"

#ifndef  configINCLUDE_TRACE_RELATED_CLI_COMMANDS
	#define configINCLUDE_TRACE_RELATED_CLI_COMMANDS 0
#endif


/*
 * Implements the run-time-stats command.
 */
static portBASE_TYPE prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the task-stats command.
 */
static portBASE_TYPE prvRunTimeStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the echo-three-parameters command.
 */
static portBASE_TYPE prvThreeParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the echo-parameters command.
 */
static portBASE_TYPE prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the "trace start" and "trace stop" commands;
 */
#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
	static portBASE_TYPE prvStartStopTraceCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif


static portBASE_TYPE get_kernel_version( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE get_date( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE get_time( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE set_date( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE set_time( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

static void convert_time_to_string(uint8_t hours, uint8_t minutes, uint8_t seconds, char *time_string);
static void convert_date_to_string(uint8_t day, uint8_t month, uint8_t year, char *date_string);
static void convert_string_to_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, char *time_string);
static void convert_string_to_date(uint8_t *day, uint8_t *month, uint8_t *year, char *date_string);
static bool is_number(char s);
static bool is_time_command_string_valid(char *time_string, BaseType_t len);
static bool is_date_command_string_valid(char *date_string, BaseType_t len);

/* Structure that defines the "run-time-stats" command line command.   This
generates a table that shows how much run time each task has */
static const CLI_Command_Definition_t xRunTimeStats =
{
	"run-time-stats", /* The command string to type. */
	"\r\nrun-time-stats:\r\n Displays a table showing how much processing time each FreeRTOS task has used\r\n",
	prvRunTimeStatsCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command.  This generates
a table that gives information on each task in the system. */
static const CLI_Command_Definition_t xTaskStats =
{
	"task-stats", /* The command string to type. */
	"\r\ntask-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n",
	prvTaskStatsCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "echo_3_parameters" command line command.  This
takes exactly three parameters that the command simply echos back one at a
time. */
static const CLI_Command_Definition_t xThreeParameterEcho =
{
	"echo-3-parameters",
	"\r\necho-3-parameters <param1> <param2> <param3>:\r\n Expects three parameters, echos each in turn\r\n",
	prvThreeParameterEchoCommand, /* The function to run. */
	3 /* Three parameters are expected, which can take any value. */
};

/* Structure that defines the "echo_parameters" command line command.  This
takes a variable number of parameters that the command simply echos back one at
a time. */
static const CLI_Command_Definition_t xParameterEcho =
{
	"echo-parameters",
	"\r\necho-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n",
	prvParameterEchoCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
	/* Structure that defines the "trace" command line command.  This takes a single
	parameter, which can be either "start" or "stop". */
	static const CLI_Command_Definition_t xStartStopTrace =
	{
		"trace",
		"\r\ntrace [start | stop]:\r\n Starts or stops a trace recording for viewing in FreeRTOS+Trace\r\n",
		prvStartStopTraceCommand, /* The function to run. */
		1 /* One parameter is expected.  Valid values are "start" and "stop". */
	};
#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */


static const CLI_Command_Definition_t kernel_version =
{
	"kernel-version",
	"\r\nkernel-version:\r\n Displays the FreeRTOS kernel version number\r\n",
	get_kernel_version,
	0
};


static const CLI_Command_Definition_t get_date_cmd = 
{
	"date",
	"\r\ndate:\r\n Displays the date in dd/mm/yy format\r\n",
	get_date,
	0
};


static const CLI_Command_Definition_t get_time_cmd =
{
	"time",
	"\r\ntime:\r\n Displays the time in hh:mm:ss format\r\n",
	get_time,
	0
};

static const CLI_Command_Definition_t set_date_cmd = 
{
	"set-date",
	"\r\nset-date <dd/mm/yy>:\r\n Sets the current date\r\n",
	set_date,
	1
};

static const CLI_Command_Definition_t set_time_cmd = 
{
	"set-time",
	"\r\nset-time <hh:mm:ss>:\r\n Sets the current time\r\n",
	set_time,
	1
};


/*-----------------------------------------------------------*/

void vRegisterSampleCLICommands( void )
{
	/* Register all the command line commands defined immediately above. */
	FreeRTOS_CLIRegisterCommand( &xTaskStats );
	FreeRTOS_CLIRegisterCommand( &xRunTimeStats );
	FreeRTOS_CLIRegisterCommand( &xThreeParameterEcho );
	FreeRTOS_CLIRegisterCommand( &xParameterEcho );
	FreeRTOS_CLIRegisterCommand( &kernel_version );
	FreeRTOS_CLIRegisterCommand( &get_date_cmd );
	FreeRTOS_CLIRegisterCommand( &get_time_cmd );
	FreeRTOS_CLIRegisterCommand( &set_date_cmd );
	FreeRTOS_CLIRegisterCommand( &set_time_cmd );

	#if( configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1 )
	{
		FreeRTOS_CLIRegisterCommand( & xStartStopTrace );
	}
	#endif
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *const pcHeader = "Task          State  Priority  Stack	#\r\n************************************************\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Generate a table of task stats. */
	strcpy( pcWriteBuffer, pcHeader );
	vTaskList( pcWriteBuffer + strlen( pcHeader ) );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvRunTimeStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char * const pcHeader = "Task            Abs Time      % Time\r\n****************************************\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Generate a table of task stats. */
	strcpy( pcWriteBuffer, pcHeader );
	vTaskGetRunTimeStats( pcWriteBuffer + strlen( pcHeader ) );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvThreeParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *pcParameter;
portBASE_TYPE xParameterStringLength, xReturn;
static portBASE_TYPE lParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( lParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		sprintf( pcWriteBuffer, "The three parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
							(
								pcCommandString,		/* The command string itself. */
								lParameterNumber,		/* Return the next parameter. */
								&xParameterStringLength	/* Store the parameter string length. */
							);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* Return the parameter string. */
		memset( pcWriteBuffer, 0x00, xWriteBufferLen );
		sprintf( pcWriteBuffer, "%d: ", ( int ) lParameterNumber );
		strncat( pcWriteBuffer, pcParameter, xParameterStringLength );
		strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

		/* If this is the last of the three parameters then there are no more
		strings to return after this one. */
		if( lParameterNumber == 3L )
		{
			/* If this is the last of the three parameters then there are no more
			strings to return after this one. */
			xReturn = pdFALSE;
			lParameterNumber = 0L;
		}
		else
		{
			/* There are more parameters to return after this one. */
			xReturn = pdTRUE;
			lParameterNumber++;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *pcParameter;
portBASE_TYPE xParameterStringLength, xReturn;
static portBASE_TYPE lParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( lParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		sprintf( pcWriteBuffer, "The parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
							(
								pcCommandString,		/* The command string itself. */
								lParameterNumber,		/* Return the next parameter. */
								&xParameterStringLength	/* Store the parameter string length. */
							);

		if( pcParameter != NULL )
		{
			/* Return the parameter string. */
			memset( pcWriteBuffer, 0x00, xWriteBufferLen );
			sprintf( pcWriteBuffer, "%d: ", ( int ) lParameterNumber );
			strncat( pcWriteBuffer, pcParameter, xParameterStringLength );
			strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

			/* There might be more parameters to return after this one. */
			xReturn = pdTRUE;
			lParameterNumber++;
		}
		else
		{
			/* No more parameters were found.  Make sure the write buffer does
			not contain a valid string. */
			pcWriteBuffer[ 0 ] = 0x00;

			/* No more data to return. */
			xReturn = pdFALSE;

			/* Start over the next time this command is executed. */
			lParameterNumber = 0;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1

	static portBASE_TYPE prvStartStopTraceCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
	{
	const char *pcParameter;
	portBASE_TYPE lParameterStringLength;

		/* Remove compile time warnings about unused parameters, and check the
		write buffer is not NULL.  NOTE - for simplicity, this example assumes the
		write buffer length is adequate, so does not check for buffer overflows. */
		( void ) pcCommandString;
		( void ) xWriteBufferLen;
		configASSERT( pcWriteBuffer );

		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
							(
								pcCommandString,		/* The command string itself. */
								1,						/* Return the first parameter. */
								&lParameterStringLength	/* Store the parameter string length. */
							);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* There are only two valid parameter values. */
		if( strncmp( pcParameter, "start", strlen( "start" ) ) == 0 )
		{
			/* Start or restart the trace. */
			vTraceStop();
			vTraceClear();
			vTraceStart();

			sprintf( pcWriteBuffer, "Trace recording (re)started.\r\n" );
		}
		else if( strncmp( pcParameter, "stop", strlen( "stop" ) ) == 0 )
		{
			/* End the trace, if one is running. */
			vTraceStop();
			sprintf( pcWriteBuffer, "Stopping trace recording.\r\n" );
		}
		else
		{
			sprintf( pcWriteBuffer, "Valid parameters are 'start' and 'stop'.\r\n" );
		}

		/* There is no more data to return after this single string, so return
		pdFALSE. */
		return pdFALSE;
	}

#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

static portBASE_TYPE get_kernel_version( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	char *freertos_kernel_version_string = "\r\nFreeRTOS Kernel Version: ";
	strcpy( pcWriteBuffer, freertos_kernel_version_string);
	strcpy( pcWriteBuffer + strlen(freertos_kernel_version_string), tskKERNEL_VERSION_NUMBER );
	strcpy( pcWriteBuffer + strlen(freertos_kernel_version_string) + strlen(tskKERNEL_VERSION_NUMBER), "\r\n" );

	return pdFALSE;
}

static portBASE_TYPE get_date( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	uint8_t day;
	uint8_t month;
	uint8_t year;
	RTC_GetDate(&day, &month, &year);

	strcpy(pcWriteBuffer, "\r\n");
	convert_date_to_string(day, month, year, pcWriteBuffer + 2);
	strcpy(pcWriteBuffer + 10, "\r\n"); 	

	return pdFALSE;
}

static portBASE_TYPE get_time( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	RTC_GetTime(&hours, &minutes, &seconds);

	strcpy(pcWriteBuffer, "\r\n");
	convert_time_to_string(hours, minutes, seconds, pcWriteBuffer + 2);
	strcpy(pcWriteBuffer + 10, "\r\n");

	return pdFALSE;
}

static portBASE_TYPE set_date( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	BaseType_t param_len;

	const char *date_to_set = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);
	configASSERT( date_to_set );
	if (true != is_date_command_string_valid(date_to_set, param_len)) {
		strcpy(pcWriteBuffer, "Invalid parameter.\r\n");
	} else {
		uint8_t day;
		uint8_t month;
		uint8_t year;
		
		convert_string_to_date(&day, &month, &year, date_to_set);

		if (true == RTC_SetDate(day, month, year)) {
			char *date_set_to_string = "\r\nDate set to ";
			strcpy(pcWriteBuffer, date_set_to_string);

			uint8_t gday;
			uint8_t gmonth;
			uint8_t gyear;
			RTC_GetDate(&gday, &gmonth, &gyear);
			convert_date_to_string(gday, gmonth, gyear, pcWriteBuffer + strlen(date_set_to_string));
			strcpy(pcWriteBuffer + strlen(date_set_to_string) + 8, "\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid parameter.\r\n");
		}
	}
	
	return pdFALSE;
}

static portBASE_TYPE set_time( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	BaseType_t param_len;

	const char *time_to_set = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);
	configASSERT( time_to_set );
	if (true != is_time_command_string_valid(time_to_set, param_len)) {
		strcpy(pcWriteBuffer, "Invalid parameter.\r\n");
	} else {
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
		
		convert_string_to_time(&hours, &minutes, &seconds, time_to_set);

		if (true == RTC_SetTime(hours, minutes, seconds)) {
			char *time_set_to_string = "\r\nTime set to ";
			strcpy(pcWriteBuffer, time_set_to_string);

			uint8_t ghours;
			uint8_t gminutes;
			uint8_t gseconds;
			RTC_GetTime(&ghours, &gminutes, &gseconds);
			convert_time_to_string(ghours, gminutes, gseconds, pcWriteBuffer + strlen(time_set_to_string));
			strcpy(pcWriteBuffer + strlen(time_set_to_string) + 8, "\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid parameter.\r\n");
		}
	}

	return pdFALSE;
}

static void convert_date_to_string(uint8_t day, uint8_t month, uint8_t year, char *date_string)
{
	configASSERT( date_string );

	if (day < 10) {
		date_string[0] = '0';
		date_string[1] = (char)day + '0';
	} else if (day <= 31) {
		date_string[0] = (char)(day / 10) + '0';
		date_string[1] = (char)(day % 10) + '0';
	} else {
		date_string[0] = '?';
		date_string[1] = '?';
	}

	date_string[2] = '/';

	if (month < 10) {
		date_string[3] = '0';
		date_string[4] = (char)month + '0';
	} else if (month <= 12) {
		date_string[3] = (char)(month / 10) + '0';
		date_string[4] = (char)(month % 10) + '0';
	} else {
		date_string[3] = '?';
		date_string[4] = '?';
	}

	date_string[5] = '/';

	if (year < 10) {
		date_string[6] = '0';
		date_string[7] = (char)year + '0';
	} else {
		date_string[6] = (char)(year / 10) + '0';
		date_string[7] = (char)(year % 10) + '0';
	}
}

static void convert_time_to_string(uint8_t hours, uint8_t minutes, uint8_t seconds, char *time_string)
{
	configASSERT( time_string );

	if (hours < 10) {
		time_string[0] = '0';
		time_string[1] = (char)hours + '0';
	} else if (hours <= 23) {
		time_string[0] = (char)(hours / 10) + '0';
		time_string[1] = (char)(hours % 10) + '0';
	} else {
		time_string[0] = '?';
		time_string[1] = '?';
	}

	time_string[2] = ':';

	if (minutes < 10) {
		time_string[3] = '0';
		time_string[4] = (char)minutes + '0';
	} else if (minutes <= 59) {
		time_string[3] = (char)(minutes / 10) + '0';
		time_string[4] = (char)(minutes % 10) + '0';
	} else {
		time_string[3] = '?';
		time_string[4] = '?';
	}

	time_string[5] = ':';

	if (seconds < 10) {
		time_string[6] = '0';
		time_string[7] = (char)seconds + '0';
	} else if (seconds <= 59) {
		time_string[6] = (char)(seconds / 10) + '0';
		time_string[7] = (char)(seconds % 10) + '0';
	} else {
		time_string[6] = '?';
		time_string[7] = '?';
	}
}

static bool is_number(char s)
{
	bool retv = false;

	if (s >= '0' && s <= '9') {
		retv = true;
	}

	return retv;
}

static bool is_time_command_string_valid(char *time_string, BaseType_t len)
{
	bool retv = true;
	if (len < 8) {
		retv = false;
	}

	if (time_string[2] != ':' || time_string[5] != ':') {
		retv = false;
	}

	uint8_t non_number_chars = 0;
	for (uint8_t i = 0; i < len; i++) {
		if (true != is_number(time_string[i])) {
			non_number_chars = non_number_chars + 1;
		}
	}

	if (2 != non_number_chars) {
		retv = false;
	}

	return retv;
}

static bool is_date_command_string_valid(char *date_string, BaseType_t len)
{
	bool retv = true;
	if (len < 8) {
		retv = false;
	}

	if (date_string[2] != '/' || date_string[5] != '/') {
		retv = false;
	}

	uint8_t non_number_chars = 0;
	for (uint8_t i = 0; i < len; i++) {
		if (true != is_number(date_string[i])) {
			non_number_chars = non_number_chars + 1;
		}
	}

	if (2 != non_number_chars) {
		retv = false;
	}

	return retv;
}

static void convert_string_to_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, char *time_string)
{
	*hours   = (uint8_t)((time_string[0] - '0')*10) + (uint8_t)(time_string[1] - '0');
	*minutes = (uint8_t)((time_string[3] - '0')*10) + (uint8_t)(time_string[4] - '0');
	*seconds = (uint8_t)((time_string[6] - '0')*10) + (uint8_t)(time_string[7] - '0');
}

static void convert_string_to_date(uint8_t *day, uint8_t *month, uint8_t *year, char *date_string)
{
	*day   = (uint8_t)((date_string[0] - '0')*10) + (uint8_t)(date_string[1] - '0');
	*month = (uint8_t)((date_string[3] - '0')*10) + (uint8_t)(date_string[4] - '0');
	*year  = (uint8_t)((date_string[6] - '0')*10) + (uint8_t)(date_string[7] - '0');
}



