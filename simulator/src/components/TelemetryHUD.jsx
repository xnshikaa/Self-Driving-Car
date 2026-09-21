import React from 'react';
import { Gauge, Radio, Activity, AlertOctagon, Brain } from 'lucide-react';

export default function TelemetryHUD({ telemetry, isSensorOk, useAi }) {
  const mode = telemetry ? telemetry.mode : 'SPEED';
  const speed = telemetry ? telemetry.speed_mps : 0.0;
  const targetSpeed = telemetry ? telemetry.target_mps : 0.0;
  const leadSpeed = telemetry ? telemetry.true_lead_mps : 0.0;
  const rawGap = telemetry ? telemetry.raw_gap_m : 1.20;
  const filteredGap = telemetry ? telemetry.gap_m : 1.20;
  const pwm = telemetry ? telemetry.pwm : 0;

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
    <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(180px, 1fr))', gap: '0.75rem' }}>
      {/* 1. ACC MODE */}
      <div className="card" style={{ borderLeft: `4px solid ${getModeColor(mode)}` }}>
        <div className="card-header">
          <span className="card-title"><Activity size={14} /> FSM MODE</span>
          <span className="badge" style={{ background: `${getModeColor(mode)}20`, color: getModeColor(mode) }}>
            {mode}
          </span>
        </div>
        <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: getModeColor(mode) }}>
          {mode}
        </div>
        <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '4px' }}>
          Hysteresis & Dwell Active
        </div>
      </div>

      {/* 2. OUR SPEED / PWM */}
      <div className="card">
        <div className="card-header">
          <span className="card-title"><Gauge size={14} /> VEHICLE SPEED</span>
          <span className="mono" style={{ fontSize: '0.75rem', color: '#94a3b8' }}>PWM: {pwm}</span>
        </div>
        <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#f8fafc' }}>
          {speed.toFixed(2)} <span style={{ fontSize: '0.8rem', fontWeight: 500, color: '#94a3b8' }}>m/s</span>
        </div>
        {/* PWM Bar */}
        <div style={{ background: '#1e293b', height: '4px', borderRadius: '2px', marginTop: '8px', overflow: 'hidden' }}>
          <div style={{ background: '#06b6d4', height: '100%', width: `${Math.min(100, (pwm / 1023) * 100)}%`, transition: 'width 0.1s ease' }} />
        </div>
      </div>

      {/* 3. TARGET SPEED */}
      <div className="card">
        <div className="card-header">
          <span className="card-title">TARGET SPEED</span>
          <span className="mono" style={{ fontSize: '0.75rem', color: '#10b981' }}>LEAD: {leadSpeed.toFixed(2)} m/s</span>
        </div>
        <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: '#06b6d4' }}>
          {targetSpeed.toFixed(2)} <span style={{ fontSize: '0.8rem', fontWeight: 500, color: '#94a3b8' }}>m/s</span>
        </div>
        <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '4px' }}>
          Inner PID Target (100 Hz)
        </div>
      </div>

      {/* 4. KALMAN FILTERED GAP */}
      <div className="card">
        <div className="card-header">
          <span className="card-title"><Radio size={14} /> FILTERED GAP</span>
          <span className="mono" style={{ fontSize: '0.75rem', color: '#94a3b8' }}>RAW: {rawGap.toFixed(2)}m</span>
        </div>
        <div style={{ fontSize: '1.4rem', fontWeight: 800, fontFamily: 'JetBrains Mono', color: filteredGap < 0.25 ? '#f43f5e' : '#10b981' }}>
          {filteredGap.toFixed(2)} <span style={{ fontSize: '0.8rem', fontWeight: 500, color: '#94a3b8' }}>m</span>
        </div>
        <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '4px' }}>
          1-D Kalman Filter State
        </div>
      </div>

      {/* 5. DISTANCE THRESHOLDS */}
      <div className="card">
        <div className="card-header">
          <span className="card-title">SAFETY GAPS</span>
          <span className="badge badge-info">50 cm ±8</span>
        </div>
        <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem', marginTop: '4px' }}>
          <span>Target: <strong style={{ color: '#10b981' }}>0.50 m</strong></span>
          <span>Dmin: <strong style={{ color: '#f59e0b' }}>0.25 m</strong></span>
        </div>
        <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '4px' }}>
          Official Safe Limit: <strong style={{ color: '#f43f5e' }}>0.20 m</strong>
        </div>
      </div>

      {/* 6. SENSOR & AI STATUS */}
      <div className="card">
        <div className="card-header">
          <span className="card-title"><Brain size={14} /> SYSTEM SAFETY</span>
          <span className={`badge ${isSensorOk ? 'badge-pass' : 'badge-fail'}`}>
            {isSensorOk ? 'OK' : 'FAULT'}
          </span>
        </div>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: '6px' }}>
          <span style={{ fontSize: '0.75rem', color: '#94a3b8' }}>HC-SR04:</span>
          <span className="mono" style={{ fontSize: '0.75rem', color: isSensorOk ? '#10b981' : '#f43f5e' }}>
            {isSensorOk ? '20 Hz Active' : '<300ms Shutdown'}
          </span>
        </div>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: '4px' }}>
          <span style={{ fontSize: '0.75rem', color: '#94a3b8' }}>AI Predictor:</span>
          <span className="mono" style={{ fontSize: '0.75rem', color: useAi ? '#06b6d4' : '#64748b' }}>
            {useAi ? 'ACTIVE (0.5s)' : 'OFF'}
          </span>
        </div>
      </div>
    </div>
  );
}
