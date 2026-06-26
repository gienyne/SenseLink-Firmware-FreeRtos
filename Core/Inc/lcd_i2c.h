/*
 * lcd_i2c.h
 *
 *  Created on: Jun 23, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "stm32f0xx_hal.h"

#define LCD_ADDR (0x27 << 1) //  0x4e ( PCF8574 I2C Adresse (8-bit Format) )
#define LCD_COLS (16)
#define LCD_ROWS (2)


/* PCF8574 Pin Mapping zum HD44780 */
#define LCD_BACKLIGHT (0x08) // 0x08 = 00001000 => P3 = 1 und P3 => Licht
#define LCD_ENABLE    (0x04) // 0x04 = 00000100 => P2 = 1 und P2 => Enable pulse (EN)
#define LCD_RW        (0x00) // wird immer geschrieben also 0x02 wegen P1 nicht nötig
#define LCD_RS        (0x01) // 0x01 = 00000001 => P0 und P0 => RS(Register select) mit (RS=0 => befehle und RS=1 =>charakter)

/* Delay-Makro: verwendet osDelay (FreeRTOS-kompatibel) */
#define LCD_DELAY(ms) osDelay(ms)

/**
 * @brief Initialisiert das LCD und speichert das I2C-Handle intern
 * @param hi2c Zeiger auf den initialisierten I2C-Bus (zb &hi2c1)
 */
void LCD_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Löscht den LCD-Inhalt
 */
void LCD_Clear(void);

/**
 * @brief Setzt den Cursor auf eine bestimmte Position
 * @param col Spalte (0-15)
 * @param row Zeile (0-1)
 */
void LCD_SetCursor(uint8_t col, uint8_t row);

/**
 * @brief Gibt einen String auf dem LCD aus
 * @param str Zu druckender String
 */
void LCD_Print(const char *str);

/**
 * @brief Schaltet das Hintergrundlicht ein oder aus
 * @param on_off 1 für EIN, 0 für AUS
 */
void LCD_Backlight(uint8_t on_off);

#endif /* INC_LCD_I2C_H_ */
