import React from 'react';

export default function SystemInfo({ uptime }) {
  const rows = [
    ['Microcontroller', 'STM32F030R8'],
    ['Sensor',          'BME280'],
    ['Firmware',        'v1.0'],
    ['Update rate',     '0.5 Hz'],
    ['Uptime',          uptime],
    ['MQTT QoS',        '0'],
  ];

  return (
    <div className="b-card">
      <div className="b-title">System information</div>
      <div className="sys-grid">
        {rows.map(([k, v]) => (
          <React.Fragment key={k}>
            <span className="sys-k">{k}</span>
            <span className="sys-v">{v}</span>
          </React.Fragment>
        ))}
      </div>
    </div>
  );
}