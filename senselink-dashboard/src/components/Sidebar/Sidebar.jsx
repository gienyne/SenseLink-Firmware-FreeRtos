import React from 'react';
import { ALARM_CFG } from '../../constants/config';
import './Sidebar.css';

export default function Sidebar({ connected, msgCount, alarmState, log, resetState, onReset }) {
  const alarmCfg = ALARM_CFG[alarmState];

  return (
    <aside className="sidebar">

      {/* Brand */}
      <div className="sb-brand">
        <div className="sb-mark">SL</div>
        <div>
          <div className="sb-name">SenseLink</div>
          <div className="sb-ver">IoT Monitor v1.0</div>
        </div>
      </div>

      {/* Connection */}
      <div className="sb-block">
        <div className="sb-label">Connection</div>
        <div className={`conn-row ${connected ? 'conn-on' : 'conn-off'}`}>
          <span className="conn-dot" />
          <span className="conn-txt">{connected ? 'Connected' : 'Offline'}</span>
        </div>
        <div className="sb-meta">Mosquitto · localhost:9001</div>
        <div className="sb-meta">{msgCount} messages received</div>
      </div>

      {/* Alarm state */}
      <div className="sb-block">
        <div className="sb-label">Alarm state</div>
        <div
          className="alarm-chip"
          style={{ background: alarmCfg.bg, color: alarmCfg.text, borderColor: alarmCfg.border }}
        >
          <span className="alarm-dot" style={{ background: alarmCfg.text }} />
          {alarmCfg.label}
        </div>
      </div>

      {/* Event log */}
      <div className="sb-block sb-log">
        <div className="sb-label">Event log</div>
        {log.length === 0
          ? <p className="log-empty">Waiting for events…</p>
          : log.map((e, i) => (
            <div key={i} className="log-entry">
              <span className="log-t">{e.t}</span>
              <span className={`log-m log-${e.type}`}>{e.msg}</span>
            </div>
          ))}
      </div>

      {/* Reset button */}
      <div className="sb-foot">
        <button
          className={`btn-reset ${resetState === 'sent' ? 'btn-sent' : ''}`}
          onClick={onReset}
          disabled={resetState === 'sent'}
        >
          {resetState === 'sent' ? 'Command sent' : 'Reset hardware alarm'}
        </button>
      </div>

    </aside>
  );
}