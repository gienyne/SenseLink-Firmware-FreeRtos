/*
 * queues.c
 *
 *  Created on: Jun 22, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Definition of all FreeRTOS inter-task queue handles.
 *
 * Handles are initialised to NULL here and created (sized and allocated
 * from the FreeRTOS heap) in main.c before the scheduler is started.
 * All tasks access these queues via the extern declarations in queues.h.
 */

#include "queues.h"

/* Queue handle for sensor-to-UART formatted telemetry payloads. */
QueueHandle_t UartQueueHandle  = NULL;

/* Queue handle for sensor-to-alarm raw measurement payloads. */
QueueHandle_t AlarmQueueHandle = NULL;

/* Queue handle for sensor-to-LCD formatted display payloads. */
QueueHandle_t LcdQueueHandle   = NULL;
