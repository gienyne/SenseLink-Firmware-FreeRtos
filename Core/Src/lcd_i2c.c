/*
 * lcd_i2c.c
 *
 *  Created on: Jun 23, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#include "lcd_i2c.h"
#include "cmsis_os.h"

static I2C_HandleTypeDef* lcd_i2c_handle = NULL;
static uint8_t lcd_backlight_state = LCD_BACKLIGHT;
extern osMutexId i2cMutexHandle;

static void LCD_WritePointer(uint8_t data);
static void LCD_EnablePulse(uint8_t data);
static void LCD_Send(uint8_t val, uint8_t mode);
static void LCD_SendCommand(uint8_t cmd);
static void LCD_SendData(uint8_t data);


void LCD_Init(I2C_HandleTypeDef *hi2c){

	lcd_i2c_handle = hi2c;

	LCD_DELAY(50);

	LCD_WritePointer(0x30); LCD_EnablePulse(0x30);
	LCD_DELAY(5);
	LCD_WritePointer(0x30); LCD_EnablePulse(0x30);
	LCD_DELAY(1);
	LCD_WritePointer(0x30); LCD_EnablePulse(0x30);
	LCD_DELAY(10);

    // Offizieller Übergang zum 4-Bit-Modus
	LCD_WritePointer(0x20); LCD_EnablePulse(0x20);
	LCD_DELAY(10);
	LCD_SendCommand(0x28); // 4-bits, 2 Zeilen, 5x8 Zeichen
	LCD_DELAY(1);
	LCD_SendCommand(0x0C); // Display ON, Cursor OFF
	LCD_DELAY(1);
	LCD_SendCommand(0x06); // Cursor auto-increment
	LCD_DELAY(1);

	LCD_Clear();
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    LCD_DELAY(2);
}

void LCD_SetCursor(uint8_t col, uint8_t row)
{
	uint8_t row_offsets[] = {0x00, 0x40};

	if(row >= LCD_ROWS){
	   row = LCD_ROWS - 1;
	}
	if (col >= LCD_COLS){
	   col = LCD_COLS - 1;
	}

	LCD_SendCommand(0x80 | (col + row_offsets[row]));

}

void LCD_Print(const char* str){

	while(*str)
	{
		LCD_SendData((uint8_t)(*str));
		str++;
	}
}

void LCD_Backlight(uint8_t on_off)
{
    lcd_backlight_state = on_off ? LCD_BACKLIGHT : 0x00;
    LCD_WritePointer(0x00);
}

static void LCD_WritePointer(uint8_t data)
{
    if (lcd_i2c_handle != NULL)
    {
        uint8_t tx_data = data | lcd_backlight_state;
        HAL_I2C_Master_Transmit(lcd_i2c_handle, LCD_ADDR, &tx_data, 1, 100);

    }
}

static void LCD_EnablePulse(uint8_t data)
{
    LCD_WritePointer(data | LCD_ENABLE);
    LCD_DELAY(1);
    LCD_WritePointer(data & ~LCD_ENABLE);
    LCD_DELAY(1);
}

static void LCD_Send(uint8_t data, uint8_t mode){

	uint8_t high_nibble = (data & 0xF0) | mode;
	LCD_WritePointer(high_nibble);
	LCD_EnablePulse(high_nibble);

	uint8_t low_nibble = ((data << 4) & 0xF0) | mode;
	LCD_WritePointer(low_nibble);
    LCD_EnablePulse(low_nibble);

}

static void LCD_SendCommand(uint8_t cmd)
{
    //LCD_Send(cmd, 0);
	 osMutexWait(i2cMutexHandle, osWaitForever);
	    LCD_Send(cmd, 0);
	    osMutexRelease(i2cMutexHandle);
}

static void LCD_SendData(uint8_t data)
{
    //LCD_Send(data, LCD_RS);
     osMutexWait(i2cMutexHandle, osWaitForever);
        LCD_Send(data, LCD_RS);
        osMutexRelease(i2cMutexHandle);
}
