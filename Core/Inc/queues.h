/*
 * queues.h
 *
 *  Created on: Jun 22, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#ifndef INC_QUEUES_H_
#define INC_QUEUES_H_

#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t UartQueueHandle;
extern QueueHandle_t AlarmQueueHandle;
extern QueueHandle_t LcdQueueHandle;

#endif /* INC_QUEUES_H_ */
