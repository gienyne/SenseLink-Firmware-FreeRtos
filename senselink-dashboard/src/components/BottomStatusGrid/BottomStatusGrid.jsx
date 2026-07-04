import React from 'react';
import { useUptime } from '../../hooks/useUptime';
import './BottomStatusGrid.css';

export default function BottomStatusGrid({ alarmState, cpuUsage }) {
  const uptime = useUptime();
  const alarm = alarmState ?? 1;

  const leds = [
    { id: 1, color: '#2a7a2a', glow: 'rgba(42,122,42,0.25)', name: 'GREEN', state: 'Nominal' },
    { id: 2, color: '#c47a00', glow: 'rgba(196,122,0,0.25)', name: 'YELLOW', state: 'Warning' },
    { id: 3, color: '#c42a2a', glow: 'rgba(196,42,42,0.25)', name: 'RED', state: 'Critical' },
  ];

  const sysInfo = [
    ['Microcontroller', 'STM32F030R8Tx'],
    ['Sensor', 'BME280'],
    ['Firmware', 'v1.2.3'],
    ['Update rate', '0.5 Hz'],
    ['Uptime', uptime],
    ['MQTT QoS', '0'],
  ];

  return (
    <div className="bot-row">
      <div className="b-card">
        <div className="b-title">Hardware status</div>
        <div className="hw-leds">
          {leds.map(l => (
            <div key={l.id} className="hw-item">
              <div
                className={`hw-bulb ${alarm === l.id ? 'hw-on' : ''} ${alarm === l.id && l.id === 3 ? 'hw-blink' : ''}`}
                style={alarm === l.id ? { background: l.color, boxShadow: `0 0 0 6px ${l.glow}, 0 0 0 12px ${l.glow.replace('0.25', '0.08')}` } : {}}
              />
              <span className="hw-name">{l.name}</span>
              <span className="hw-state" style={{ color: alarm === l.id ? l.color : '#b0a898' }}>
                {l.state}
              </span>
            </div>
          ))}
        </div>
      </div>

      <div className="b-card">
        <div className="b-title">System information</div>
        <div className="sys-grid">
          {sysInfo.map(([k, v]) => (
            <React.Fragment key={k}>
              <span className="sys-k">{k}</span>
              <span className="sys-v">{v}</span>
            </React.Fragment>
          ))}
        </div>
      </div>

      <div className="b-card">
        <div className="b-title">FreeRTOS CPU Tasks</div>
        <div className="cpu-list">
          {Object.entries(cpuUsage).map(([task, val]) => (
            <div key={task} className="cpu-row-item">
              <div className="cpu-meta-info">
                <span className="cpu-task-name">{task}</span>
                <span className="cpu-task-val">{val}%</span>
              </div>
              <div className="cpu-progress-bar-track">
                <div
                  className="cpu-progress-bar-fill"
                  style={{
                    width: `${val}%`,
                    background: task === 'IDLE' ? '#4caf50' : '#4a90e2',
                    transition: 'width 0.8s ease-in-out',
                  }}
                />
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}