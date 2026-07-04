/*
 * alarm_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Header for the alarm management task.
 *        Defines the GPIO pin mapping for the three-color LED alarm indicator
 *        and declares the FreeRTOS task entry point.
 */

#ifndef INC_ALARM_TASK_H_
#define INC_ALARM_TASK_H_

/* GPIO pin mapping for the tri-color LED alarm system.
 * Green LED : PA9  - Nominal state (all thresholds within range)
 * Yellow LED: PA8  - Warning state (one threshold exceeded)
 * Red LED   : PB5  - Critical state (both thresholds exceeded, blinks)
 */
#define LED_GREEN_PORT   GPIOA
#define LED_GREEN_PIN    GPIO_PIN_9
#define LED_YELLOW_PORT  GPIOA
#define LED_YELLOW_PIN   GPIO_PIN_8
#define LED_RED_PORT     GPIOB
#define LED_RED_PIN      GPIO_PIN_5

/*
 * @brief FreeRTOS task entry point for alarm state management.
 *
 * Receives raw sensor data from AlarmQueueHandle, evaluates it against
 * the defined thresholds, updates the global alarm state, and drives
 * the physical LEDs accordingly.
 *
 * Alarm states:
 *   1 - Nominal  : Green LED on
 *   2 - Warning  : Yellow LED on (one threshold exceeded)
 *   3 - Critical : Red LED blinking (both thresholds exceeded)
 *
 * The alarm state is latched: once triggered, it can only be cleared
 * by a hardware reset command received via UART.
 *
 * @param argument Unused (required by CMSIS-RTOS API)
 */
void StartTaskAlarm(void const * argument);

#endif /* INC_ALARM_TASK_H_ */
