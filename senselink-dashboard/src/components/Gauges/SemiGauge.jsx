import React from 'react';

export default function SemiGauge({ value, min, max, color, threshold }) {
  const pct  = Math.min(Math.max((value - min) / (max - min), 0), 1);
  const tPct = Math.min(Math.max((threshold - min) / (max - min), 0), 1);
  const cx = 60, cy = 54, r = 44;

  const ptOnArc = (fraction) => {
    const angle = Math.PI + fraction * Math.PI;
    return { x: cx + r * Math.cos(angle), y: cy + r * Math.sin(angle) };
  };

  const start = ptOnArc(0);
  const end   = ptOnArc(1);
  const fill  = ptOnArc(pct);
  const tick  = ptOnArc(tPct);
  const tickI = {
    x: cx + (r - 8) * Math.cos(Math.PI + tPct * Math.PI),
    y: cy + (r - 8) * Math.sin(Math.PI + tPct * Math.PI),
  };

  return (
    <svg viewBox="0 0 120 62" className="gauge-svg">
      {/* Track */}
      <path
        d={`M ${start.x} ${start.y} A ${r} ${r} 0 0 1 ${end.x} ${end.y}`}
        fill="none" stroke="#e8e4de" strokeWidth="7" strokeLinecap="round"
      />
      {/* Fill */}
      {pct > 0.005 && (
        <path
          d={`M ${start.x} ${start.y} A ${r} ${r} 0 ${pct > 0.5 ? 1 : 0} 1 ${fill.x} ${fill.y}`}
          fill="none" stroke={color} strokeWidth="7" strokeLinecap="round"
          style={{ transition: 'all 0.7s cubic-bezier(.4,0,.2,1)' }}
        />
      )}
      {/* Threshold tick */}
      <line
        x1={tickI.x} y1={tickI.y} x2={tick.x} y2={tick.y}
        stroke="rgba(0,0,0,0.18)" strokeWidth="1.5" strokeLinecap="round"
      />
      {/* Value */}
      <text x={cx} y={cy - 4} textAnchor="middle" className="g-val" fill={color}>
        {typeof value === 'number' ? value.toFixed(1) : '--'}
      </text>
    </svg>
  );
}