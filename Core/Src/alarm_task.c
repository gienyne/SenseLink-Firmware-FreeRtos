/*
 * alarm_task.c
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 *
 * @brief Implementation of the alarm management task.
 *
 * This module implements a latching three-state alarm state machine driven
 * by environmental sensor readings. The alarm state is shared with other
 * tasks via the global volatile variables defined here, and can only be
 * reset to Nominal by an explicit hardware reset command received over UART.
 */

#include "alarm_task.h"
#include "sensor_data.h"
#include "stm32f0xx_hal.h"
#include "FreeRTOS.h"
#include "queues.h"

/*
 * Global alarm state shared across tasks.
 *
 * reset_request       : Set to 1 by the UART ISR when a 'R' command is received.
 *                       Consumed here to clear the alarm latch.
 * current_alarm_state : Reflects the current alarm severity (1=Nominal,
 *                       2=Warning, 3=Critical). Read by bme280_task.c to
 *                       include the state in the UART telemetry string.
 *
 * Both are declared volatile because they are written by an ISR context
 * (HAL_UART_RxCpltCallback) and read by task context.
 */
volatile uint8_t reset_request       = 0;
volatile uint8_t current_alarm_state = 1;


void StartTaskAlarm(void const * argument)
{
    SensorData_t receivedData;

    /* Tracks the blinking state of the red LED in Critical mode.
     * Toggled on each queue receive cycle (~2 s period) to produce
     * a visible blink without blocking the scheduler. */
    uint8_t red_led_state = 0;

    for(;;)
    {
        /* Block until a new sensor reading is available, or until
         * the 2-second timeout expires. The timeout ensures the task
         * does not block indefinitely if the sensor pipeline stalls. */
        if(xQueueReceive(AlarmQueueHandle, &receivedData, pdMS_TO_TICKS(2000)) == pdTRUE)
        {
            /* --- Alarm state machine (latching) --- */

            /* State 3 - Critical: both temperature AND humidity exceed their thresholds.
             * This transition is unconditional and always overrides lower states. */
            if(receivedData.temperature > TEMP_THRESHOLD_MAX && receivedData.humidity > HUMIDITY_THRESHOLD_MAX)
            {
                current_alarm_state = 3;
            }

            /* State 2 - Warning: at least one threshold is exceeded.
             * Only escalates from Nominal or Warning; never downgrades a
             * Critical latch. */
            else if(receivedData.temperature > TEMP_THRESHOLD_MAX || receivedData.humidity > HUMIDITY_THRESHOLD_MAX)
            {
                if(current_alarm_state != 3)
                {
                    current_alarm_state = 2;
                }
            }

            /* State 1 - Nominal: all values within range.
             * The latch mechanism prevents automatic return to Nominal:
             * the alarm can only be cleared after a RESET command has been
             * received over UART (reset_request == 1). */
            else
            {
                if(reset_request == 1)
                {
                    current_alarm_state = 1;
                    reset_request = 0;
                }
            }

            /* --- Physical LED output based on current alarm state --- */

            if(current_alarm_state == 3)
            {
                /* Critical: extinguish Green and Yellow, blink Red. */
                HAL_GPIO_WritePin(LED_GREEN_PORT,  LED_GREEN_PIN,  GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);

                /* Toggle the red LED on each cycle to produce a ~2 s blink. */
                red_led_state = !red_led_state;
                HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, red_led_state);
            }
            else if(current_alarm_state == 2)
            {
                /* Warning: extinguish Green and Red, light Yellow steady. */
                HAL_GPIO_WritePin(LED_GREEN_PORT,  LED_GREEN_PIN,  GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED_RED_PORT,    LED_RED_PIN,    GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_SET);
                red_led_state = 0;
            }
            else
            {
                /* Nominal: extinguish Yellow and Red, light Green steady. */
                HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED_RED_PORT,    LED_RED_PIN,    GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED_GREEN_PORT,  LED_GREEN_PIN,  GPIO_PIN_SET);
                red_led_state = 0;
            }
        }
    }
}
