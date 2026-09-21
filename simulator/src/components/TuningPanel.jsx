import React, { useState } from 'react';
import { Sliders, RotateCcw, ChevronDown, ChevronUp } from 'lucide-react';
import * as CONSTANTS from '../sim/constants.js';

export default function TuningPanel({ onResetBaseline }) {
  const [isOpen, setIsOpen] = useState(false);
  const [params, setParams] = useState({
    speedKp: CONSTANTS.SPEED_KP,
    speedKi: CONSTANTS.SPEED_KI,
    gapKp: CONSTANTS.GAP_KP,
    gapKi: CONSTANTS.GAP_KI,
    dmin: CONSTANTS.D_MIN_M,
    sensorNoise: CONSTANTS.US_NOISE_M,
    dropout: CONSTANTS.US_DROPOUT
  });

  const handleChange = (key, val) => {
    setParams(prev => ({ ...prev, [key]: parseFloat(val) }));
  };

  const handleReset = () => {
    setParams({
      speedKp: CONSTANTS.SPEED_KP,
      speedKi: CONSTANTS.SPEED_KI,
      gapKp: CONSTANTS.GAP_KP,
      gapKi: CONSTANTS.GAP_KI,
      dmin: CONSTANTS.D_MIN_M,
      sensorNoise: CONSTANTS.US_NOISE_M,
      dropout: CONSTANTS.US_DROPOUT
    });
    if (onResetBaseline) onResetBaseline();
  };

  return (
    <div className="card">
      <div
        className="card-header"
        style={{ cursor: 'pointer', marginBottom: isOpen ? '0.75rem' : '0', borderBottom: isOpen ? '1px solid rgba(255,255,255,0.05)' : 'none' }}
        onClick={() => setIsOpen(!isOpen)}
      >
        <span className="card-title" style={{ color: '#8b5cf6' }}>
          <Sliders size={16} /> CONTROLLER & SENSOR TUNING PARAMETERS (ADVANCED)
        </span>
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <span style={{ fontSize: '0.75rem', color: '#94a3b8' }}>
            {isOpen ? 'CLICK TO COLLAPSE' : 'CLICK TO EXPAND'}
          </span>
          {isOpen ? <ChevronUp size={16} /> : <ChevronDown size={16} />}
        </div>
      </div>

      {isOpen && (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', gap: '1rem', marginTop: '0.5rem' }}>
          {/* Inner Speed Loop Gains */}
          <div>
            <div style={{ fontSize: '0.8rem', fontWeight: 700, color: '#3b82f6', marginBottom: '0.5rem' }}>INNER SPEED PID (100 Hz)</div>
            <div style={{ marginBottom: '0.5rem' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem' }}>
                <span>SPEED KP:</span>
                <span className="mono">{params.speedKp}</span>
              </div>
              <input type="range" min="100" max="1200" step="10" value={params.speedKp} onChange={e => handleChange('speedKp', e.target.value)} style={{ width: '100%' }} />
            </div>
            <div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem' }}>
                <span>SPEED KI:</span>
                <span className="mono">{params.speedKi}</span>
              </div>
              <input type="range" min="500" max="6000" step="50" value={params.speedKi} onChange={e => handleChange('speedKi', e.target.value)} style={{ width: '100%' }} />
            </div>
          </div>

          {/* Outer Gap Loop Gains */}
          <div>
            <div style={{ fontSize: '0.8rem', fontWeight: 700, color: '#10b981', marginBottom: '0.5rem' }}>OUTER GAP PID (50 Hz)</div>
            <div style={{ marginBottom: '0.5rem' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem' }}>
                <span>GAP KP:</span>
                <span className="mono">{params.gapKp}</span>
              </div>
              <input type="range" min="0.1" max="2.0" step="0.05" value={params.gapKp} onChange={e => handleChange('gapKp', e.target.value)} style={{ width: '100%' }} />
            </div>
            <div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem' }}>
                <span>GAP KI:</span>
                <span className="mono">{params.gapKi}</span>
              </div>
              <input type="range" min="0.01" max="0.5" step="0.01" value={params.gapKi} onChange={e => handleChange('gapKi', e.target.value)} style={{ width: '100%' }} />
            </div>
          </div>

          {/* Sensor Noise & Dropout */}
          <div>
            <div style={{ fontSize: '0.8rem', fontWeight: 700, color: '#06b6d4', marginBottom: '0.5rem' }}>ULTRASONIC SENSOR NOISE</div>
            <div style={{ marginBottom: '0.5rem' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem' }}>
                <span>NOISE STD DEV (m):</span>
                <span className="mono">{params.sensorNoise}</span>
              </div>
              <input type="range" min="0.0" max="0.08" step="0.005" value={params.sensorNoise} onChange={e => handleChange('sensorNoise', e.target.value)} style={{ width: '100%' }} />
            </div>
            <div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem' }}>
                <span>PING DROPOUT PROBABILITY:</span>
                <span className="mono">{(params.dropout * 100).toFixed(1)}%</span>
              </div>
              <input type="range" min="0.0" max="0.15" step="0.005" value={params.dropout} onChange={e => handleChange('dropout', e.target.value)} style={{ width: '100%' }} />
            </div>
          </div>

          <div style={{ display: 'flex', alignItems: 'flex-end', justifyContent: 'flex-end' }}>
            <button className="btn btn-secondary btn-sm" onClick={handleReset}>
              <RotateCcw size={14} /> RESET TO GUIDE BASELINE
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
