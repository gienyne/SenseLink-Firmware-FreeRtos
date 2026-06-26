/*
 * alarm_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#ifndef INC_ALARM_TASK_H_
#define INC_ALARM_TASK_H_

#define ALARM_LED_PORT   GPIOA
#define ALARM_LED_PIN    GPIO_PIN_5

void StartTaskAlarm(void const * argument);
#endif /* INC_ALARM_TASK_H_ */
