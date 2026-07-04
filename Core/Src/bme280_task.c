/*
 * bme280_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief BME280 sensor acquisition task implementation.
 *
 * This module acts as the data producer for the entire SenseLink pipeline.
 * It owns the BME280 driver configuration, performs all I2C communication
 * under mutex protection (shared bus with the LCD), and centralises all
 * string formatting so that consumer tasks (LCD, UART) require minimal
 * stack space.
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

/* External handles provided by main.c. */
extern I2C_HandleTypeDef hi2c1;
extern osMutexId i2cMutexHandle;

/* BME280 I2C address in 8-bit format (7-bit address shifted left by 1).
 * SDO pin is left unconnected, selecting the primary address 0x76. But he can be also connected to the GND */
static const uint8_t bme280_addr = (BME280_I2C_ADDR_PRIM << 1);

/* BME280 driver instance and settings structures managed by the Bosch API. */
static struct bme280_dev      bme;
static struct bme280_settings settings;


/* --- BME280 HAL I2C bridge functions required by the Bosch driver --- */

/*
 * @brief I2C read callback for the Bosch BME280 driver.
 *
 * Wraps HAL_I2C_Mem_Read so the Bosch API can perform register reads
 * without direct knowledge of the STM32 HAL layer.
 *
 * @param reg_addr  Register address to read from.
 * @param reg_data  Buffer to store the received bytes.
 * @param len       Number of bytes to read.
 * @param intf_ptr  Pointer to the I2C handle (cast from void*).
 * @return 0 on success, non-zero on HAL error.
 */
static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)intf_ptr, bme280_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, reg_data, len, 100);
}

/*
 * @brief I2C write callback for the Bosch BME280 driver.
 *
 * Wraps HAL_I2C_Mem_Write so the Bosch API can perform register writes.
 *
 * @param reg_addr  Register address to write to.
 * @param reg_data  Buffer containing bytes to write.
 * @param len       Number of bytes to write.
 * @param intf_ptr  Pointer to the I2C handle (cast from void*).
 * @return 0 on success, non-zero on HAL error.
 */
static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)intf_ptr, bme280_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t *)reg_data, len, 100);
}

/*
 * @brief Microsecond delay callback for the Bosch BME280 driver.
 *
 * Uses osDelay (FreeRTOS tick-based) to yield the CPU while waiting,
 * rather than blocking with HAL_Delay. Converts microseconds to
 * milliseconds with a +1 safety margin to avoid under-waiting.
 *
 * @param period    Delay duration in microseconds.
 * @param intf_ptr  Unused.
 */
static void user_delay_us(uint32_t period, void *intf_ptr)
{
    osDelay(period / 1000 + 1);
}


/*
 * @brief Initialises the BME280 sensor with the desired sampling configuration.
 *
 * Configures the Bosch driver instance with the I2C bridge functions,
 * calls bme280_init() to verify the chip ID, then applies oversampling
 * and filter settings for balanced accuracy and response time.
 *
 * Oversampling:
 *   Temperature : x2
 *   Pressure    : x4
 *   Humidity    : x1
 * IIR filter coefficient: 2
 *
 * @return BME280_OK (0) on success, negative error code on failure.
 */
static int8_t bme280_sensor_init(void)
{
    int rslt;

    bme.intf     = BME280_I2C_INTF;
    bme.intf_ptr = &hi2c1;
    bme.read     = user_i2c_read;
    bme.write    = user_i2c_write;
    bme.delay_us = user_delay_us;

    rslt = bme280_init(&bme);
    if (rslt != BME280_OK)
    {
        return rslt;
    }

    settings.osr_t  = BME280_OVERSAMPLING_2X;
    settings.osr_p  = BME280_OVERSAMPLING_4X;
    settings.osr_h  = BME280_OVERSAMPLING_1X;
    settings.filter = BME280_FILTER_COEFF_2;

    uint8_t settings_sel = BME280_SEL_OSR_PRESS |
                           BME280_SEL_OSR_TEMP  |
                           BME280_SEL_OSR_HUM   |
                           BME280_SEL_FILTER;

    rslt = bme280_set_sensor_settings(settings_sel, &settings, &bme);
    return rslt;
}

/*
 * @brief Triggers a single forced-mode measurement and reads the result.
 *
 * Sets the sensor to forced mode (single-shot), waits for the measurement
 * to complete using the driver-calculated delay, then reads all three
 * channels. Pressure is converted from Pa to hPa before storage.
 *
 * @param data  Pointer to a SensorData_t struct to populate.
 * @return BME280_OK on success, negative error code on failure.
 */
static int8_t bme280_sensor_read(SensorData_t *data)
{
    int8_t rslt;
    struct bme280_data sensor_data;

    rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &bme);
    if (rslt != BME280_OK)
    {
        return rslt;
    }

    /* Wait for the measurement to complete.
     * The Bosch API calculates the exact delay based on the configured
     * oversampling rates. */
    uint32_t delay_max_us = 40000;
    bme280_cal_meas_delay(&delay_max_us, &settings);
    bme.delay_us(delay_max_us, NULL);

    rslt = bme280_get_sensor_data(BME280_ALL, &sensor_data, &bme);
    if (rslt == BME280_OK)
    {
        data->temperature = sensor_data.temperature;
        data->pressure    = sensor_data.pressure / 100.0f; /* Pa to hPa */
        data->humidity    = sensor_data.humidity;
    }

    return rslt;
}


void StartTaskSensor(void const * argument)
{
    SensorData_t    sensorData    = {0};
    FormattedData_t formattedData = {0};

    /* Wait for the LCD task to complete its initialisation sequence
     * before attempting any I2C communication on the shared bus. */
    osDelay(3000);

    int8_t init_rslt = bme280_sensor_init();

    for(;;)
    {
        if (init_rslt == BME280_OK)
        {
            /* Acquire the I2C bus mutex before reading.
             * The LCD task also holds this mutex during display writes,
             * preventing bus contention on the shared I2C1 peripheral. */
            osMutexWait(i2cMutexHandle, osWaitForever);
            bme280_sensor_read(&sensorData);
            osMutexRelease(i2cMutexHandle);
        }
        else
        {
            /* Sensor unavailable: populate with out-of-range sentinel values
             * so that the failure is immediately visible on all outputs. */
            sensorData.temperature = 99.9f;
            sensorData.humidity    = 99.9f;
            sensorData.pressure    = 999.9f;
        }

        /* Format all output strings once here, centralising snprintf usage.
         * This keeps the LCD and UART consumer tasks lightweight and avoids
         * duplicating the floating-point formatting library on their stacks. */
        snprintf(formattedData.lcd_line1, sizeof(formattedData.lcd_line1),
                 "T:%.1fC  H:%.1f%%",
                 sensorData.temperature, sensorData.humidity);

        snprintf(formattedData.lcd_line2, sizeof(formattedData.lcd_line2),
                 "P:%.1f hPa",
                 sensorData.pressure);

        snprintf(formattedData.uart_buffer, sizeof(formattedData.uart_buffer),
                 "T:%.1f H:%.1f P:%.1f A:%d\r\n",
                 sensorData.temperature, sensorData.humidity,
                 sensorData.pressure, current_alarm_state);

        /* Dispatch payloads to consumer queues.
         * A timeout of 0 is used so the sensor task never blocks on a full
         * queue; if a consumer is too slow, the oldest sample is simply
         * discarded and the latest reading takes its place next cycle. */
        xQueueSend(UartQueueHandle,  &formattedData, 0);
        xQueueSend(AlarmQueueHandle, &sensorData,    0);
        xQueueSend(LcdQueueHandle,   &formattedData, 0);

        osDelay(2000);
    }
}
