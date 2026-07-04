import React from 'react';
import './MainHeader.css';

export default function MainHeader({ timestamp }) {
  const ts = timestamp ? new Date(timestamp * 1000).toLocaleTimeString() : '--:--:--';

  return (
    <div className="main-hdr">
      <div>
        <h1 className="main-title">Live telemetry</h1>
        <p className="main-sub">STM32 Nucleo-F030R8 · BME280 · FreeRTOS</p>
      </div>
      <div className="ts-chip">Last update · {ts}</div>
    </div>
  );
}