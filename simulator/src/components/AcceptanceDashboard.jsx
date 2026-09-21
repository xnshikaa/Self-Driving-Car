import React from 'react';
import { CheckCircle2, XCircle, Clock, ShieldCheck } from 'lucide-react';

export default function AcceptanceDashboard({ acceptanceResults, trialStats, sensorFailResult, isRunning }) {
  const getStatusBadge = (ok) => {
    if (isRunning) {
      return (
        <span className="badge badge-info" style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
          <Clock size={12} /> RUNNING
        </span>
      );
    }
    if (ok === true) {
      return (
        <span className="badge badge-pass" style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
          <CheckCircle2 size={12} /> SIMULATION PASS
        </span>
      );
    }
    if (ok === false) {
      return (
        <span className="badge badge-fail" style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
          <XCircle size={12} /> SIMULATION FAIL
        </span>
      );
    }
    return <span className="badge badge-warning">NOT RUN</span>;
  };

  const getCheckValue = (name) => {
    if (!acceptanceResults) return null;
    const match = acceptanceResults.find(c => c.name.toLowerCase().includes(name.toLowerCase()));
    return match ? match : null;
  };

  const testA = getCheckValue('speed hold');
  const testB = getCheckValue('gap hold');
  const testC = getCheckValue('stop-and-go');
  const testD = getCheckValue('mode switch');
  const testE = getCheckValue('never closer');
  const testDmin = getCheckValue('our own dmin');

  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title" style={{ fontSize: '1rem', color: '#f8fafc' }}>
          <ShieldCheck size={18} color="#10b981" /> OFFICIAL PROJECT 4 ACCEPTANCE TESTS (REQUIREMENTS R1 - R10)
        </span>
        <span className="mono" style={{ fontSize: '0.75rem', color: '#94a3b8' }}>
          Academic Integrity: All Results Are Simulation-Based
        </span>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(280px, 1fr))', gap: '0.75rem', marginTop: '0.75rem' }}>
        {/* Test A */}
        <div style={{ background: '#0f172a', border: '1px solid #1e293b', borderRadius: '8px', padding: '0.75rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 700, fontSize: '0.85rem' }}>TEST A — Speed Hold (R1)</span>
            {getStatusBadge(testA ? testA.ok : null)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8', marginTop: '4px' }}>
            Requirement: Error &le; &plusmn;5% of set speed on clear road
          </div>
          <div className="mono" style={{ fontSize: '1.1rem', fontWeight: 800, marginTop: '6px', color: '#06b6d4' }}>
            {testA && testA.value !== null && !isNaN(testA.value) ? `${testA.value.toFixed(2)}%` : '—'}
          </div>
        </div>

        {/* Test B */}
        <div style={{ background: '#0f172a', border: '1px solid #1e293b', borderRadius: '8px', padding: '0.75rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 700, fontSize: '0.85rem' }}>TEST B — Gap Hold (R2)</span>
            {getStatusBadge(testB ? testB.ok : null)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8', marginTop: '4px' }}>
            Requirement: 50 cm &plusmn; 8 cm (0.08 m) behind lead
          </div>
          <div className="mono" style={{ fontSize: '1.1rem', fontWeight: 800, marginTop: '6px', color: '#10b981' }}>
            {testB && testB.value !== null && !isNaN(testB.value) ? `${(testB.value * 100).toFixed(1)} cm error` : '—'}
          </div>
        </div>

        {/* Test C */}
        <div style={{ background: '#0f172a', border: '1px solid #1e293b', borderRadius: '8px', padding: '0.75rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 700, fontSize: '0.85rem' }}>TEST C — Stop & Go (R3)</span>
            {getStatusBadge(testC ? testC.ok : null)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8', marginTop: '4px' }}>
            Requirement: Stop &ge; 20 cm (0.20 m) behind lead & resume
          </div>
          <div className="mono" style={{ fontSize: '1.1rem', fontWeight: 800, marginTop: '6px', color: '#f59e0b' }}>
            {testC && testC.value !== null && !isNaN(testC.value) ? `${(testC.value * 100).toFixed(1)} cm stop gap` : '—'}
          </div>
        </div>

        {/* Test D */}
        <div style={{ background: '#0f172a', border: '1px solid #1e293b', borderRadius: '8px', padding: '0.75rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 700, fontSize: '0.85rem' }}>TEST D — Mode Switch Jump (R4)</span>
            {getStatusBadge(testD ? testD.ok : null)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8', marginTop: '4px' }}>
            Requirement: Target speed jump &le; 10% of set speed
          </div>
          <div className="mono" style={{ fontSize: '1.1rem', fontWeight: 800, marginTop: '6px', color: '#3b82f6' }}>
            {testD && testD.value !== null && !isNaN(testD.value) ? `${testD.value.toFixed(1)}% jump` : '—'}
          </div>
        </div>

        {/* Test E */}
        <div style={{ background: '#0f172a', border: '1px solid #1e293b', borderRadius: '8px', padding: '0.75rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 700, fontSize: '0.85rem' }}>TEST E — 10 Cut-In Safety (R5)</span>
            {getStatusBadge(testE ? testE.ok : null)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8', marginTop: '4px' }}>
            Requirement: Never closer than 20 cm (0.20 m) in 10 trials
          </div>
          <div className="mono" style={{ fontSize: '1.1rem', fontWeight: 800, marginTop: '6px', color: '#10b981' }}>
            {trialStats ? `Smallest: ${(trialStats.minGap * 100).toFixed(1)} cm (${trialStats.collisions} collisions)` : '—'}
          </div>
        </div>

        {/* Test F */}
        <div style={{ background: '#0f172a', border: '1px solid #1e293b', borderRadius: '8px', padding: '0.75rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 700, fontSize: '0.85rem' }}>TEST F — Sensor Fault (R7)</span>
            {getStatusBadge(sensorFailResult ? sensorFailResult.ok : null)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8', marginTop: '4px' }}>
            Requirement: Shutdown motor command &le; 300 ms upon fault
          </div>
          <div className="mono" style={{ fontSize: '1.1rem', fontWeight: 800, marginTop: '6px', color: '#06b6d4' }}>
            {sensorFailResult ? `${sensorFailResult.responseTimeMs} ms response` : '—'}
          </div>
        </div>
      </div>
    </div>
  );
}
