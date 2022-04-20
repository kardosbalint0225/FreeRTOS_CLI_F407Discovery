/*
 *
 */

#include "cli_io.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Library includes. */
#include "stm32f4xx_hal.h"


/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE					50

/* The maximum time in ticks to wait for the UART access mutex. */
#define cmdMAX_MUTEX_WAIT					( 200 / portTICK_PERIOD_MS )

/* Characters are only ever received slowly on the CLI so it is ok to pass
received characters from the UART interrupt to the task on a queue.  This sets
the length of the queue used for that purpose. */
#define cmdRXED_CHARS_QUEUE_LENGTH			( 10 )

/* DEL acts as a backspace. */
#define cmdASCII_DEL						( 0x7F )

/*
 * The task that implements the command console processing.
 */
static void cli_io_task(void *pvParameters);

/*
 * Ensure a previous interrupt driven Tx has completed before sending the next
 * data block to the UART.
 */
static void cli_io_write(const char *pcBuffer, size_t xBufferLength);
static void cli_io_read(uint8_t *buf, uint16_t size);

/*
 * Register the 'standard' sample CLI commands with FreeRTOS+CLI.
 */
extern void vRegisterSampleCLICommands( void );

/*
 * Configure the UART used for IO.
 */
static void cli_io_init(void);
static void cli_io_mspinit(UART_HandleTypeDef *huart);

/*
 * Callback functions registered with the UART driver.  Both functions
 * just 'give' a semaphore to unblock a task that may be waiting for a
 * character to be received, or a transmission to complete.
 */
static void cli_io_tx_complete_callback(UART_HandleTypeDef *huart);
static void cli_io_rx_complete_callback(UART_HandleTypeDef *huart);

static bool is_end_of_line(char ch);
static void process_command();
static void process_input(char *received_char);

/* Const messages output by the command console. */
static const char * const welcome_message      = "\r\n\r\nFreeRTOS command server.\r\nType Help to view a list of registered commands.\r\n\r\n>";
static const char * const pcEndOfOutputMessage = "\r\n[Press ENTER to execute the previous command again]\r\n>";
static const char * const new_line             = "\r\n";
static char input_string_buffer[ cmdMAX_INPUT_SIZE ];
static char last_input_string[ cmdMAX_INPUT_SIZE ];
char *output_string;
uint8_t input_index = 0;

/* This semaphore is used to allow the task to wait for a Tx to complete without wasting any CPU time. */
static SemaphoreHandle_t xTxCompleteSemaphore = NULL;

/* This semaphore is sued to allow the task to wait for an Rx to complete without wasting any CPU time. */
static SemaphoreHandle_t xRxCompleteSemaphore = NULL;

UART_HandleTypeDef h_uart_cli;

cli_callback_t commandline_interpreter;

void cli_io_task_start( uint16_t usStackSize, unsigned portBASE_TYPE uxPriority, char *cli_output_buffer, cli_callback_t cli_callback )
{
	vRegisterSampleCLICommands();

	/* Obtain the address of the output buffer.  Note there is no mutual
	exclusion on this buffer as it is assumed only one command console
	interface will be used at any one time. */
	output_string = cli_output_buffer;
	commandline_interpreter = cli_callback;

	/* Create that task that handles the console itself. */
	xTaskCreate(cli_io_task,			/* The task that implements the command console. */
				"CLI_IO",				/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
				usStackSize,			/* The size of the stack allocated to the task. */
				NULL,					/* The parameter is not used, so NULL is passed. */
				uxPriority,				/* The priority allocated to the task. */
				NULL );					/* A handle is not required, so just pass NULL. */		
}

static void cli_io_task( void *pvParameters )
{
	( void ) pvParameters;
	char cRxedChar;

	/* A UART is used for printf() output and CLI input and output.  Note there
	is no mutual exclusion on the UART, but the demo as it stands does not
	require mutual exclusion. */
	cli_io_init();

	/* Send the welcome message. */
	cli_io_write( welcome_message, strlen( welcome_message ) );

	for( ;; )
	{
		/* Wait for the next character to arrive.  A semaphore is used to
		ensure no CPU time is used until data has arrived. */
		cli_io_read( ( uint8_t * ) &cRxedChar, sizeof( cRxedChar ) );
		if( xSemaphoreTake( xRxCompleteSemaphore, portMAX_DELAY ) == pdPASS ) {
			/* Echo the character back. */
			cli_io_write( &cRxedChar, sizeof( cRxedChar ) );

			/* Was it the end of the line? */
			if (true == is_end_of_line(cRxedChar)) {
				process_command();				
			} else {
				process_input(&cRxedChar);
			}
		}
	}
}

static bool is_end_of_line(char ch)
{
	bool retv;

	if (ch == '\n' || ch == '\r') {
		retv = true;
	} else {
		retv = false;
	}

	return retv;
}

static void process_input(char *received_char)
{
	if ( *received_char == '\r' ) {
		
		/* Ignore the character. */

	} else if ( ( *received_char == '\b' ) || ( *received_char == cmdASCII_DEL ) ) {
		
		/* Backspace was pressed.  Erase the last character in the string - if any. */
		
		if ( input_index > 0 ) {
			input_index = input_index - 1;
			input_string_buffer[ input_index ] = '\0';
		}

	} else {
		
		/* A character was entered.  Add it to the string entered so far.  
		When a \n is entered the complete string will be passed to the command interpreter. */

		if ( ( *received_char >= ' ' ) && ( *received_char <= '~' ) && ( input_index < cmdMAX_INPUT_SIZE ) ) {

			input_string_buffer[ input_index ] = *received_char;
			input_index = input_index + 1;
		}
	}
}

static void process_command(void)
{
	/* Just to space the output from the input. */
	cli_io_write( new_line, strlen( new_line ) );

	/* See if the command is empty, indicating that the last command is	to be executed again. */
	if( input_index == 0 ) {					
		strcpy( input_string_buffer, last_input_string ); /* Copy the last command back into the input string. */
	}

	/* Pass the received command to the command interpreter.  The command interpreter is called repeatedly until it returns pdFALSE
	(indicating there is no more output) as it might generate more than	one string. */

	portBASE_TYPE xReturned;
	
	do {
		/* Get the next output string from the command interpreter. */
		xReturned = commandline_interpreter( input_string_buffer, output_string, configCOMMAND_INT_MAX_OUTPUT_SIZE );
	
		/* Write the generated string to the UART. */
		cli_io_write( output_string, strlen( output_string ) );
	
	} while( xReturned != pdFALSE );

	/* All the strings generated by the input command have been sent.
	Clear the input	string ready to receive the next command.  Remember
	the command that was just processed first in case it is to be
	processed again. */

	
	strcpy( last_input_string, input_string_buffer );
	input_index = 0;
	memset( input_string_buffer, 0x00, cmdMAX_INPUT_SIZE );

	cli_io_write( pcEndOfOutputMessage, strlen( pcEndOfOutputMessage ) );
}

static void cli_io_write(const char * pcBuffer, size_t xBufferLength )
{
	const TickType_t xBlockMax100ms = 100UL / portTICK_PERIOD_MS;

	if( xBufferLength > 0 )
	{
		HAL_UART_Transmit_IT( &h_uart_cli, ( uint8_t * ) pcBuffer, xBufferLength );
		
		/* Wait for the Tx to complete so the buffer can be reused without
		corrupting the data that is being sent. */
		xSemaphoreTake( xTxCompleteSemaphore, xBlockMax100ms );
	}
}

static void cli_io_read(uint8_t *buf, uint16_t size)
{
	HAL_UART_Receive_IT(&h_uart_cli, buf, size);
}

static void cli_io_init(void)
{
	/* This semaphore is used to allow the task to wait for the Tx to complete
	without wasting any CPU time. */
	vSemaphoreCreateBinary( xTxCompleteSemaphore );
	configASSERT( xTxCompleteSemaphore );

	/* This semaphore is used to allow the task to block for an Rx to complete
	without wasting any CPU time. */
	vSemaphoreCreateBinary( xRxCompleteSemaphore );
	configASSERT( xRxCompleteSemaphore );

	/* Take the semaphores so they start in the wanted state.  A block time is
	not necessary, and is therefore set to 0, as it is known that the semaphores
	exists - they have just been created. */
	xSemaphoreTake( xTxCompleteSemaphore, 0 );
	xSemaphoreTake( xRxCompleteSemaphore, 0 );

	/* Configure the hardware. */
	h_uart_cli.Instance          = USART2;
	h_uart_cli.Init.BaudRate     = 115200;
	h_uart_cli.Init.WordLength   = UART_WORDLENGTH_8B;
	h_uart_cli.Init.StopBits     = UART_STOPBITS_1;
	h_uart_cli.Init.Parity       = UART_PARITY_NONE;
	h_uart_cli.Init.Mode         = UART_MODE_TX_RX;
	h_uart_cli.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	h_uart_cli.Init.OverSampling = UART_OVERSAMPLING_16;

	HAL_UART_RegisterCallback(&h_uart_cli, HAL_UART_MSPINIT_CB_ID, cli_io_mspinit);
	while( HAL_UART_Init( &h_uart_cli ) != HAL_OK )	{
		/* Nothing to do here.  Should include a timeout really but this is
		init code only. */
	}
	
	/* Register the driver callbacks. */
	HAL_UART_RegisterCallback(&h_uart_cli, HAL_UART_TX_COMPLETE_CB_ID, cli_io_tx_complete_callback);
	HAL_UART_RegisterCallback(&h_uart_cli, HAL_UART_RX_COMPLETE_CB_ID, cli_io_rx_complete_callback);
}

static void cli_io_mspinit(UART_HandleTypeDef* huart)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	/* USART2 clock enable */
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/**USART2 GPIO Configuration
	PA2     ------> USART2_TX
	PA3     ------> USART2_RX */
	GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USART2 interrupt Init */
	HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
}

static void cli_io_rx_complete_callback(UART_HandleTypeDef * huart)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Remove compiler warnings. */
	( void ) huart;

	/* Give the semaphore  to unblock any tasks that might be waiting for an Rx
	to complete.  If a task is unblocked, and the unblocked task has a priority
	above the currently running task, then xHigherPriorityTaskWoken will be set
	to pdTRUE inside the xSemaphoreGiveFromISR() function. */
	xSemaphoreGiveFromISR( xRxCompleteSemaphore, &xHigherPriorityTaskWoken );

	/* portEND_SWITCHING_ISR() or portYIELD_FROM_ISR() can be used here. */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

static void cli_io_tx_complete_callback(UART_HandleTypeDef * huart)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Remove compiler warnings. */
	( void ) huart;

	/* Give the semaphore  to unblock any tasks that might be waiting for a Tx
	to complete.  If a task is unblocked, and the unblocked task has a priority
	above the currently running task, then xHigherPriorityTaskWoken will be set
	to pdTRUE inside the xSemaphoreGiveFromISR() function. */
	xSemaphoreGiveFromISR( xTxCompleteSemaphore, &xHigherPriorityTaskWoken );

	/* portEND_SWITCHING_ISR() or portYIELD_FROM_ISR() can be used here. */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


