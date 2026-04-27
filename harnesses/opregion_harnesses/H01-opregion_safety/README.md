# H01-opregion_safety

Patch-driven moving correctness harness for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/opregion_missions/opregion_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/opregion_missions/opregion_motion).

This harness focuses on `BHV_OpRegionV24` as the behavior under test. The stem
mission keeps the vehicle motion simple and grades on behavior-owned safety
signals:

- whether the nominal transit stays inside the configured region
- whether a save-region exit posts recovery flags without causing a halt breach
- whether positive `save_dist` generated buffers allow or reject the expected
  route shapes
- whether a halt-region exit causes the expected mission failure
- whether `trigger_on_poly_entry` handles a mission that starts outside the
  halt envelope
- whether `trigger_entry_time` delays formal halt-envelope entry
- whether `trigger_exit_time` debounces a short halt-region excursion
- whether `reset=true` clears entry state before a later halt-envelope exit
- whether `max_time` causes the expected failure
- whether `max_depth` and `min_altitude` runtime breaches halt through
  behavior-owned errors
- whether invalid scalar and boolean configuration values put the helm into
  `MALCONFIG`
- whether runtime `OPREGION_CORE_POLY` shrink and expansion updates are accepted
  and enforced

## Current Matrix

- `inside_region_pass`
  Baseline case. The vehicle remains inside the save and halt envelopes and
  arrives cleanly.
- `save_recover_pass`
  The vehicle briefly leaves the save region, posts save/recovery flags, and
  returns without breaching the halt region.
- `save_dist_buffer_pass`
  The vehicle leaves a small core polygon but stays inside generated positive
  `save_dist` and `halt_dist` buffers. The case should pass with no save flag.
- `save_dist_buffer_fail`
  The vehicle uses the same positive `save_dist` setup, but drives beyond the
  generated save buffer while staying inside the generated halt buffer. The case
  is expected to grade `fail` due to save-region breach.
- `halt_breach_fail`
  The vehicle is driven outside a tight halt region. The case is expected to
  grade `fail`.
- `entry_gate_start_outside_pass`
  The configured halt polygon starts to the east of the vehicle, so the vehicle
  launches outside the halt envelope, enters it, and finishes safely. With
  `trigger_on_poly_entry=true`, the initial outside state should not fail.
- `entry_gate_disabled_fail`
  Same start-outside geometry, but with `trigger_on_poly_entry=false`. The case
  is expected to grade `fail` quickly.
- `trigger_exit_debounce_pass`
  The vehicle briefly leaves the halt region, but the configured
  `trigger_exit_time` is long enough that no halt breach should be declared.
  This case grades the debounce contract rather than late transit completion.
- `trigger_exit_strict_fail`
  Companion case using the same short excursion shape, but with the stock
  short `trigger_exit_time`. The case is expected to grade `fail`.
- `trigger_entry_delay_pass`
  The vehicle leaves the halt envelope before the long `trigger_entry_time`
  matures, so the later outside state should not count as a breach.
- `reset_before_exit_pass`
  The vehicle would normally fail on halt exit, but a vehicle-side
  `OPREGION_UPDATES=reset=true` is posted during the exit debounce window and
  clears the entry state before breach declaration. The case should pass with
  no halt breach.
- `max_time_fail`
  The behavior is patched with a short `max_time`. The case is expected to
  grade `fail`.
- `max_depth_breach_fail`
  Vehicle-side sensor input posts `NAV_DEPTH` beyond a configured `max_depth`.
  The case should grade `fail` through the OpRegion depth-breach error path.
- `min_altitude_breach_fail`
  Vehicle-side sensor input posts `NAV_ALTITUDE` below a configured
  `min_altitude`. The case should grade `fail` through the OpRegion
  altitude-breach error path.
- `bad_save_dist_fail`
  The behavior is configured with a negative `save_dist`. The case should
  grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_halt_dist_fail`
  The behavior is configured with a negative `halt_dist`. The case should
  grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_max_depth_fail`
  The behavior is configured with a negative `max_depth`. The case should
  grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_min_altitude_fail`
  The behavior is configured with a negative `min_altitude`. The case should
  grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_recover_spd_fail`
  The behavior is configured with zero `recover_spd`. The case should grade
  `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_trigger_on_poly_entry_fail`
  The behavior is configured with an invalid `trigger_on_poly_entry` boolean.
  The case should grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_trigger_entry_time_fail`
  The behavior is configured with a negative `trigger_entry_time`. The case
  should grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `bad_trigger_exit_time_fail`
  The behavior is configured with a negative `trigger_exit_time`. The case
  should grade `fail` when `pHelmIvP` reports `MALCONFIG`.
- `dynamic_region_expand_pass`
  The behavior starts with a narrow dynamic-region core, then receives an early
  vehicle-side `OPREGION_CORE_POLY` expansion before the vehicle reaches the
  narrow boundary. The case should pass without save or halt breach.
- `dynamic_region_update_pass`
  A timer-posted `OPREGION_CORE_POLY` update shrinks the operating region at
  runtime from the vehicle community. The vehicle should post save-region
  recovery flags and still finish without a halt breach.
- `dynamic_region_halt_fail`
  A timer-posted update shrinks the operating region far enough that the
  vehicle should breach the recomputed halt region. The case is expected to
  grade `fail`.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=save_recover_pass 10
./zlaunch.sh --case=trigger_exit_debounce_pass --gui 1
./zlaunch.sh --just_make 10
./zlaunch.sh --jobs=4 --port_base=31000 10
```

Results are appended to `results.txt` with the mission-owned `grade` and the
OpRegion safety flags used for grading. Most cases evaluate as soon as the
natural terminal event occurs: arrival for pass transits, save/halt/time flags
for expected failures. The exit-debounce, entry-delay, and reset cases retain a
short `TEST_EVAL_READY` timer because they verify that a breach does not happen
after a configured timing window.

Wave mode runs the full matrix in isolated temporary mission copies. Each case
gets its own MOOSDB and pShare port block using
`case_base = port_base + case_idx*PORT_STRIDE`, with pShare ports starting at
`case_base + 10`. The default is serial
`--jobs=1`; use `--keep_workdirs` when preserving the temporary case folders is
useful for debugging.

Latest validation:

- April 27, 2026
- just_make matrix: `25/25` target generations completed with `--jobs=4 --port_base=31000`
- wave matrix: `25/25` expected outcomes matched with `--jobs=4 --port_base=31000`
- warp: `10`
