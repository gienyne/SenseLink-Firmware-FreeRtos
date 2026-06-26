/*
 * alarm_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#ifndef INC_ALARM_TASK_H_
#define INC_ALARM_TASK_H_

#define LED_GREEN_PORT GPIOA
#define LED_GREEN_PIN    GPIO_PIN_9
#define LED_YELLOW_PORT  GPIOA
#define LED_YELLOW_PIN   GPIO_PIN_8
#define LED_RED_PORT     GPIOB
#define LED_RED_PIN      GPIO_PIN_5

void StartTaskAlarm(void const * argument);
#endif /* INC_ALARM_TASK_H_ */
