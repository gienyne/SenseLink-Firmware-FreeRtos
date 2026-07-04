import React from 'react';
import HardwareStatus from './HardwareStatus';
import SystemInfo from './SystemInfo';
import CpuMonitor from './CpuMonitor';
import './BottomRow.css';

export default function BottomRow({ alarmState, uptime, cpuUsage }) {
  return (
    <div className="bot-row">
      <HardwareStatus alarmState={alarmState} />
      <SystemInfo uptime={uptime} />
      <CpuMonitor cpuUsage={cpuUsage} />
    </div>
  );
}