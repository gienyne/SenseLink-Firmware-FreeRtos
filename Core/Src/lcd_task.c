/*
 * lcd_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#include "lcd_task.h"
#include "lcd_i2c.h"
#include "sensor_data.h"
#include "queues.h"
#include "stm32f0xx_hal.h"
#include "cmsis_os.h"

extern I2C_HandleTypeDef hi2c1;

void StartTaskLCD(void const * argument)
{
    FormattedData_t receivedData; // verwendet den Typ "Text"

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
        if(xQueueReceive(LcdQueueHandle, &receivedData, pdMS_TO_TICKS(2000)) == pdTRUE)
        {

            LCD_SetCursor(0, 0);
            LCD_Print(receivedData.lcd_line1);
            LCD_SetCursor(0, 1);
            LCD_Print(receivedData.lcd_line2);
        }
        else
        {
            LCD_SetCursor(0, 0);
            LCD_Print("Waiting data... ");
            LCD_SetCursor(0, 1);
            LCD_Print("                ");
        }
    }
}
