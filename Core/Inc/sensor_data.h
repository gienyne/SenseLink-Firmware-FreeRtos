/*
 * sensor_data.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#ifndef INC_SENSOR_DATA_H_
#define INC_SENSOR_DATA_H_

#define TEMP_THRESHOLD_MAX 30.0f // °C
#define HUMIDITY_THRESHOLD_MAX 70.0f // %


typedef struct{
	float temperature;
	float humidity;
	float pressure;
} SensorData_t;

typedef struct {
    char lcd_line1[17];
    char lcd_line2[17];
    char uart_buffer[64];
} FormattedData_t;

#endif /* INC_SENSOR_DATA_H_ */
