/*
 * uart_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#include "uart_task.h"
#include "sensor_data.h"
#include "stm32f0xx_hal.h"
#include "FreeRTOS.h"
#include "queues.h"
#include <string.h>


extern UART_HandleTypeDef huart2;


void StartTaskUART(void const * argument){

	FormattedData_t receivedData;

	char *init_msg = "SenseLink v1 - UART ready\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t*)init_msg, strlen(init_msg), 100);

	for(;;){

		if(xQueueReceive(UartQueueHandle, &receivedData, pdMS_TO_TICKS(2000)) == pdTRUE)
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)receivedData.uart_buffer, strlen(receivedData.uart_buffer), 100);
		}
	}
}
