import React from 'react';
import { Play, Pause, RotateCcw, FastForward, ShieldAlert, Cpu, Download, FlaskConical, Zap } from 'lucide-react';

export const SCENARIO_MAP = [
  { label: 'Stop & Go (Default)', id: 'stop_and_go' },
  { label: 'Constant Speed', id: 'constant' },
  { label: 'Speed Hold (Clear Road)', id: 'none' },
  { label: 'Gap Hold', id: 'slow_down' },
  { label: 'Sudden Stop', id: 'stop' },
  { label: 'Cut-In', id: 'cut_in' },
  { label: 'Sensor Failure', id: 'sensor_failure' },
];

export default function ExperimentControls({
  currentScenario,
  onSelectScenario,
  isPlaying,
  onTogglePlay,
  onReset,
  simSpeed,
  onChangeSimSpeed,
  onFailSensor,
  onRunTrials,
  onRunAiBenchmark,
  onExportCsv,
  useAi,
  onToggleAi
}) {
  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title" style={{ color: '#06b6d4' }}>
          <FlaskConical size={16} /> SCENARIO CONTROLS & TEST INJECTION
        </span>
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <span style={{ fontSize: '0.75rem', color: '#94a3b8' }}>SPEED:</span>
          {[0.25, 0.5, 1, 2, 5, 10].map(speed => (
            <button
              key={speed}
              className={`btn btn-sm ${simSpeed === speed ? 'btn-primary' : 'btn-secondary'}`}
              onClick={() => onChangeSimSpeed(speed)}
              style={{ padding: '2px 6px', fontSize: '0.7rem' }}
            >
              {speed}x
            </button>
          ))}
        </div>
      </div>

      <div style={{ display: 'flex', flexWrap: 'wrap', gap: '1rem', alignItems: 'center', justifyContent: 'space-between' }}>
        {/* Playback Controls */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <button className={`btn ${isPlaying ? 'btn-secondary' : 'btn-primary'}`} onClick={onTogglePlay}>
            {isPlaying ? <Pause size={16} /> : <Play size={16} />}
            {isPlaying ? 'PAUSE' : 'PLAY SIM'}
          </button>
          <button className="btn btn-secondary" onClick={onReset}>
            <RotateCcw size={16} /> RESET
          </button>
        </div>

        {/* Scenario Selection Dropdown */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <span style={{ fontSize: '0.8rem', fontWeight: 700, color: '#94a3b8' }}>SELECT SCENARIO:</span>
          <select
            value={currentScenario}
            onChange={(e) => onSelectScenario(e.target.value)}
            style={{
              background: '#0f172a',
              color: '#06b6d4',
              border: '1px solid #06b6d4',
              borderRadius: '6px',
              padding: '0.45rem 0.85rem',
              fontSize: '0.85rem',
              fontWeight: 700,
              fontFamily: 'JetBrains Mono'
            }}
          >
            {SCENARIO_MAP.map(item => (
              <option key={item.id} value={item.id}>{item.label}</option>
            ))}
          </select>
        </div>

        {/* Automated Benchmark & Test Injection Buttons */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', flexWrap: 'wrap' }}>
          <button className="btn btn-danger btn-sm" onClick={onFailSensor}>
            <ShieldAlert size={14} /> INJECT SENSOR FAILURE
          </button>

          <button className="btn btn-secondary btn-sm" onClick={() => onRunTrials(50)}>
            <FastForward size={14} /> RUN 50 TRIALS
          </button>

          <button className={`btn btn-sm ${useAi ? 'btn-primary' : 'btn-secondary'}`} onClick={onToggleAi}>
            <Cpu size={14} /> AI PREDICTOR: {useAi ? 'ON' : 'OFF'}
          </button>

          <button className="btn btn-secondary btn-sm" onClick={onRunAiBenchmark}>
            <Zap size={14} /> AI BENCHMARK (70 RUNS)
          </button>

          <button className="btn btn-secondary btn-sm" onClick={onExportCsv}>
            <Download size={14} /> EXPORT CSV
          </button>
        </div>
      </div>
    </div>
  );
}
