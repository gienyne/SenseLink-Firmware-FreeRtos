/*
 * bme280_task.h
 *
 *  Created on: Jun 21, 2026
 *      Author: Dimitry Ntofeu Nyatcha
 */

#ifndef INC_BME280_TASK_H_
#define INC_BME280_TASK_H_

#include "cmsis_os.h"

/**
 * liest periodisch Daten vom BME280 Sensor und sendet sie über die SensorDataQueue.
 * @param argument: nicht verwendet
 */
void StartTaskSensor(void const * argument);

#endif /* INC_BME280_TASK_H_ */
