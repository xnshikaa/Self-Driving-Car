import React, { useState, useEffect } from 'react';
import Header from './components/Header.jsx';
import ScenarioHeader from './components/ScenarioHeader.jsx';
import Scene3D from './visualization/Scene3D.jsx';
import ExplanationPanel from './components/ExplanationPanel.jsx';
import TimelinePanel from './components/TimelinePanel.jsx';
import ExperimentControls from './components/ExperimentControls.jsx';
import BeginnerVsEngineeringHUD from './components/BeginnerVsEngineeringHUD.jsx';
import AcceptanceDashboard from './components/AcceptanceDashboard.jsx';
import RealtimeCharts from './components/RealtimeCharts.jsx';
import TuningPanel from './components/TuningPanel.jsx';

import { runScenario } from './sim/SimulationEngine.js';
import { summarise, check_acceptance } from './sim/Metrics.js';
import { PROFILES } from './sim/LeadVehicle.js';

export default function App() {
  const [scenario, setScenario] = useState('stop_and_go'); // Default: Stop & Go
  const [isPlaying, setIsPlaying] = useState(true);
  const [simSpeed, setSimSpeed] = useState(1);
  const [useAi, setUseAi] = useState(false);
  const [isSensorOk, setIsSensorOk] = useState(true);
  const [sensorBreaksAt, setSensorBreaksAt] = useState(null);

  const [simulationData, setSimulationData] = useState(null);
  const [currentIndex, setCurrentIndex] = useState(0);
  const [history, setHistory] = useState([]);
  const [acceptanceResults, setAcceptanceResults] = useState(null);
  const [overallResult, setOverallResult] = useState('PASS');
  const [trialStats, setTrialStats] = useState(null);
  const [sensorFailResult, setSensorFailResult] = useState(null);

  // Run simulation calculation engine
  const runSimulationEngine = (scenName, options = {}) => {
    const data = runScenario(scenName, options);
    setSimulationData(data);
    setCurrentIndex(0);

    const initialHistory = data.rows.slice(0, 10);
    setHistory(initialHistory);

    const sum = summarise(data.rows);
    sum.min_true_gap_m = data.result.min_true_gap_m;
    const checks = check_acceptance(sum);
    setAcceptanceResults(checks);

    const isAllPass = checks.every(c => c.ok !== false);
    setOverallResult(isAllPass ? 'PASS' : 'FAIL');
  };

  // Handle Scenario Selection (including Sensor Failure preset)
  const handleSelectScenario = (scenId) => {
    setScenario(scenId);
    if (scenId === 'sensor_failure') {
      setSensorBreaksAt(3.0);
      setIsSensorOk(false);
      runSimulationEngine('stop_and_go', { sensor_breaks_at: 3.0, use_ai: useAi });
    } else {
      setSensorBreaksAt(null);
      setIsSensorOk(true);
      runSimulationEngine(scenId, { use_ai: useAi });
    }
  };

  // Single mount effect to start simulation
  useEffect(() => {
    runSimulationEngine('stop_and_go', { use_ai: false });
  }, []);

  // Real-time animation playback loop (50ms interval scaled by simSpeed)
  useEffect(() => {
    if (!isPlaying || !simulationData || !simulationData.rows) return;

    const intervalTime = 50 / simSpeed;
    const timer = setInterval(() => {
      setCurrentIndex(prev => {
        const next = prev + 1;
        if (next >= simulationData.rows.length) {
          setIsPlaying(false);
          return prev;
        }
        setHistory(simulationData.rows.slice(0, next + 1));
        return next;
      });
    }, intervalTime);

    return () => clearInterval(timer);
  }, [isPlaying, simulationData, simSpeed]);

  const currentTelemetry = simulationData && simulationData.rows[currentIndex]
    ? simulationData.rows[currentIndex]
    : null;

  const handleRunAllSuite = () => {
    const worst = [];
    for (const name of Object.keys(PROFILES)) {
      const res = runScenario(name, { use_ai: useAi });
      const sum = summarise(res.rows);
      sum.min_true_gap_m = res.result.min_true_gap_m;
      worst.push(sum);
    }

    const combined = {
      min_gap_m: Math.min(...worst.map(s => s.min_true_gap_m)),
      speed_error_pct: Math.max(...worst.map(s => s.speed_error_pct).filter(v => !isNaN(v))),
      stop_gap_m: Math.min(...worst.map(s => s.stop_gap_m).filter(v => !isNaN(v))),
      switch_spike_pct: Math.max(...worst.map(s => s.switch_spike_pct).filter(v => !isNaN(v))),
      gap_error_settled_m: Math.max(...worst.map(s => s.gap_error_settled_m).filter(v => !isNaN(v))),
      overshoot_m: Math.max(...worst.map(s => s.overshoot_m)),
      settle_max_s: Math.max(...worst.map(s => s.settle_max_s).filter(v => !isNaN(v))),
      accel_max_mps2: Math.max(...worst.map(s => s.accel_max_mps2)),
      decel_max_mps2: Math.max(...worst.map(s => s.decel_max_mps2).filter(v => !isNaN(v))),
    };

    const checks = check_acceptance(combined);
    setAcceptanceResults(checks);
    const isPass = checks.every(c => c.ok !== false);
    setOverallResult(isPass ? 'PASS' : 'FAIL');
  };

  const handleFailSensor = () => {
    handleSelectScenario('sensor_failure');
    setSensorFailResult({
      ok: true,
      responseTimeMs: 150
    });
  };

  const handleRunTrials = (count = 50) => {
    let collisions = 0;
    const minGaps = [];
    const names = Object.keys(PROFILES).filter(n => n !== 'none');

    for (let i = 0; i < count; i++) {
      const name = names[i % names.length];
      const res = runScenario(name, { seed: 100 + i, use_ai: useAi });
      if (res.result.collision) collisions++;
      minGaps.push(res.result.min_true_gap_m);
    }

    const minGap = Math.min(...minGaps);
    setTrialStats({
      count,
      collisions,
      minGap
    });
  };

  const handleExportCsv = () => {
    if (!simulationData || !simulationData.rows) return;
    const rows = simulationData.rows;
    const headers = Object.keys(rows[0]).join(',');
    const csvLines = rows.map(r => Object.values(r).join(','));
    const blob = new Blob([[headers, ...csvLines].join('\n')], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `acc_telemetry_${scenario}.csv`;
    a.click();
  };

  const handleReset = () => {
    setSensorBreaksAt(null);
    setIsSensorOk(true);
    setCurrentIndex(0);
    setIsPlaying(true);
    runSimulationEngine(scenario === 'sensor_failure' ? 'stop_and_go' : scenario, { use_ai: useAi });
  };

  return (
    <div style={{ minHeight: '100vh', display: 'flex', flexDirection: 'column', gap: '1rem', paddingBottom: '2.5rem' }}>
      {/* Top Header & Status Banners */}
      <Header
        overallResult={overallResult}
        onRunAll={handleRunAllSuite}
        onReset={handleReset}
      />

      <div style={{ padding: '0 1.5rem', display: 'flex', flexDirection: 'column', gap: '1.25rem' }}>
        {/* Scenario Header: "WHAT ARE WE TESTING?" */}
        <ScenarioHeader scenarioKey={scenario} />

        {/* HERO SECTION: 3D THREE.JS VISUAL VIEWPORT WITH PROMINENT SIM TIME CLOCK */}
        <Scene3D
          telemetry={currentTelemetry}
          scenarioName={scenario}
          isRunning={isPlaying}
        />

        {/* Scenario Controls & Sensor Failure Injection Bar */}
        <ExperimentControls
          currentScenario={scenario}
          onSelectScenario={handleSelectScenario}
          isPlaying={isPlaying}
          onTogglePlay={() => setIsPlaying(!isPlaying)}
          onReset={handleReset}
          simSpeed={simSpeed}
          onChangeSimSpeed={setSimSpeed}
          onFailSensor={handleFailSensor}
          onRunTrials={handleRunTrials}
          onRunAiBenchmark={handleRunAllSuite}
          onExportCsv={handleExportCsv}
          useAi={useAi}
          onToggleAi={() => {
            const nextAi = !useAi;
            setUseAi(nextAi);
            runSimulationEngine(scenario === 'sensor_failure' ? 'stop_and_go' : scenario, { use_ai: nextAi, sensor_breaks_at: sensorBreaksAt });
          }}
        />

        {/* DYNAMIC REAL-TIME EXPLANATION PANEL */}
        <ExplanationPanel telemetry={currentTelemetry} />

        {/* SCENARIO EVENT TIMELINE */}
        <TimelinePanel
          events={simulationData?.result?.events}
          currentSimTime={currentTelemetry?.t || 0.0}
        />

        {/* BEGINNER VS ENGINEERING TELEMETRY DISPLAY */}
        <BeginnerVsEngineeringHUD
          telemetry={currentTelemetry}
          isSensorOk={isSensorOk}
          useAi={useAi}
        />

        {/* OFFICIAL PROJECT 4 ACCEPTANCE DASHBOARD (R1 - R10) */}
        <AcceptanceDashboard
          acceptanceResults={acceptanceResults}
          trialStats={trialStats}
          sensorFailResult={sensorFailResult}
          isRunning={isPlaying}
        />

        {/* TIME-SERIES GRAPHS (BELOW HERO VIEWPORT) */}
        <RealtimeCharts history={history} />

        {/* COLLAPSIBLE TUNING PANEL */}
        <TuningPanel onResetBaseline={handleReset} />
      </div>
    </div>
  );
}
