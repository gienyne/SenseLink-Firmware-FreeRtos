import React from 'react';
import SemiGauge from './SemiGauge';
import { THRESHOLDS } from '../../constants/config';
import './Gauges.css';

export default function GaugesRow({ temperature, humidity, pressure }) {
  return (
    <div className="gauges-row">

      <div className="g-card">
        <div className="g-eyebrow"> Temperature</div>
        <SemiGauge
          value={temperature ?? 0}
          min={0} max={50}
          color="#c45c28"
          threshold={THRESHOLDS.temp}
        />
        <div className="g-bottom">
          <span className="g-unit">°C</span>
          <span className="g-thresh">Threshold · {THRESHOLDS.temp} °C</span>
        </div>
      </div>

      <div className="g-card">
        <div className="g-eyebrow"> Humidity</div>
        <SemiGauge
          value={humidity ?? 0}
          min={0} max={100}
          color="#1e5fa0"
          threshold={THRESHOLDS.hum}
        />
        <div className="g-bottom">
          <span className="g-unit">%</span>
          <span className="g-thresh">Threshold · {THRESHOLDS.hum} %</span>
        </div>
      </div>

      <div className="g-card g-press">
        <div className="g-eyebrow"> Pressure</div>
        <div className="press-num">
          {pressure?.toFixed(1) ?? '--'}
          <span className="press-u">hPa</span>
        </div>
        <div className="press-bar-track">
          <div
            className="press-bar-fill"
            style={{
              width: pressure
                ? Math.min(((pressure - 950) / 100) * 100, 100) + '%'
                : '0%'
            }}
          />
        </div>
        <div className="press-range">
          <span>950</span>
          <span>1050 hPa</span>
        </div>
      </div>

    </div>
  );
}