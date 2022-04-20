/*
 * cli.c
 *
 *  Created on: 2022. apr. 17.
 *      Author: Balint
 */
#include "cli.h"
#include "cli_io.h"
#include "FreeRTOS_CLI.h"


void cli_init(void)
{
    char *cli_output = FreeRTOS_CLIGetOutputBuffer();
    cli_io_task_start( ( configMINIMAL_STACK_SIZE * 3 ), tskIDLE_PRIORITY, cli_output, FreeRTOS_CLIProcessCommand );
}

