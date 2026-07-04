/*
 * queues.h
 *
 *  Created on: Jun 22, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Central declaration point for all FreeRTOS inter-task queues.
 *
 * All queues are defined in queues.c and declared extern here so that
 * every translation unit that includes this header can access them
 * without redefinition conflicts.
 *
 * Queue overview:
 *   UartQueueHandle  : FormattedData_t  - sensor task -> UART task
 *   AlarmQueueHandle : SensorData_t     - sensor task -> alarm task
 *   LcdQueueHandle   : FormattedData_t  - sensor task -> LCD task
 */

#ifndef INC_QUEUES_H_
#define INC_QUEUES_H_

#include "FreeRTOS.h"
#include "queue.h"

/* Handle for the UART telemetry queue.
 * Carries pre-formatted FormattedData_t payloads from the sensor task
 * to the UART task for serial transmission. Depth: 2. */
extern QueueHandle_t UartQueueHandle;

/* Handle for the alarm evaluation queue.
 * Carries raw SensorData_t payloads from the sensor task to the alarm
 * task for threshold comparison. Depth: 1. */
extern QueueHandle_t AlarmQueueHandle;

/* Handle for the LCD display queue.
 * Carries pre-formatted FormattedData_t payloads from the sensor task
 * to the LCD task for display. Depth: 3 to absorb timing jitter between
 * the sensor acquisition cycle and the I2C display refresh cycle. */
extern QueueHandle_t LcdQueueHandle;

#endif /* INC_QUEUES_H_ */
