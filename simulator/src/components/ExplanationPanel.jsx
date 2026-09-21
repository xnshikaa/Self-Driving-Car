import React from 'react';
import { MessageSquare, ArrowRight, Activity, ShieldCheck, AlertTriangle } from 'lucide-react';

export function getLiveExplanation(telemetry, prevTelemetry) {
  if (!telemetry) return { title: 'SIMULATION READY', detail: 'Select a scenario and click PLAY SIM to begin watching real-time ACC behavior.', status: 'info' };

  const mode = telemetry.mode;
  const gap = telemetry.gap_m;
  const speed = telemetry.speed_mps;
  const targetSpeed = telemetry.target_mps;
  const leadSpeed = telemetry.true_lead_mps;
  const pwm = telemetry.pwm;
  const isBraking = telemetry.braking === 1;

  if (mode === 'OFF') {
    return { title: 'ACC IS OFF', detail: 'Vehicle is stationary. Motor command is zero.', status: 'neutral' };
  }

  if (mode === 'FAULT') {
    return { title: 'SENSOR FAULT DETECTED', detail: 'HC-SR04 ultrasonic sensor stop command active. Motor PWM zeroed in <300 ms for safety.', status: 'danger' };
  }

  if (mode === 'EMERG') {
    return { title: 'EMERGENCY BRAKING ACTIVE', detail: `Gap dropped below safe dynamic limit (${gap.toFixed(2)} m < 0.25 m). Controller applied maximum brake deceleration (2.0 m/s²).`, status: 'danger' };
  }

  if (mode === 'STOP') {
    return { title: 'VEHICLE SAFELY STOPPED', detail: `Lead vehicle has stopped. ACC car brought to a complete stop ${gap.toFixed(2)} m behind lead vehicle. Remaining safely stationary.`, status: 'warning' };
  }

  if (mode === 'SPEED') {
    if (speed < targetSpeed - 0.05) {
      return { title: 'CLEAR ROAD: ACCELERATING', detail: `No lead vehicle within 1.50 m. ACC inner loop accelerating vehicle toward set speed (${targetSpeed.toFixed(2)} m/s). PWM: ${pwm}.`, status: 'info' };
    }
    return { title: 'CLEAR ROAD: MAINTAINING SET SPEED', detail: `Holding set speed (${speed.toFixed(2)} m/s) within ±5% accuracy using inner PID loop (100 Hz).`, status: 'pass' };
  }

  if (mode === 'GAP') {
    const relativeSpeed = leadSpeed - speed;
    if (leadSpeed < 0.08) {
      return { title: 'LEAD VEHICLE BRAKING / STOPPING', detail: `Lead vehicle slowed to ${leadSpeed.toFixed(2)} m/s. Outer Gap controller reduced target speed to ${targetSpeed.toFixed(2)} m/s. Motor braking active.`, status: 'warning' };
    }
    if (Math.abs(gap - 0.50) <= 0.08) {
      return { title: 'SAFE GAP REGULATION (50 cm)', detail: `Maintaining safe 50 cm gap behind lead vehicle (${gap.toFixed(2)} m). Target speed matched to lead speed (${leadSpeed.toFixed(2)} m/s).`, status: 'pass' };
    }
    if (gap < 0.50) {
      return { title: 'CLOSING GAP: SLOWING DOWN', detail: `Gap decreased to ${gap.toFixed(2)} m (< 0.50 m target). Outer controller reduced target speed to increase spacing.`, status: 'warning' };
    }
    return { title: 'CATCHING UP TO LEAD VEHICLE', detail: `Lead vehicle detected at ${gap.toFixed(2)} m. Target speed adjusted to smoothly close distance to 0.50 m.`, status: 'info' };
  }

  return { title: 'SYSTEM ACTIVE', detail: `Mode: ${mode}, Speed: ${speed.toFixed(2)} m/s, Gap: ${gap.toFixed(2)} m`, status: 'info' };
}

export default function ExplanationPanel({ telemetry }) {
  const explanation = getLiveExplanation(telemetry);

  const getBorderColor = (status) => {
    switch (status) {
      case 'pass': return '#10b981';
      case 'warning': return '#f59e0b';
      case 'danger': return '#f43f5e';
      case 'info': return '#06b6d4';
      default: return '#64748b';
    }
  };

  return (
    <div className="card" style={{ borderLeft: `4px solid ${getBorderColor(explanation.status)}` }}>
      <div className="card-header">
        <span className="card-title" style={{ color: getBorderColor(explanation.status) }}>
          <MessageSquare size={16} /> LIVE ACC BEHAVIOR EXPLANATION (REAL-TIME ENGINE STATE)
        </span>
        <span className="badge badge-info">DYNAMIC STATE ENGINE</span>
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: '0.4rem' }}>
        <div style={{ fontSize: '1.05rem', fontWeight: 800, color: '#f8fafc', display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <ArrowRight size={18} color={getBorderColor(explanation.status)} />
          {explanation.title}
        </div>
        <p style={{ fontSize: '0.85rem', color: '#cbd5e1', lineHeight: '1.45', paddingLeft: '1.6rem' }}>
          {explanation.detail}
        </p>
      </div>
    </div>
  );
}
