import React from 'react';
import {
  ResponsiveContainer, AreaChart, Area,
  XAxis, YAxis, Tooltip, CartesianGrid
} from 'recharts';
import { MAX_HISTORY } from '../../constants/config';
import './Chart.css';

function ChartTooltip({ active, payload, label }) {
  if (!active || !payload?.length) return null;
  return (
    <div className="ctip">
      <div className="ctip-time">{label}</div>
      {payload.map(p => (
        <div key={p.dataKey} className="ctip-row">
          <span className="ctip-dot" style={{ background: p.color }} />
          {p.name}: <strong>{p.value?.toFixed(1)}</strong>
        </div>
      ))}
    </div>
  );
}

export default function TelemetryChart({ history }) {
  return (
    <div className="chart-card">
      <div className="chart-hdr">
        <span className="chart-title">
          Temperature &amp; humidity -- last {MAX_HISTORY} readings
        </span>
        <div className="chart-legend">
          <span><span className="leg-dot" style={{ background: '#c45c28' }} />Temp (°C)</span>
          <span><span className="leg-dot" style={{ background: '#1e5fa0' }} />Hum (%)</span>
        </div>
      </div>

      {history.length < 2
        ? <div className="chart-empty">Collecting data…</div>
        : (
          <ResponsiveContainer width="100%" height={200}>
            <AreaChart data={history} margin={{ top: 6, right: 4, left: -18, bottom: 0 }}>
              <defs>
                <linearGradient id="gT" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%"  stopColor="#c45c28" stopOpacity={0.18} />
                  <stop offset="95%" stopColor="#c45c28" stopOpacity={0} />
                </linearGradient>
                <linearGradient id="gH" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%"  stopColor="#1e5fa0" stopOpacity={0.14} />
                  <stop offset="95%" stopColor="#1e5fa0" stopOpacity={0} />
                </linearGradient>
              </defs>
              <CartesianGrid strokeDasharray="3 3" stroke="#e8e4de" vertical={false} />
              <XAxis
                dataKey="time"
                tick={{ fontSize: 10, fill: '#9a8f80' }}
                tickLine={false} axisLine={false}
                interval="preserveStartEnd"
              />
              <YAxis
                tick={{ fontSize: 10, fill: '#9a8f80' }}
                tickLine={false} axisLine={false}
              />
              <Tooltip content={<ChartTooltip />} />
              <Area type="monotone" dataKey="T" name="Temp" stroke="#c45c28" strokeWidth={2} fill="url(#gT)" dot={false} />
              <Area type="monotone" dataKey="H" name="Hum"  stroke="#1e5fa0" strokeWidth={2} fill="url(#gH)" dot={false} />
            </AreaChart>
          </ResponsiveContainer>
        )}
    </div>
  );
}