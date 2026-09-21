import React from 'react';
import { ShieldCheck, Cpu, AlertTriangle, Play, RotateCcw } from 'lucide-react';

export default function Header({ overallResult, onRunAll, onReset }) {
  const isPass = overallResult === 'PASS';

  return (
    <header style={{
      background: '#0a0d14',
      borderBottom: '1px solid #1e293b',
      padding: '1rem 1.5rem',
      display: 'flex',
      flexDirection: 'column',
      gap: '0.75rem'
    }}>
      {/* Top Title Bar */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '1rem' }}>
        <div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
            <h1 style={{ fontSize: '1.3rem', fontWeight: 800, letterSpacing: '-0.025em', color: '#f8fafc' }}>
              PROJECT 4 — ADAPTIVE CRUISE CONTROL (ACC) PROTOTYPE
            </h1>
            <span className="badge badge-info" style={{ textTransform: 'none' }}>
              B.Tech CS (AI & ML)
            </span>
          </div>
          <p style={{ fontSize: '0.8rem', color: '#94a3b8', marginTop: '2px' }}>
            Course: Intelligent Transportation Systems & Autonomous Mobility | Cascaded Dual-Loop PID Engine
          </p>
        </div>

        {/* Global Action Buttons */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
          <button className="btn btn-primary btn-sm" onClick={onRunAll}>
            <Play size={14} /> RUN ALL ACC SUITE
          </button>
          <button className="btn btn-secondary btn-sm" onClick={onReset}>
            <RotateCcw size={14} /> RESET
          </button>
        </div>
      </div>

      {/* MANDATORY ACADEMIC VALIDATION BANNERS */}
      <div style={{ display: 'flex', gap: '1rem', flexWrap: 'wrap' }}>
        {/* Banner 1: SIMULATION RESULT */}
        <div style={{
          flex: '1',
          minWidth: '280px',
          background: isPass ? 'rgba(16, 185, 129, 0.1)' : 'rgba(244, 63, 94, 0.1)',
          border: `1px solid ${isPass ? 'rgba(16, 185, 129, 0.3)' : 'rgba(244, 63, 94, 0.3)'}`,
          borderRadius: '8px',
          padding: '0.625rem 1rem',
          display: 'flex',
          alignItems: 'center',
          gap: '0.75rem'
        }}>
          <ShieldCheck size={20} color={isPass ? '#10b981' : '#f43f5e'} />
          <div>
            <div style={{ fontSize: '0.7rem', fontWeight: 600, color: '#94a3b8', textTransform: 'uppercase' }}>
              SOFTWARE VERIFICATION STATUS
            </div>
            <div style={{ fontSize: '0.95rem', fontWeight: 800, color: isPass ? '#10b981' : '#f43f5e', fontFamily: 'JetBrains Mono' }}>
              SIMULATION RESULT: {overallResult}
            </div>
          </div>
        </div>

        {/* Banner 2: PHYSICAL VALIDATION PENDING */}
        <div style={{
          flex: '1.5',
          minWidth: '320px',
          background: 'rgba(245, 158, 11, 0.1)',
          border: '1px solid rgba(245, 158, 11, 0.3)',
          borderRadius: '8px',
          padding: '0.625rem 1rem',
          display: 'flex',
          alignItems: 'center',
          gap: '0.75rem'
        }}>
          <Cpu size={20} color="#f59e0b" />
          <div>
            <div style={{ fontSize: '0.7rem', fontWeight: 600, color: '#94a3b8', textTransform: 'uppercase' }}>
              ACADEMIC HARDWARE INTEGRATION STATUS
            </div>
            <div style={{ fontSize: '0.9rem', fontWeight: 700, color: '#f59e0b', fontFamily: 'JetBrains Mono' }}>
              PHYSICAL VALIDATION: PENDING ESP32 + BLDC HARDWARE
            </div>
          </div>
        </div>

        {/* Banner 3: Brushed 4WD Note */}
        <div style={{
          flex: '1',
          minWidth: '260px',
          background: 'rgba(51, 65, 85, 0.5)',
          border: '1px solid rgba(255, 255, 255, 0.1)',
          borderRadius: '8px',
          padding: '0.625rem 1rem',
          display: 'flex',
          alignItems: 'center',
          gap: '0.5rem',
          fontSize: '0.75rem',
          color: '#cbd5e1'
        }}>
          <AlertTriangle size={16} color="#94a3b8" style={{ flexShrink: 0 }} />
          <span>Brushed 4WD + TB6612 platform used for initial hardware prototyping only; not final Project 4 BLDC validation.</span>
        </div>
      </div>
    </header>
  );
}
