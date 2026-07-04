import React, { useState, useEffect, useRef } from 'react';
import mqtt from 'mqtt';

import { useUptime } from './hooks/useUptime';
import {
  THRESHOLDS, MAX_HISTORY, ALARM_CFG, DEFAULT_CPU,
  MQTT_URL, TOPIC_DATA, TOPIC_CPU, TOPIC_CMD,
  getAlarmState,
} from './constants/config';

import Sidebar        from './components/Sidebar/Sidebar';
import GaugesRow     from './components/Gauges/GaugesRow';
import TelemetryChart from './components/Chart/TelemetryChart';
import BottomRow     from './components/BottomRow/BottomRow';

import './App.css';

export default function App() {
  const [data, setData] = useState({
    temperature: null, humidity: null,
    pressure: null, alarmState: 1, timestamp: null,
  });
  const [history,    setHistory]    = useState([]);
  const [connected,  setConnected]  = useState(false);
  const [msgCount,   setMsgCount]   = useState(0);
  const [log,        setLog]        = useState([]);
  const [resetState, setResetState] = useState('idle');
  const [cpuUsage,   setCpuUsage]   = useState(DEFAULT_CPU);
  
  // Saving the timestamp of the last data reception
  const [lastSeen, setLastSeen] = useState(0);

  const mqttRef = useRef(null);
  const uptime  = useUptime();

  const pushLog = (msg, type = 'default') =>
    setLog(prev => [{ t: new Date().toLocaleTimeString(), msg, type }, ...prev].slice(0, 7));

  // Watchdog / Heartbeat (Checked every second)
  useEffect(() => {
    const interval = setInterval(() => {
      if (lastSeen === 0) {
        setConnected(false);
        return;
      }
      
      // If a message was received less than 10 seconds ago, the system is "Online"
      const isAlive = Date.now() - lastSeen < 10000; 
      
      setConnected(isAlive);
    }, 1000);

    return () => clearInterval(interval);
  }, [lastSeen]);

  // MQTT connection & message handling 
  useEffect(() => {
    const client = mqtt.connect(MQTT_URL);
    mqttRef.current = client;

    client.on('connect', () => {
      setConnected(true); 
      client.subscribe(TOPIC_DATA);
      client.subscribe(TOPIC_CPU);
      pushLog('Bridge connected successfully', 'ok');
    });

    client.on('error', () => {
      pushLog('Connection error -> check Mosquitto', 'err');
    });

    client.on('close', () => {
      pushLog('Connection closed', 'err');
    });

    client.on('message', (topic, message) => {
      try {
        const payload = JSON.parse(message.toString());

        if (topic === TOPIC_DATA) {

          setLastSeen(Date.now());

          const state = payload.alarmState ?? getAlarmState(payload.temperature, payload.humidity);
          setData(prev => {
            if (prev.alarmState !== state && state !== 1)
              pushLog(`Alarm -> ${ALARM_CFG[state].label}`, state === 3 ? 'err' : 'warn');
            return { ...payload, alarmState: state };
          });
          setMsgCount(c => c + 1);
          const label = new Date((payload.timestamp || Date.now() / 1000) * 1000)
            .toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
          setHistory(h => {
            const next = [...h, { time: label, T: payload.temperature, H: payload.humidity }];
            return next.length > MAX_HISTORY ? next.slice(-MAX_HISTORY) : next;
          });
        }

        if (topic === TOPIC_CPU) {
          setLastSeen(Date.now());
          setCpuUsage(prev => ({ ...prev, [payload.taskName]: payload.cpuUsage }));
        }

      } catch {
        pushLog('Malformed packet received', 'err');
      }
    });

    return () => client.end();
  }, []);

  // Reset command 
  const handleReset = () => {
    if (resetState === 'sent') return;
    mqttRef.current?.publish(TOPIC_CMD, 'RESET');
    pushLog('RESET sent -> STM32', 'warn');
    setResetState('sent');
    setTimeout(() => setResetState('idle'), 2500);
  };

  const ts = data.timestamp
    ? new Date(data.timestamp * 1000).toLocaleTimeString()
    : '--:--:--';

  return (
    <div className="app">
      <Sidebar
        connected={connected}
        msgCount={msgCount}
        alarmState={data.alarmState ?? 1}
        log={log}
        resetState={resetState}
        onReset={handleReset}
      />

      <main className="main">
        <div className="main-hdr">
          <div>
            <h1 className="main-title">Live telemetry</h1>
            <p className="main-sub">STM32 Nucleo-F030R8 · BME280 · FreeRTOS</p>
          </div>
          <div className="ts-chip">Last update · {ts}</div>
        </div>

        <GaugesRow
          temperature={data.temperature}
          humidity={data.humidity}
          pressure={data.pressure}
        />

        <TelemetryChart history={history} />

        <BottomRow
          alarmState={data.alarmState ?? 1}
          uptime={uptime}
          cpuUsage={cpuUsage}
        />
      </main>
    </div>
  );
}