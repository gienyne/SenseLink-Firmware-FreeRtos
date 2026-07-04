/*
 * lcd_i2c.c
 *
 *  Created on: Jun 23, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Low-level driver implementation for a 16x2 HD44780 LCD connected
 *        via a PCF8574 I2C I/O expander operating in 4-bit mode.
 *
 * All I2C transmissions are protected by the shared i2cMutexHandle to
 * prevent bus collisions with the BME280 sensor task, which uses the
 * same I2C1 peripheral.
 */

#include "lcd_i2c.h"
#include "cmsis_os.h"

/* Internal state retained across calls. */
static I2C_HandleTypeDef *lcd_i2c_handle = NULL;
static uint8_t lcd_backlight_state       = LCD_BACKLIGHT;

/* Shared I2C mutex defined in main.c, also used by bme280_task.c. */
extern osMutexId i2cMutexHandle;

/* Forward declarations for internal helper functions. */
static void LCD_WritePointer(uint8_t data);
static void LCD_EnablePulse(uint8_t data);
static void LCD_Send(uint8_t val, uint8_t mode);
static void LCD_SendCommand(uint8_t cmd);
static void LCD_SendData(uint8_t data);


void LCD_Init(I2C_HandleTypeDef *hi2c)
{
    lcd_i2c_handle = hi2c;

    /* Wait for the LCD power supply to stabilise after MCU reset. */
    LCD_DELAY(50);

    /* HD44780 software reset sequence (datasheet).
     * Three 0x30 pulses switch the controller from an undefined state
     * to 8-bit mode, after which a 0x20 pulse selects 4-bit mode. */
    LCD_WritePointer(0x30); LCD_EnablePulse(0x30);
    LCD_DELAY(5);
    LCD_WritePointer(0x30); LCD_EnablePulse(0x30);
    LCD_DELAY(1);
    LCD_WritePointer(0x30); LCD_EnablePulse(0x30);
    LCD_DELAY(10);

    /* Switch to 4-bit interface mode. */
    LCD_WritePointer(0x20); LCD_EnablePulse(0x20);
    LCD_DELAY(10);

    /* Function set: 4-bit bus, 2 display lines, 5x8 character font. */
    LCD_SendCommand(0x28);
    LCD_DELAY(1);

    /* Display control: display ON, cursor OFF, blink OFF. */
    LCD_SendCommand(0x0C);
    LCD_DELAY(1);

    /* Entry mode: increment cursor position after each character write*/
    LCD_SendCommand(0x06);
    LCD_DELAY(1);

    LCD_Clear();
}


void LCD_Clear(void)
{
    /* Clear display command (0x01): clears all DDRAM and returns cursor
     * to home position. Requires a 2 ms execution delay. */
    LCD_SendCommand(0x01);
    LCD_DELAY(2);
}


void LCD_SetCursor(uint8_t col, uint8_t row)
{
    /* DDRAM address offsets for each row on a 16x2 display. */
    uint8_t row_offsets[] = {0x00, 0x40};

    /* Clamp parameters to the valid display range. */
    if (row >= LCD_ROWS) row = LCD_ROWS - 1;
    if (col >= LCD_COLS) col = LCD_COLS - 1;

    /* Set DDRAM address command (0x80 | address). */
    LCD_SendCommand(0x80 | (col + row_offsets[row]));
}


void LCD_Print(const char *str)
{
    /* Write each character until the null terminator is reached. */
    while (*str)
    {
        LCD_SendData((uint8_t)(*str));
        str++;
    }
}


void LCD_Backlight(uint8_t on_off)
{
    /* Update the backlight bit retained in every subsequent PCF8574 write. */
    lcd_backlight_state = on_off ? LCD_BACKLIGHT : 0x00;

    /* Send a dummy byte to apply the new backlight state immediately. */
    LCD_WritePointer(0x00);
}


/*
 * @brief Writes one byte to the PCF8574 over I2C.
 *
 *the data byte with the current backlight state before transmitting
 * so that the backlight bit is preserved on every write.
 *
 * This function does NOT hold the I2C mutex. Callers that require mutual
 * exclusion (LCD_SendCommand, LCD_SendData) must acquire the mutex before
 * calling functions that eventually reach this one.
 *
 * @param data  Byte to write to the PCF8574 output register.
 */
static void LCD_WritePointer(uint8_t data)
{
    if (lcd_i2c_handle != NULL)
    {
        uint8_t tx_data = data | lcd_backlight_state;
        HAL_I2C_Master_Transmit(lcd_i2c_handle, LCD_ADDR, &tx_data, 1, 100);
    }
}


/*
 * @brief Generates a single Enable pulse to latch data into the HD44780.
 *
 * The HD44780 reads its data bus on the falling edge of the Enable signal.
 * A 1 ms delay is inserted around each edge to satisfy the controller's
 * minimum pulse-width timing requirement.
 *
 * @param data  Data byte currently on the bus (EN bit will be set/cleared).
 */
static void LCD_EnablePulse(uint8_t data)
{
    LCD_WritePointer(data | LCD_ENABLE);   /* EN high: rising edge */
    LCD_DELAY(1);
    LCD_WritePointer(data & ~LCD_ENABLE);  /* EN low:  falling edge (latch) */
    LCD_DELAY(1);
}


/*
 * @brief Sends one full byte to the HD44780 as two 4-bit nibbles.
 *
 * In 4-bit mode each byte is split into a high nibble (bits 7-4) and a
 * low nibble (bits 3-0), each latched separately with an Enable pulse.
 *
 * @param data  Byte to send (command or character data).
 * @param mode  0 for command (RS=0), LCD_RS for character data (RS=1).
 */
static void LCD_Send(uint8_t data, uint8_t mode)
{
    uint8_t high_nibble = (data & 0xF0) | mode;
    LCD_WritePointer(high_nibble);
    LCD_EnablePulse(high_nibble);

    uint8_t low_nibble = ((data << 4) & 0xF0) | mode;
    LCD_WritePointer(low_nibble);
    LCD_EnablePulse(low_nibble);
}


/*
 * @brief Sends a command byte to the HD44780 (RS = 0) under mutex protection.
 *
 * Acquires the shared I2C mutex before the transfer and releases it
 * after the full two-nibble sequence completes, preventing concurrent
 * I2C access by the BME280 sensor task.
 *
 * @param cmd  HD44780 command byte.
 */
static void LCD_SendCommand(uint8_t cmd)
{
    osMutexWait(i2cMutexHandle, osWaitForever);
    LCD_Send(cmd, 0);
    osMutexRelease(i2cMutexHandle);
}


/*
 * @brief Sends a character data byte to the HD44780 (RS = 1) under mutex protection.
 *
 * Acquires the shared I2C mutex before the transfer and releases it
 * after the full two-nibble sequence completes.
 *
 * @param data  ASCII character byte to write to the current DDRAM position.
 */
static void LCD_SendData(uint8_t data)
{
    osMutexWait(i2cMutexHandle, osWaitForever);
    LCD_Send(data, LCD_RS);
    osMutexRelease(i2cMutexHandle);
}
