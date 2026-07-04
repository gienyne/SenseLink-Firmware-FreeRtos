/*
 * lcd_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Header for the LCD display task.
 *
 * Declares the FreeRTOS task entry point responsible for receiving
 * pre-formatted strings from the sensor task and rendering them on
 * the 16x2 HD44780 LCD display.
 */

#ifndef INC_LCD_TASK_H_
#define INC_LCD_TASK_H_

#include "cmsis_os.h"

/*
 * @brief FreeRTOS task entry point for the LCD display task.
 *
 * Initialises the LCD on startup and then blocks on LcdQueueHandle
 * waiting for FormattedData_t payloads produced by the sensor task.
 * On each received payload, the two pre-formatted lines are written
 * directly to the display without additional string processing.
 *
 * If no data is received within the 2-second timeout, a "Waiting data"
 * message is shown to indicate that the sensor pipeline has stalled.
 *
 * @param argument Unused (required by CMSIS-RTOS API)
 */
void StartTaskLCD(void const * argument);

#endif /* INC_LCD_TASK_H_ */
