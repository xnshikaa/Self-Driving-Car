import React, { useState } from 'react';
import { Gauge, Radio, Activity, Eye, Sliders, Cpu } from 'lucide-react';

export default function BeginnerVsEngineeringHUD({ telemetry, isSensorOk, useAi }) {
  const [viewMode, setViewMode] = useState('beginner'); // 'beginner' | 'engineering'

  const mode = telemetry ? telemetry.mode : 'SPEED';
  const speed = telemetry ? telemetry.speed_mps : 0.0;
  const targetSpeed = telemetry ? telemetry.target_mps : 0.0;
  const leadSpeed = telemetry ? telemetry.true_lead_mps : 0.0;
  const rawGap = telemetry ? telemetry.raw_gap_m : 1.20;
  const filteredGap = telemetry ? telemetry.gap_m : 1.20;
  const pwm = telemetry ? telemetry.pwm : 0;
  const tracking = telemetry ? telemetry.tracking === 1 : false;

  const getModeColor = (m) => {
    switch (m) {
      case 'SPEED': return '#3b82f6';
      case 'GAP': return '#10b981';
      case 'STOP': return '#f59e0b';
      case 'EMERG': return '#f43f5e';
      case 'FAULT': return '#e11d48';
      default: return '#94a3b8';
    }
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem' }}>
      {/* View Mode Toggle Header */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', background: '#0f172a', padding: '0.5rem 1rem', borderRadius: '8px', border: '1px solid #1e293b' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <Eye size={16} color="#06b6d4" />
          <span style={{ fontSize: '0.85rem', fontWeight: 700, color: '#f8fafc' }}>
            TELEMETRY DISPLAY MODE:
          </span>
        </div>

        <div style={{ display: 'flex', gap: '0.5rem' }}>
          <button
            className={`btn btn-sm ${viewMode === 'beginner' ? 'btn-primary' : 'btn-secondary'}`}
            onClick={() => setViewMode('beginner')}
          >
            BEGINNER VIEW (ESSENTIAL)
          </button>
          <button
            className={`btn btn-sm ${viewMode === 'engineering' ? 'btn-primary' : 'btn-secondary'}`}
            onClick={() => setViewMode('engineering')}
          >
            ENGINEERING VIEW (FULL SPECS)
          </button>
        </div>
      </div>

      {/* BEGINNER VIEW HUD */}
      {viewMode === 'beginner' && (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(170px, 1fr))', gap: '0.75rem' }}>
          <div className="card" style={{ borderLeft: `4px solid ${getModeColor(mode)}` }}>
            <div className="card-header"><span className="card-title">CURRENT MODE</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: getModeColor(mode) }}>{mode}</div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>FSM Controller State</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">CURRENT GAP</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: filteredGap < 0.25 ? '#f43f5e' : '#10b981' }}>
              {filteredGap.toFixed(2)} <span style={{ fontSize: '0.8rem', color: '#94a3b8' }}>m</span>
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Distance to Lead Car</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">TARGET GAP</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#10b981' }}>
              0.50 <span style={{ fontSize: '0.8rem', color: '#94a3b8' }}>m</span>
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Desired Gap (50 cm ±8)</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">YOUR SPEED</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#f8fafc' }}>
              {speed.toFixed(2)} <span style={{ fontSize: '0.8rem', color: '#94a3b8' }}>m/s</span>
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Measured Wheel Speed</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">LEAD SPEED</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#f43f5e' }}>
              {leadSpeed.toFixed(2)} <span style={{ fontSize: '0.8rem', color: '#94a3b8' }}>m/s</span>
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Lead Vehicle Velocity</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">TARGET SPEED</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#06b6d4' }}>
              {targetSpeed.toFixed(2)} <span style={{ fontSize: '0.8rem', color: '#94a3b8' }}>m/s</span>
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Outer Loop Command</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">PWM COMMAND</span></div>
            <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#8b5cf6' }}>
              {pwm} <span style={{ fontSize: '0.75rem', color: '#94a3b8' }}>/ 1023</span>
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Motor Drive Power</div>
          </div>

          <div className="card">
            <div className="card-header"><span className="card-title">SAFETY STATUS</span></div>
            <div style={{ fontSize: '1.0rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: filteredGap < 0.20 ? '#f43f5e' : (filteredGap < 0.25 ? '#f59e0b' : '#10b981') }}>
              {filteredGap < 0.20 ? 'EMERGENCY' : (filteredGap < 0.25 ? 'WARNING' : 'SAFE (OK)')}
            </div>
            <div style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Min Safe Limit 20 cm</div>
          </div>
        </div>
      )}

      {/* ENGINEERING VIEW HUD */}
      {viewMode === 'engineering' && (
        <div className="card" style={{ background: '#0a0d14', border: '1px solid #334155' }}>
          <div className="card-header">
            <span className="card-title" style={{ color: '#06b6d4' }}>
              <Cpu size={16} /> ADVANCED TELEMETRY & INTERNAL ESTIMATION ENGINE STATE
            </span>
            <span className="mono" style={{ fontSize: '0.75rem', color: '#10b981' }}>
              INNER LOOP: 100 Hz | OUTER LOOP: 50 Hz | SENSOR: 20 Hz
            </span>
          </div>

          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', gap: '1rem', marginTop: '0.5rem' }}>
            <div>
              <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#94a3b8' }}>ULTRASONIC SENSOR TELEMETRY</div>
              <div style={{ fontSize: '0.85rem', marginTop: '4px' }}>Raw Distance: <strong className="mono">{rawGap.toFixed(2)} m</strong></div>
              <div style={{ fontSize: '0.85rem' }}>Filtered (Median of 5): <strong className="mono">{filteredGap.toFixed(2)} m</strong></div>
              <div style={{ fontSize: '0.7rem', color: tracking ? '#10b981' : '#f59e0b', marginTop: '2px' }}>
                Status: {tracking ? 'Kalman Lock Active' : 'Acquiring Target (Searching)'}
              </div>
            </div>

            <div>
              <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#94a3b8' }}>KALMAN ESTIMATOR STATE</div>
              <div style={{ fontSize: '0.85rem', marginTop: '4px' }}>Estimated Gap: <strong className="mono">{filteredGap.toFixed(2)} m</strong></div>
              <div style={{ fontSize: '0.85rem' }}>Estimated Lead Speed: <strong className="mono">{telemetry ? telemetry.lead_mps.toFixed(2) : '0.00'} m/s</strong></div>
              <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '2px' }}>Innovation Gate: ±0.35 m</div>
            </div>

            <div>
              <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#94a3b8' }}>PID & FEED-FORWARD CONTROLLER</div>
              <div style={{ fontSize: '0.85rem', marginTop: '4px' }}>Feed-forward: <strong className="mono">{(150 + 1000 * targetSpeed).toFixed(0)} PWM</strong></div>
              <div style={{ fontSize: '0.85rem' }}>Speed Error: <strong className="mono">{(targetSpeed - speed).toFixed(3)} m/s</strong></div>
              <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '2px' }}>Integral Band: ±0.15 m/s</div>
            </div>

            <div>
              <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#94a3b8' }}>REQUIREMENT VS INTERNAL THRESHOLDS</div>
              <div style={{ fontSize: '0.85rem', marginTop: '4px' }}>Official R3/R5 Safe Gap: <strong className="mono" style={{ color: '#f43f5e' }}>0.20 m</strong></div>
              <div style={{ fontSize: '0.85rem' }}>Internal Tuned Dmin: <strong className="mono" style={{ color: '#f59e0b' }}>0.25 m</strong></div>
              <div style={{ fontSize: '0.85rem' }}>Target Gap (R2): <strong className="mono" style={{ color: '#10b981' }}>0.50 m ±0.08</strong></div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
