# H01-opregion_safety

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven moving correctness harness for
[`missions/opregion_missions/opregion_motion`](../../../missions/opregion_missions/opregion_motion).

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

- `inside_region_pass` Sends the vehicle from `(-18,-121)` through `(42,-121)` and back to `(12,-121)` inside the core box `[-23,47]×[-152,-90]`, testing nominal containment; passes on arrival with no save-, halt-, or time-breach flag and no `BHV_ERROR` mail.
- `save_recover_pass` Narrows the core's east edge to `x=27`, keeps the halt edge at `x=49`, and routes through `x=34` before returning to `x=10`, testing save-region recovery without a halt; passes on arrival with `SAVE_REGION_BREACHED=true`, no halt/time breach, and no behavior error, while `SAVE_REGION_RECOVERING` is only reported.
- `save_dist_buffer_pass` Uses core east edge `x=32`, `save_dist=16`, and `halt_dist=24` on the stock route to `x=42`, testing that the generated save buffer contains a point outside the core; passes on arrival with all breach flags false and no behavior error.
- `save_dist_buffer_fail` Uses the same core with `save_dist=16`, enlarges `halt_dist` to 36, and routes through `x=58`, testing a save-buffer exit that remains inside the halt buffer; the harness passes when `SAVE_REGION_BREACHED=true` while halt/time breach and behavior error remain false.
- `halt_breach_fail` Uses a core ending at `x=10` with `halt_dist=4`, placing the eastbound route outside the halt envelope; the harness passes when `HALT_REGION_BREACHED=true`, `TIME_LIMIT_BREACHED=false`, and any `BHV_ERROR` mail has appeared.
- `entry_gate_start_outside_pass` Places the core's west edge at `x=-10` with `halt_dist=5`, so the vehicle starts three meters outside the halt envelope, and sets `trigger_on_poly_entry=true`; passes after the vehicle enters and completes the route with no save-, halt-, or time-breach flag and no behavior error.
- `entry_gate_disabled_fail` Uses the same start-outside geometry with `trigger_on_poly_entry=false`, testing immediate enforcement before polygon entry; the harness passes when `HALT_REGION_BREACHED=true`, time breach remains false, and any behavior error has appeared.
- `trigger_exit_debounce_pass` Routes to `x=58` and back across a halt edge at `x=50` with `trigger_exit_time=80`, exercising exit debounce during the short excursion; passes at 44 seconds when no halt/time breach or behavior error is present, without requiring arrival.
- `trigger_exit_strict_fail` Uses the same out-and-back route and halt geometry with `trigger_exit_time=0.5`, testing strict exit enforcement; the harness passes when the excursion produces `HALT_REGION_BREACHED=true` and a behavior error without a time breach.
- `trigger_entry_delay_pass` Uses the same route with `trigger_entry_time=80` and `trigger_exit_time=0.5`, exercising delayed recognition of initial polygon entry; passes at 44 seconds when no halt/time breach or behavior error is present, without requiring arrival.
- `reset_before_exit_pass` Uses the same route with `trigger_exit_time=20` and posts `OPREGION_UPDATES="reset=true"` at 34 seconds, exercising runtime entry-state reset during the exit window; passes at 44 seconds when no halt/time breach or behavior error is present, without directly grading update acceptance.
- `max_time_fail` Sets `max_time=8` while keeping the region large enough for the route, testing the behavior's mission timer; the harness passes when `TIME_LIMIT_BREACHED=true` and a behavior error appear while save and halt breach remain false.
- `max_depth_breach_fail` Sets `max_depth=1` and posts `NAV_DEPTH=5` once per second from times two through six, exercising the maximum-depth safety path; the harness passes when any `BHV_ERROR` mail appears while save-, halt-, and time-breach flags remain false.
- `min_altitude_breach_fail` Sets `min_altitude=10` and posts `NAV_ALTITUDE=2` once per second from times two through six, exercising the minimum-altitude safety path; the harness passes when any `BHV_ERROR` mail appears while save-, halt-, and time-breach flags remain false.
- `bad_save_dist_fail` Sets `save_dist=-1`, exercising invalid buffer configuration; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_halt_dist_fail` Sets `halt_dist=-1`, exercising invalid buffer configuration; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_max_depth_fail` Sets `max_depth=-1`, exercising invalid depth-limit configuration; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_min_altitude_fail` Sets `min_altitude=-1`, exercising invalid altitude-limit configuration; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_recover_spd_fail` Sets `recover_spd=0`, exercising rejection of a nonpositive recovery speed; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_trigger_on_poly_entry_fail` Sets `trigger_on_poly_entry=maybe`, exercising rejection of a non-boolean entry gate; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_trigger_entry_time_fail` Sets `trigger_entry_time=-1`, exercising rejection of a negative entry delay; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_trigger_exit_time_fail` Sets `trigger_exit_time=-1`, exercising rejection of a negative exit delay; the harness passes when any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `dynamic_region_expand_pass` Starts with a dynamic core ending at `x=21`, then posts an expanded core ending at `x=60` after three seconds, testing expansion before the vehicle reaches the narrow save boundary; passes on arrival with all breach flags false and no behavior error.
- `dynamic_region_update_pass` Starts with a core ending at `x=47`, posts a shrink to `x=21` after six seconds, and routes through `x=34` before returning to `x=4`, testing runtime enforcement of a new save boundary; passes on arrival with `SAVE_REGION_BREACHED=true`, no halt/time breach, and no behavior error.
- `dynamic_region_halt_fail` Posts a shrink from east edge `x=47` to `x=5` after six seconds while retaining the stock eight-meter halt distance, testing immediate enforcement of a recomputed halt boundary; the harness passes when `HALT_REGION_BREACHED=true` and a behavior error appear without a time breach.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=save_recover_pass 10
./zlaunch.sh --case=trigger_exit_debounce_pass --gui 1
./zlaunch.sh --just_make 10
./zlaunch.sh --jobs=4 --port_base=31000 10
./zlaunch.sh --jobs=4 --port_base=35600 --port_stride=20 10
```

Results are appended to `results.txt` with `case=<case_name>` followed by the
mission-owned `grade` and the OpRegion safety flags used for grading. Expected
negative cases grade `pass` when their breach, behavior-error, or malconfig
evidence is observed. The exit-debounce, entry-delay, and reset cases retain a
short `TEST_EVAL_READY` timer because they verify that a breach does not happen
after a configured timing window.

Wave mode runs the full matrix in isolated temporary mission copies. Each case
gets its own MOOSDB and pShare port block using
`case_base = port_base + case_idx*PORT_STRIDE`, with pShare ports starting at
`case_base + 10`. The default is serial `--jobs=1` and `PORT_STRIDE=30`; use
`--port_stride=20` with the 25-case matrix to keep validation inside a 500-port
band. Use `--keep_workdirs` when preserving the temporary case folders is useful
for debugging.

Latest validation:

- May 21, 2026
- syntax: `bash -n harnesses/opregion_harnesses/H01-opregion_safety/zlaunch.sh`
- just_make matrix: `25/25` target generations completed with
  `--just_make --jobs=4 --port_base=35600 --port_stride=20 10`
- wave matrix: `25/25` mission-owned grades passed with
  `--jobs=4 --port_base=35600 --port_stride=20 10`
- warp: `10`
