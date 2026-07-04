/*
 * lcd_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief LCD display task implementation.
 *
 * This task is a lightweight consumer: it performs no formatting and
 * contains no floating-point operations. All string preparation is
 * delegated to the sensor task, keeping this task's stack requirement
 * at 128 words.
 *
 * The LCD queue depth is set to 3 in main.c to absorb timing jitter
 * between the 2-second sensor cycle and the I2C display refresh, which
 * can be temporarily delayed by mutex contention with the sensor task.
 */

#include "lcd_task.h"
#include "lcd_i2c.h"
#include "sensor_data.h"
#include "queues.h"
#include "stm32f0xx_hal.h"
#include "cmsis_os.h"
#include <string.h>

/* I2C handle provided by main.c, passed to the LCD driver on init. */
extern I2C_HandleTypeDef hi2c1;


void StartTaskLCD(void const * argument)
{
    /* Receive buffer for pre-formatted display strings from the sensor task. */
    FormattedData_t receivedData;

    /* Initialise the LCD hardware and display a startup splash screen.
     * The 2-second delay also gives the BME280 sensor task time to
     * complete its own initialisation before the first data packet arrives. */
    LCD_Init(&hi2c1);
    LCD_Backlight(1);
    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_Print("  SenseLink v1  ");
    LCD_SetCursor(0, 1);
    LCD_Print("  Initializing  ");
    osDelay(2000);
    LCD_Clear();

    for(;;)
    {
        /* Block until a formatted payload is available, or until the
         * 2-second timeout elapses. The timeout ensures the display
         * always shows something meaningful even if the sensor pipeline
         * is temporarily stalled. */
        if(xQueueReceive(LcdQueueHandle, &receivedData, pdMS_TO_TICKS(2000)) == pdTRUE)
        {
            // Write line 1: temperature and humidity (e.g. "T:25.9C  H:53.9%")
            LCD_SetCursor(0, 0);
            LCD_Print(receivedData.lcd_line1);

            // Write line 2: atmospheric pressure (e.g. "P:1001.1 hPa")
            LCD_SetCursor(0, 1);
            LCD_Print(receivedData.lcd_line2);
        }
        else
        {
            /* Timeout: no data received. Display a waiting message so the
             * user can distinguish a healthy idle state from a sensor fault. */
            LCD_SetCursor(0, 0);
            LCD_Print("Waiting data... ");
            LCD_SetCursor(0, 1);
            LCD_Print("                ");
        }
    }
}
