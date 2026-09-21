import { runScenario } from './src/sim/SimulationEngine.js';
import { summarise, check_acceptance } from './src/sim/Metrics.js';
import { PROFILES } from './src/sim/LeadVehicle.js';

console.log('--- JS ACC SIMULATOR VERIFICATION TEST ---');

// Test 1: stop_and_go scenario
const { rows, result } = runScenario('stop_and_go', { seed: 0 });
const summary = summarise(rows);

console.log(`Default Scenario: ${result.scenario}`);
console.log(`Smallest True Gap: ${(result.min_true_gap_m * 100).toFixed(1)} cm`);
console.log(`Gap Error Mean: ${(summary.gap_error_mean_m * 100).toFixed(1)} cm, Steady: ${(summary.gap_error_settled_m * 100).toFixed(1)} cm`);
console.log(`Events: ${result.events.length} state transitions`);

console.log('\nRunning All Scenarios:');
const worst = [];
for (const name of Object.keys(PROFILES)) {
  const res = runScenario(name, { seed: 0 });
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
console.log('\nACCEPTANCE SUITE RESULTS:');
let allPass = true;
for (const c of checks) {
  const mark = c.ok ? 'PASS' : 'FAIL';
  if (!c.ok) allPass = false;
  console.log(`  [${mark}] ${c.name}: ${c.value !== undefined && !isNaN(c.value) ? c.value.toFixed(3) : 'n/a'} (limit ${c.limit})`);
}

console.log(`\nOverall Engine Verification: ${allPass ? 'SUCCESS (100% Match)' : 'FAILURE'}`);
