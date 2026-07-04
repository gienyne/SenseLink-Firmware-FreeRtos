/*
 * bme280_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Header for the BME280 sensor acquisition task.
 *
 * Declares the FreeRTOS task entry point responsible for initialising
 * the BME280 environmental sensor and periodically reading temperature,
 * humidity and pressure measurements.
 */

#ifndef INC_BME280_TASK_H_
#define INC_BME280_TASK_H_

#include "cmsis_os.h"

/*
 * @brief FreeRTOS task entry point for BME280 sensor acquisition.
 *
 * On startup, waits 3 seconds to allow the LCD task to complete its
 * initialisation sequence before claiming the shared I2C bus.
 * Then initialises the BME280 in forced mode with the configured
 * oversampling and filter settings.
 *
 * Each cycle (every 2 seconds):
 *   1. Acquires the I2C mutex and reads one measurement from the BME280.
 *   2. Formats the raw values into display and telemetry strings using
 *      snprintf (centralised here to avoid stack pressure in consumers).
 *   3. Dispatches payloads to UartQueueHandle, AlarmQueueHandle and
 *      LcdQueueHandle.
 *
 * If sensor initialisation fails, fallback values (99.9 / 999.9) are
 * sent so that downstream tasks continue to run and the failure is
 * visible on the LCD and in the UART stream.
 *
 * @param argument Unused (required by CMSIS-RTOS API)
 */
void StartTaskSensor(void const * argument);

#endif /* INC_BME280_TASK_H_ */
