/*
 * uart_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Header for the UART telemetry and CPU statistics task.
 *
 * Declares the FreeRTOS task entry point responsible for forwarding
 * sensor telemetry over UART and periodically reporting FreeRTOS
 * runtime CPU usage statistics.
 */

#ifndef INC_UART_TASK_H_
#define INC_UART_TASK_H_

#include "cmsis_os.h"

/*
 * @brief FreeRTOS task entry point for UART telemetry output.
 *
 * On startup, transmits a banner message to confirm the UART link is active.
 *
 * Each cycle (500 ms queue timeout):
 *   - Forwards the pre-formatted UART telemetry string received from the
 *     sensor task directly to the UART peripheral.
 *
 * Every 5 seconds additionally:
 *   - Queries FreeRTOS runtime statistics via uxTaskGetSystemState().
 *   - Transmits a formatted CPU usage report for all active tasks.
 *     Tasks consuming less than 1% of CPU time are reported as "<1%"
 *     to distinguish them from tasks that have genuinely never run (0%).
 *
 * The TaskStatus_t array and formatting buffer are declared static to
 * avoid placing them on the task stack and exhausting the 192-word limit.
 *
 * @param argument Unused (required by CMSIS-RTOS API)
 */
void StartTaskUART(void const * argument);

#endif /* INC_UART_TASK_H_ */
