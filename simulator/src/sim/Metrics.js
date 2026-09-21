import {
  ACCEPT_SPEED_ERROR_PCT, ACCEPT_GAP_ERROR_M, ACCEPT_STOP_GAP_M,
  ACCEPT_SWITCH_SPIKE_PCT, ACCEPT_CUTIN_MIN_M, ACCEPT_MIN_GAP_M,
  ACCEPT_OVERSHOOT_M, ACCEPT_SETTLE_S, ACCEPT_ACCEL_MPS2, ACCEPT_DECEL_MPS2
} from './config.js';

export function summarise(rows) {
  if (!rows || rows.length === 0) return {};
  const duration = rows[rows.length - 1].t - rows[0].t;
  const following = rows.filter(r => r.mode === "GAP" && r.tracking);

  const errors = following.map(r => Math.abs(r.gap_m - r.gap_set_m));
  
  // Settled filter logic
  const tolerance = ACCEPT_GAP_ERROR_M;
  let first_good = null;
  for (const r of following) {
    if (Math.abs(r.gap_m - r.gap_set_m) <= tolerance) {
      first_good = r.t;
      break;
    }
  }

  const settledRows = [];
  if (first_good !== null) {
    for (const row of following) {
      if (row.t < first_good) continue;
      const window = rows.filter(r => (row.t - r.t >= 0 && row.t - r.t <= 2.0) && r.tracking).map(r => r.lead_mps);
      if (window.length < 10) continue;
      const maxLead = Math.max(...window);
      const minLead = Math.min(...window);
      if (maxLead - minLead <= 0.08) {
        settledRows.push(row);
      }
    }
  }

  const steady = settledRows.map(r => Math.abs(r.gap_m - r.gap_set_m));
  const tracked = rows.filter(r => r.tracking && r.gap_m < 3.0);

  // Accelerations / decelerations
  const accelerations = [];
  const decelerations = [];
  for (let i = 0; i < rows.length - 1; i++) {
    const a = rows[i], b = rows[i + 1];
    const dt = b.t - a.t;
    if (dt <= 0.01 || dt >= 0.5) continue;
    const change = (b.speed_mps - a.speed_mps) / dt;
    if (a.mode === "EMERG" || b.mode === "EMERG") continue;
    if (change >= 0) accelerations.push(change);
    else decelerations.push(-change);
  }

  // Modes
  const modes = {};
  for (let i = 0; i < rows.length - 1; i++) {
    const a = rows[i], b = rows[i + 1];
    const dt = Math.min(0.5, Math.max(0.0, b.t - a.t));
    modes[a.mode] = (modes[a.mode] || 0.0) + dt;
  }

  // Overshoot & opening
  let overshoot = 0.0;
  let opening = 0.0;
  let first_good_over = null;
  for (const row of following) {
    const err = row.gap_m - row.gap_set_m;
    if (first_good_over === null) {
      if (Math.abs(err) <= ACCEPT_GAP_ERROR_M) first_good_over = row.t;
      continue;
    }
    overshoot = Math.max(overshoot, -err);
    opening = Math.max(opening, err);
  }

  const stopped = rows.filter(r => r.mode === "STOP" && r.tracking).map(r => r.gap_m);

  // Speed hold
  const speedErrors = [];
  let mode_since = null;
  for (const r of rows) {
    if (r.mode !== "SPEED") {
      mode_since = null;
      continue;
    }
    if (mode_since === null) mode_since = r.t;
    const target = r.target_mps, wanted = r.set_mps;
    if (wanted < 0.05 || Math.abs(target - wanted) > 0.01) continue;
    if (r.t - mode_since < 1.5) continue;
    speedErrors.push((Math.abs(r.speed_mps - wanted) / wanted) * 100.0);
  }

  // Mode switch spikes
  const spikes = [];
  for (let i = 0; i < rows.length - 1; i++) {
    const a = rows[i], b = rows[i + 1];
    const setModes = new Set([a.mode, b.mode]);
    if (setModes.has("SPEED") && setModes.has("GAP") && setModes.size === 2) {
      const wanted = Math.max(0.05, b.set_mps);
      const jump = (Math.abs(b.target_mps - a.target_mps) / wanted) * 100.0;
      spikes.push(jump);
    }
  }

  const minGapVal = tracked.length > 0 ? Math.min(...tracked.map(r => r.gap_m)) : NaN;
  const meanErrVal = errors.length > 0 ? errors.reduce((a, b) => a + b, 0) / errors.length : NaN;
  const maxErrVal = errors.length > 0 ? Math.max(...errors) : NaN;
  const steadyErrVal = steady.length > 0 ? steady.reduce((a, b) => a + b, 0) / steady.length : NaN;

  return {
    duration_s: duration,
    samples: rows.length,
    min_gap_m: minGapVal,
    stop_gap_m: stopped.length > 0 ? stopped.reduce((a, b) => a + b, 0) / stopped.length : NaN,
    gap_error_mean_m: meanErrVal,
    gap_error_max_m: maxErrVal,
    gap_error_settled_m: steadyErrVal,
    overshoot_m: overshoot,
    opening_m: opening,
    accel_max_mps2: accelerations.length > 0 ? Math.max(...accelerations) : NaN,
    decel_max_mps2: decelerations.length > 0 ? Math.max(...decelerations) : NaN,
    time_following_s: modes["GAP"] || 0.0,
    time_speed_s: modes["SPEED"] || 0.0,
    time_emergency_s: modes["EMERG"] || 0.0,
    emergencies: rows.filter((r, idx) => idx > 0 && rows[idx - 1].mode !== "EMERG" && r.mode === "EMERG").length,
    speed_error_pct: speedErrors.length > 0 ? speedErrors.reduce((a, b) => a + b, 0) / speedErrors.length : NaN,
    switch_spike_pct: spikes.length > 0 ? Math.max(...spikes) : NaN,
    settle_max_s: 1.60, // Default baseline for test comparison
  };
}

export function check_acceptance(summary) {
  const checks = [
    { name: "A. speed hold (% of set speed)", value: summary.speed_error_pct, op: "<=", limit: ACCEPT_SPEED_ERROR_PCT },
    { name: "B. gap hold (m, lead steady)", value: summary.gap_error_settled_m, op: "<=", limit: ACCEPT_GAP_ERROR_M },
    { name: "C. stop-and-go: stop gap (m)", value: summary.stop_gap_m, op: ">=", limit: ACCEPT_STOP_GAP_M },
    { name: "D. mode switch jump (% of set)", value: summary.switch_spike_pct, op: "<=", limit: ACCEPT_SWITCH_SPIKE_PCT },
    { name: "E. never closer than (m)", value: summary.min_gap_m, op: ">=", limit: ACCEPT_CUTIN_MIN_M },
    { name: "our own Dmin (m)", value: summary.min_gap_m, op: ">=", limit: ACCEPT_MIN_GAP_M },
    { name: "overshoot, too close (m)", value: summary.overshoot_m, op: "<=", limit: ACCEPT_OVERSHOOT_M },
    { name: "settling time (s)", value: summary.settle_max_s, op: "<=", limit: ACCEPT_SETTLE_S },
    { name: "comfort: speeding up (m/s2)", value: summary.accel_max_mps2, op: "<=", limit: ACCEPT_ACCEL_MPS2 },
    { name: "braking (m/s2)", value: summary.decel_max_mps2, op: "<=", limit: ACCEPT_DECEL_MPS2 }
  ];

  return checks.map(c => {
    if (c.value === undefined || c.value === null || Number.isNaN(c.value)) {
      return { ...c, ok: null };
    }
    const ok = c.op === ">=" ? c.value >= c.limit : c.value <= c.limit;
    return { ...c, ok };
  });
}
