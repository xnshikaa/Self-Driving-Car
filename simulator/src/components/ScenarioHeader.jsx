import React from 'react';
import { Target, Info, CheckCircle2 } from 'lucide-react';

export const SCENARIO_DETAILS = {
  stop_and_go: {
    title: 'Stop & Go Scenario (Default)',
    testing: 'Validates lead vehicle stopping, complete stationary gap holding, and automatic resumption when lead vehicle moves.',
    criteria: 'Requirement R3: Must stop at least 20 cm behind lead and automatically resume when lead vehicle accelerates above 0.10 m/s.',
    difficulty: 'Standard Operating Test'
  },
  constant: {
    title: 'Constant Speed Tracking Scenario',
    testing: 'Validates steady-state 50 cm distance holding behind a lead vehicle driving at a constant 0.30 m/s.',
    criteria: 'Requirement R2: Must maintain 50 cm ± 8 cm (0.42 m - 0.58 m) gap with steady-state error ≤ 8 cm.',
    difficulty: 'Baseline Tracking Test'
  },
  none: {
    title: 'Speed Hold Scenario (Clear Road)',
    testing: 'Validates set-speed regulation when no lead vehicle is nearby.',
    criteria: 'Requirement R1: Speed must remain within ±5% of the driver-set speed (0.50 m/s ± 0.025 m/s) on a clear straight run.',
    difficulty: 'Inner Speed Loop Test'
  },
  slow_down: {
    title: 'Gap Hold / Slowing Lead Scenario',
    testing: 'Validates controller response when lead vehicle decelerates from 0.42 m/s down to 0.18 m/s.',
    criteria: 'Requirement R2 & R4: Outer controller must reduce target speed smoothly without overshoot or mode-switch jump > 10%.',
    difficulty: 'Deceleration Tracking Test'
  },
  stop: {
    title: 'Sudden Stop Scenario',
    testing: 'Validates controlled stopping when lead vehicle rapidly decelerates to a complete stop.',
    criteria: 'Requirement R3 & R5: Vehicle must stop safely ≥ 20 cm behind lead without emergency collision.',
    difficulty: 'Braking Response Test'
  },
  cut_in: {
    title: 'Cut-In Safety Scenario',
    testing: 'Validates safety response when a lead vehicle suddenly appears close in front (0.45 m initial gap).',
    criteria: 'Requirement R5: Gap shall NEVER fall below 20 cm in 10 cut-in trials.',
    difficulty: 'Safety System Test'
  },
  sensor_failure: {
    title: 'Sensor Failure Injection Scenario',
    testing: 'Validates fault layer behavior when ultrasonic sensor pings stop returning valid data.',
    criteria: 'Requirement R7: Motor command must equal zero within ≤ 300 ms of sensor fault detection.',
    difficulty: 'Fault Mode Safety Test'
  }
};

export default function ScenarioHeader({ scenarioKey }) {
  const info = SCENARIO_DETAILS[scenarioKey] || SCENARIO_DETAILS.stop_and_go;

  return (
    <div className="card" style={{ background: 'linear-gradient(135deg, #0f172a 0%, #1e293b 100%)', border: '1px solid #06b6d4' }}>
      <div className="card-header">
        <span className="card-title" style={{ color: '#06b6d4', fontSize: '0.95rem' }}>
          <Target size={18} /> WHAT ARE WE TESTING? — {info.title.toUpperCase()}
        </span>
        <span className="badge badge-info">{info.difficulty}</span>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1rem' }}>
        <div>
          <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#94a3b8', textTransform: 'uppercase' }}>SCENARIO OBJECTIVE</div>
          <div style={{ fontSize: '0.85rem', color: '#f8fafc', marginTop: '4px', lineHeight: '1.4' }}>
            {info.testing}
          </div>
        </div>

        <div>
          <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#94a3b8', textTransform: 'uppercase' }}>ACCEPTANCE CRITERIA</div>
          <div style={{ fontSize: '0.85rem', color: '#10b981', marginTop: '4px', lineHeight: '1.4', fontWeight: 600 }}>
            {info.criteria}
          </div>
        </div>
      </div>
    </div>
  );
}
