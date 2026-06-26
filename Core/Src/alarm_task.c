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
	uint8_t red_led_state = 0;

	for(;;){

		if(xQueueReceive(AlarmQueueHandle, &receivedData, pdMS_TO_TICKS(2000)) == pdTRUE){

			if(receivedData.temperature > TEMP_THRESHOLD_MAX && receivedData.humidity > HUMIDITY_THRESHOLD_MAX){

				HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);

				red_led_state = !red_led_state;
			    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, red_led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}

			else if(receivedData.temperature > TEMP_THRESHOLD_MAX || receivedData.humidity > HUMIDITY_THRESHOLD_MAX){

				HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

				HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_SET);
				red_led_state = 0;
			}

			else{

				HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

				HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
				red_led_state = 0;
			}

		}
		else {
			/* Timeout: Wenn keine Daten empfangen -> LED ausschalten */
			HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
			red_led_state = 0;
		}

	}
}

