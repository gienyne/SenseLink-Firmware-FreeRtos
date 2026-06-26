/*
 * bme280_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#include "bme280_task.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "sensor_data.h"
#include "stm32f0xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queues.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;
extern osMutexId i2cMutexHandle;

static const uint8_t bme280_addr = ( BME280_I2C_ADDR_PRIM << 1);
static struct bme280_dev bme;
static struct bme280_settings settings;

static int8_t user_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void *intf_ptr){
	return HAL_I2C_Mem_Read((I2C_HandleTypeDef*) intf_ptr , bme280_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, reg_data, len, 100);
}

static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr){
	return HAL_I2C_Mem_Write((I2C_HandleTypeDef*) intf_ptr, bme280_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)reg_data, len, 100);
}

static void user_delay_us(uint32_t period, void* intf_ptr){
	osDelay(period / 1000 + 1);
}

static int8_t bme280_sensor_init(void){

	int rslt;

	bme.intf = BME280_I2C_INTF;
	bme.intf_ptr = &hi2c1;
	bme.read = user_i2c_read;
	bme.write = user_i2c_write;
	bme.delay_us = user_delay_us;

	rslt = bme280_init(&bme);

	if (rslt != BME280_OK) {
	        return rslt;
	    }

	settings.osr_t = BME280_OVERSAMPLING_2X;
	settings.osr_p = BME280_OVERSAMPLING_4X;
	settings.osr_h = BME280_OVERSAMPLING_1X;
	settings.filter = BME280_FILTER_COEFF_2;

	uint8_t settings_sel = BME280_SEL_OSR_PRESS |
	                            BME280_SEL_OSR_TEMP  |
	                            BME280_SEL_OSR_HUM   |
	                            BME280_SEL_FILTER;

	rslt = bme280_set_sensor_settings(settings_sel, &settings, &bme);

	return rslt;
}

static int8_t bme280_sensor_read(SensorData_t* data){

	int8_t rslt;
	struct bme280_data sensor_data;

	rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &bme);

	if(rslt != BME280_OK) {
		return rslt;
	}

	uint32_t delay_max_us = 40000;

	bme280_cal_meas_delay(&delay_max_us, &settings);
	bme.delay_us(delay_max_us, NULL);

	rslt = bme280_get_sensor_data(BME280_ALL, &sensor_data, &bme);

	if(rslt == BME280_OK){
		data->temperature = sensor_data.temperature;
		data->pressure = sensor_data.pressure / 100.0f;
		data->humidity = sensor_data.humidity;
	}

	return rslt;
}

void StartTaskSensor(void const * argument){

    SensorData_t sensorData = {0};
    FormattedData_t formattedData = {0};

    osDelay(3000); // lass genug Zeit für die Initialisierung des Lcd

    int8_t init_rslt = bme280_sensor_init();

    for(;;) {
        if (init_rslt == BME280_OK) {
            osMutexWait(i2cMutexHandle, osWaitForever);
            bme280_sensor_read(&sensorData);
            osMutexRelease(i2cMutexHandle);
        } else {
            sensorData.temperature = 99.9f;
            sensorData.humidity = 99.9f;
            sensorData.pressure = 999.9f;
        }

        // Die Zeichenfolgen HIER eingeben (nur einmal für alle!
        snprintf(formattedData.lcd_line1, sizeof(formattedData.lcd_line1), "T:%.1fC  H:%.1f%%", sensorData.temperature, sensorData.humidity);
        snprintf(formattedData.lcd_line2, sizeof(formattedData.lcd_line2), "P:%.1f hPa", sensorData.pressure);
        snprintf(formattedData.uart_buffer, sizeof(formattedData.uart_buffer), "T:%.1f H:%.1f P:%.1f\r\n", sensorData.temperature, sensorData.humidity, sensorData.pressure);

        xQueueSend(UartQueueHandle,  &formattedData, 0); // Text senden
        xQueueSend(AlarmQueueHandle, &sensorData, 0);    // Der Alarm behält die Roh-Floats bei!
        xQueueSend(LcdQueueHandle,   &formattedData, 0); // Text senden

        osDelay(2000);
    }
}
