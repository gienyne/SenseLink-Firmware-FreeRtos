import React from 'react';

const LEDS = [
  { id: 1, color: '#2a7a2a', glow: 'rgba(42,122,42,0.25)',  name: 'GREEN',  state: 'Nominal'  },
  { id: 2, color: '#c47a00', glow: 'rgba(196,122,0,0.25)',  name: 'YELLOW', state: 'Warning'  },
  { id: 3, color: '#c42a2a', glow: 'rgba(196,42,42,0.25)',  name: 'RED',    state: 'Critical' },
];

export default function HardwareStatus({ alarmState }) {
  return (
    <div className="b-card">
      <div className="b-title">Hardware status</div>
      <div className="hw-leds">
        {LEDS.map(l => {
          const isActive = alarmState === l.id;
          return (
            <div key={l.id} className="hw-item">
              <div
                className={`hw-bulb ${isActive ? 'hw-on' : ''} ${isActive && l.id === 3 ? 'hw-blink' : ''}`}
                style={isActive ? {
                  background: l.color,
                  boxShadow: `0 0 0 6px ${l.glow}, 0 0 0 12px ${l.glow.replace('0.25', '0.08')}`
                } : {}}
              />
              <span className="hw-name">{l.name}</span>
              <span className="hw-state" style={{ color: isActive ? l.color : '#b0a898' }}>
                {l.state}
              </span>
            </div>
          );
        })}
      </div>
    </div>
  );
}