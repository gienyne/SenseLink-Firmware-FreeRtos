/*
 * alarm_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#include "alarm_task.h"
#include "sensor_data.h"
#include "stm32f0xx_hal.h"
#include "FreeRTOS.h"
#include "queues.h"

void StartTaskAlarm(void const * argument){

	SensorData_t receivedData;

	for(;;){

		if(xQueueReceive(AlarmQueueHandle, &receivedData, pdMS_TO_TICKS(2000)) == pdTRUE){

			if(receivedData.temperature > TEMP_THRESHOLD_MAX || receivedData.humidity > HUMIDITY_THRESHOLD_MAX){
				HAL_GPIO_WritePin(ALARM_LED_PORT, ALARM_LED_PIN, GPIO_PIN_SET);
			}

			else {
				HAL_GPIO_WritePin(ALARM_LED_PORT, ALARM_LED_PIN, GPIO_PIN_RESET);
			}
		}
		else {
			/* Timeout: Wenn keine Daten empfangen -> LED ausschalten */
			HAL_GPIO_WritePin(ALARM_LED_PORT, ALARM_LED_PIN, GPIO_PIN_RESET);
		}

	}
}

