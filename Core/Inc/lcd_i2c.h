/*
 * lcd_i2c.h
 *
 *  Created on: Jun 23, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Driver header for a 16x2 HD44780-compatible LCD connected via
 *        a PCF8574 I2C I/O expander.
 *
 * The PCF8574 maps its 8 output pins to the HD44780 control and data
 * lines. This driver operates the display in 4-bit mode and uses the
 * FreeRTOS-safe osDelay macro for all timing delays.
 */

#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "stm32f0xx_hal.h"

/* 8-bit I2C address of the PCF8574 expander (7-bit address 0x27 shifted left). */
#define LCD_ADDR      (0x27 << 1)

/* Display geometry. */
#define LCD_COLS      (16)
#define LCD_ROWS      (2)

/* PCF8574 output pin mapping to HD44780 signals.
 *
 * P3 (0x08) -> Backlight transistor (BL)
 * P2 (0x04) -> Enable pulse        (EN)
 * P1 (0x02) -> Read/Write select   (RW) - always 0 (write-only)
 * P0 (0x01) -> Register select     (RS) - 0=command, 1=character data
 */
#define LCD_BACKLIGHT (0x08)
#define LCD_ENABLE    (0x04)
#define LCD_RW        (0x00)   /* RW permanently tied low (write-only) */
#define LCD_RS        (0x01)

/* Delay macro: delegates to osDelay for FreeRTOS compatibility.
 * Using osDelay instead of HAL_Delay yields the CPU to other tasks
 * during display timing gaps. */
#define LCD_DELAY(ms) osDelay(ms)


/*
 * @brief Initialises the LCD and stores the I2C handle for subsequent calls.
 *
 * Performs the HD44780 power-on reset sequence manually (three 0x30 pulses
 * followed by a 4-bit mode switch command), then configures the display
 * for 2-line, 5x8 font, cursor off, and left-to-right entry mode.
 *
 * Must be called once before any other LCD function.
 *
 * @param hi2c  Pointer to the initialised I2C handle (e.g. &hi2c1).
 */
void LCD_Init(I2C_HandleTypeDef *hi2c);

/*
 * @brief Clears all characters from the display and returns the cursor
 *        to position (0, 0).
 */
void LCD_Clear(void);

/*
 * @brief Moves the cursor to the specified column and row.
 *
 * @param col  Column index (0 to LCD_COLS - 1).
 * @param row  Row index (0 to LCD_ROWS - 1).
 */
void LCD_SetCursor(uint8_t col, uint8_t row);

/*
 * @brief Writes a null-terminated string to the display starting at the
 *        current cursor position.
 *
 * @param str  Pointer to the string to display.
 */
void LCD_Print(const char *str);

/*
 * @brief Controls the LCD backlight.
 *
 * @param on_off  1 to enable the backlight, 0 to disable it.
 */
void LCD_Backlight(uint8_t on_off);

#endif /* INC_LCD_I2C_H_ */
