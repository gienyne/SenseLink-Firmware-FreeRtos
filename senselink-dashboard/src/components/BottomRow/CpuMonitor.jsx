import React from 'react';

export default function CpuMonitor({ cpuUsage }) {
  return (
    <div className="b-card">
      <div className="b-title">FreeRTOS CPU tasks</div>
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
  );
}