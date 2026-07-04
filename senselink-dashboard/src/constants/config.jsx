export const THRESHOLDS = { temp: 30.0, hum: 55.0 };
export const MAX_HISTORY = 20;
export const BOOT_TIME = Date.now();

export const MQTT_URL   = 'ws://localhost:9001';
export const TOPIC_DATA = 'senselink/data';
export const TOPIC_CPU  = 'senselink/cpu';
export const TOPIC_CMD  = 'senselink/cmd';

export const ALARM_CFG = {
  1: { label: 'Nominal',  bg: '#f0faf0', text: '#2d6a2d', border: '#b8ddb8' },
  2: { label: 'Warning',  bg: '#fffbeb', text: '#92400e', border: '#fcd34d' },
  3: { label: 'Critical', bg: '#fef2f2', text: '#991b1b', border: '#fca5a5' },
};

export const DEFAULT_CPU = {
  IDLE:        98,
  TaskSensor:  0,
  TaskUART:    0,
  TaskLCD:     0,
  TaskAlarm:   0,
};

export function getAlarmState(t, h) {
  if (t > THRESHOLDS.temp && h > THRESHOLDS.hum) return 3;
  if (t > THRESHOLDS.temp || h > THRESHOLDS.hum) return 2;
  return 1;
}