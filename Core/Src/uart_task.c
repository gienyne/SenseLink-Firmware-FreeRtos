/*
 * uart_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief UART telemetry and CPU statistics task implementation.
 *
 * This task serves two purposes:
 *
 *   1. Real-time telemetry forwarding: receives pre-formatted sensor
 *      strings from the sensor task via UartQueueHandle and transmits
 *      them at the natural sensor cadence (~2 s).
 *
 *   2. Periodic CPU usage reporting: every 5 seconds, queries the
 *      FreeRTOS scheduler for runtime statistics and transmits a
 *      human-readable table over UART, parseable by the Python bridge
 *      for display on the React dashboard.
 *
 * UART format for telemetry lines (parsed by bridge.py):
 *   "T:27.9 H:57.9 P:1001.3 A:1\r\n"
 *
 * UART format for CPU report (parsed by bridge.py):
 *   "\r\n--- CPU USE ---\r\n"
 *   "TaskSensor   : <1%\r\n"
 *   "TaskUART     : <1%\r\n"
 *   ...
 *   "IDLE         : 98%\r\n"
 *   "---------------\r\n\r\n"
 */

#include "uart_task.h"
#include "sensor_data.h"
#include "stm32f0xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queues.h"
#include <string.h>
#include <stdio.h>

/* UART handle provided by main.c. */
extern UART_HandleTypeDef huart2;


void StartTaskUART(void const * argument)
{
    FormattedData_t receivedData;
    TickType_t      lastStatsTime = xTaskGetTickCount();

    /* Static allocation keeps these large objects off the task stack.
     * The array is sized to accommodate all active tasks plus the FreeRTOS
     * Idle task. uxTaskGetNumberOfTasks() is called at runtime to determine
     * the exact count and prevent buffer overrun. */
    static TaskStatus_t pxTaskStatusArray[10];
    static char         msg[64];
    uint32_t            TotalRunTime;

    /* Banner transmitted once on startup to confirm UART connectivity. */
    const char *init_msg = "SenseLink v1 - UART ready\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)init_msg, strlen(init_msg), 100);

    for(;;)
    {
        /* --- Telemetry forwarding --- */

        /* Block for up to 500 ms for a new sensor payload.
         * The short timeout allows the CPU statistics block below to fire
         * at roughly the correct 5-second interval even if no sensor data
         * is currently flowing. */
        if(xQueueReceive(UartQueueHandle, &receivedData, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            HAL_UART_Transmit(&huart2, (uint8_t *)receivedData.uart_buffer, strlen(receivedData.uart_buffer), 100);
        }

        /* --- Periodic CPU usage report (every 5 seconds) --- */

        if ((xTaskGetTickCount() - lastStatsTime) >= pdMS_TO_TICKS(5000))
        {
            lastStatsTime = xTaskGetTickCount();

            /* Query the actual task count at runtime to correctly size the
             * uxTaskGetSystemState() call and prevent writing beyond the
             * bounds of pxTaskStatusArray. */
            UBaseType_t uxNbTasks = uxTaskGetNumberOfTasks();
            if (uxNbTasks > 10) uxNbTasks = 10;

            UBaseType_t uxArraySize = uxTaskGetSystemState(
                pxTaskStatusArray, uxNbTasks, &TotalRunTime);

            if (uxArraySize > 0 && TotalRunTime > 0)
            {
                HAL_UART_Transmit(&huart2,
                                  (uint8_t *)"\r\n--- CPU USE ---\r\n", 18, 100);

                for (UBaseType_t i = 0; i < uxArraySize; i++)
                {
                    uint32_t percent =
                        (pxTaskStatusArray[i].ulRunTimeCounter * 100) / TotalRunTime;

                    if (percent > 100) percent = 100;

                    /* Distinguish tasks that have run but consumed less than 1%
                     * of CPU time from tasks that have genuinely never been
                     * scheduled (ulRunTimeCounter == 0). */
                    if (percent == 0 && pxTaskStatusArray[i].ulRunTimeCounter > 0)
                    {
                        snprintf(msg, sizeof(msg), "%-12s : <1%%\r\n",
                                 pxTaskStatusArray[i].pcTaskName);
                    }
                    else
                    {
                        snprintf(msg, sizeof(msg), "%-12s : %u%%\r\n", pxTaskStatusArray[i].pcTaskName, (unsigned int)percent);
                    }

                    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
                }

                    /* End-of-block delimiter parsed by bridge.py to detect the
                    * boundary between CPU report and telemetry lines. */
                    HAL_UART_Transmit(&huart2, (uint8_t *)"---------------\r\n\r\n", 19, 100);
            }
        }
    }
}
