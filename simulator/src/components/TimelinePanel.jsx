import React from 'react';
import { Clock, CheckCircle, AlertCircle } from 'lucide-react';

export default function TimelinePanel({ events, currentSimTime }) {
  if (!events || events.length === 0) {
    return (
      <div className="card">
        <div className="card-header">
          <span className="card-title" style={{ color: '#8b5cf6' }}>
            <Clock size={16} /> SCENARIO EVENT TIMELINE
          </span>
        </div>
        <div style={{ fontSize: '0.8rem', color: '#94a3b8', fontStyle: 'italic' }}>
          No state transitions recorded yet. Press PLAY SIM to watch timeline events populate.
        </div>
      </div>
    );
  }

  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title" style={{ color: '#8b5cf6' }}>
          <Clock size={16} /> SCENARIO EVENT TIMELINE
        </span>
        <span className="mono" style={{ fontSize: '0.75rem', color: '#94a3b8' }}>
          {events.length} TRANSITIONS RECORDED
        </span>
      </div>

      <div style={{ display: 'flex', gap: '0.75rem', overflowX: 'auto', paddingBottom: '0.5rem' }}>
        {events.map((ev, idx) => {
          const isPast = currentSimTime >= ev.t;
          return (
            <div
              key={idx}
              style={{
                minWidth: '170px',
                background: isPast ? '#0f172a' : 'rgba(15, 23, 42, 0.4)',
                border: `1px solid ${isPast ? '#334155' : 'rgba(255, 255, 255, 0.05)'}`,
                borderRadius: '8px',
                padding: '0.5rem 0.75rem',
                opacity: isPast ? 1 : 0.5,
                transition: 'all 0.2s ease'
              }}
            >
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '4px' }}>
                <span className="mono" style={{ fontSize: '0.75rem', fontWeight: 700, color: '#06b6d4' }}>
                  {ev.t.toFixed(2)}s
                </span>
                <span className="badge badge-info" style={{ fontSize: '0.65rem', padding: '1px 4px' }}>
                  {ev.mode}
                </span>
              </div>
              <div style={{ fontSize: '0.72rem', color: '#cbd5e1' }}>
                Gap: <strong>{ev.gap.toFixed(2)}m</strong>
              </div>
              <div style={{ fontSize: '0.72rem', color: '#cbd5e1' }}>
                Speed: <strong>{ev.speed.toFixed(2)}m/s</strong>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
