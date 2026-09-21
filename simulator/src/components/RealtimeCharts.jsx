import React from 'react';
import {
  ResponsiveContainer, LineChart, Line, XAxis, YAxis, Tooltip, Legend, ReferenceLine, CartesianGrid
} from 'recharts';

export default function RealtimeCharts({ history }) {
  // Use last 150 history samples (approx 7.5 seconds) for crisp real-time plotting
  const data = history.slice(-150);

  return (
    <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(420px, 1fr))', gap: '1rem' }}>
      {/* Chart 1: Gap Telemetry */}
      <div className="card">
        <div className="card-header">
          <span className="card-title" style={{ color: '#06b6d4' }}>GRAPH 1: ULTRASONIC GAP & KALMAN FILTER</span>
          <span className="mono" style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Target: 0.50m | Dmin: 0.25m</span>
        </div>
        <div style={{ width: '100%', height: '220px' }}>
          <ResponsiveContainer>
            <LineChart data={data} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis dataKey="t" stroke="#64748b" fontSize={10} tickFormatter={(val) => `${val}s`} />
              <YAxis stroke="#64748b" fontSize={10} domain={[0, 3.0]} />
              <Tooltip contentStyle={{ background: '#0f172a', border: '1px solid #334155', fontSize: '11px' }} />
              <Legend wrapperStyle={{ fontSize: '11px' }} />

              <ReferenceLine y={0.50} stroke="#10b981" strokeDasharray="3 3" label={{ value: 'Target 0.50m', fill: '#10b981', fontSize: 10 }} />
              <ReferenceLine y={0.25} stroke="#f59e0b" strokeDasharray="3 3" label={{ value: 'Dmin 0.25m', fill: '#f59e0b', fontSize: 10 }} />
              <ReferenceLine y={0.20} stroke="#f43f5e" strokeDasharray="2 2" label={{ value: 'Official 0.20m', fill: '#f43f5e', fontSize: 10 }} />

              <Line type="monotone" dataKey="raw_gap_m" name="Raw Gap (m)" stroke="#94a3b8" strokeWidth={1} dot={false} isAnimationActive={false} />
              <Line type="monotone" dataKey="gap_m" name="Kalman Gap (m)" stroke="#06b6d4" strokeWidth={2} dot={false} isAnimationActive={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Chart 2: Speed Dynamics */}
      <div className="card">
        <div className="card-header">
          <span className="card-title" style={{ color: '#3b82f6' }}>GRAPH 2: SPEED DYNAMICS & LEAD TRACKING</span>
          <span className="mono" style={{ fontSize: '0.7rem', color: '#94a3b8' }}>100 Hz Speed PID</span>
        </div>
        <div style={{ width: '100%', height: '220px' }}>
          <ResponsiveContainer>
            <LineChart data={data} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis dataKey="t" stroke="#64748b" fontSize={10} tickFormatter={(val) => `${val}s`} />
              <YAxis stroke="#64748b" fontSize={10} domain={[0, 0.8]} />
              <Tooltip contentStyle={{ background: '#0f172a', border: '1px solid #334155', fontSize: '11px' }} />
              <Legend wrapperStyle={{ fontSize: '11px' }} />

              <Line type="monotone" dataKey="speed_mps" name="Car Speed (m/s)" stroke="#3b82f6" strokeWidth={2} dot={false} isAnimationActive={false} />
              <Line type="monotone" dataKey="target_mps" name="Target Speed (m/s)" stroke="#06b6d4" strokeWidth={1.5} strokeDasharray="4 4" dot={false} isAnimationActive={false} />
              <Line type="monotone" dataKey="lead_mps" name="Lead Speed (m/s)" stroke="#f43f5e" strokeWidth={1.5} dot={false} isAnimationActive={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Chart 3: PWM Motor Command */}
      <div className="card">
        <div className="card-header">
          <span className="card-title" style={{ color: '#8b5cf6' }}>GRAPH 3: MOTOR COMMAND (PWM 0-1023)</span>
          <span className="mono" style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Max Step: 25 / 10ms</span>
        </div>
        <div style={{ width: '100%', height: '220px' }}>
          <ResponsiveContainer>
            <LineChart data={data} margin={{ top: 10, right: 10, left: -10, bottom: 0 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis dataKey="t" stroke="#64748b" fontSize={10} tickFormatter={(val) => `${val}s`} />
              <YAxis stroke="#64748b" fontSize={10} domain={[0, 1023]} />
              <Tooltip contentStyle={{ background: '#0f172a', border: '1px solid #334155', fontSize: '11px' }} />
              <Legend wrapperStyle={{ fontSize: '11px' }} />

              <ReferenceLine y={150} stroke="#64748b" strokeDasharray="3 3" label={{ value: 'Deadband 150', fill: '#64748b', fontSize: 10 }} />
              <Line type="stepAfter" dataKey="pwm" name="PWM Command" stroke="#8b5cf6" strokeWidth={1.5} dot={false} isAnimationActive={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>
    </div>
  );
}
