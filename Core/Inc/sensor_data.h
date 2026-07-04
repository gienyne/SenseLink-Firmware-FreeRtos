/*
 * sensor_data.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Shared data structures, alarm thresholds, and global state
 *        declarations used across all FreeRTOS tasks.
 *
 * This header is the single source of truth for inter-task data contracts.
 * It defines two queue payload types:
 *   - SensorData_t    : raw floating-point measurements for the alarm task.
 *   - FormattedData_t : pre-formatted strings for the LCD and UART tasks,
 *                       produced once by the sensor task to avoid redundant
 *                       snprintf calls in consumer tasks.
 */

#ifndef INC_SENSOR_DATA_H_
#define INC_SENSOR_DATA_H_

#include <stdint.h>

/* Alarm thresholds.
 * When temperature exceeds TEMP_THRESHOLD_MAX OR humidity exceeds
 * HUMIDITY_THRESHOLD_MAX, a Warning is raised. When both are exceeded
 * simultaneously, the alarm escalates to Critical. */
#define TEMP_THRESHOLD_MAX     30.0f   /* degrees Celsius */
#define HUMIDITY_THRESHOLD_MAX 55.0f   /* percent relative humidity */

/*
 * @brief Raw sensor readings as floating-point values.
 *
 * Sent on AlarmQueueHandle so that the alarm task can compare values
 * directly against the numeric thresholds without string parsing.
 */
typedef struct {
    float temperature;   /* degrees Celsius  */
    float humidity;      /* percent RH       */
    float pressure;      /* hPa              */
} SensorData_t;

/*
 * @brief Pre-formatted display and telemetry strings.
 *
 * Built once per cycle in StartTaskSensor using snprintf, then dispatched
 * to UartQueueHandle and LcdQueueHandle. This centralises all formatting
 * and reduces stack pressure in the LCD and UART consumer tasks.
 *
 * lcd_line1[17] : First LCD line  e.g. "T:28.6C  H:45.3%"
 * lcd_line2[17] : Second LCD line e.g. "P:1001.2 hPa"
 * uart_buffer[64]: Full telemetry string e.g. "T:28.6 H:45.3 P:1001.2 A:1\r\n"
 */
typedef struct {
    char lcd_line1[17];
    char lcd_line2[17];
    char uart_buffer[64];
} FormattedData_t;

/*
 * Global alarm state variables defined in alarm_task.c.
 *
 * Declared volatile because they are written from ISR context
 * (HAL_UART_RxCpltCallback) and read from multiple task contexts.
 *
 * current_alarm_state : Active alarm severity (1=Nominal, 2=Warning, 3=Critical).
 * reset_request       : Set to 1 by the UART ISR on reception of 'R';
 *                       consumed by StartTaskAlarm to clear the alarm latch.
 */
extern volatile uint8_t current_alarm_state;
extern volatile uint8_t reset_request;

#endif /* INC_SENSOR_DATA_H_ */
