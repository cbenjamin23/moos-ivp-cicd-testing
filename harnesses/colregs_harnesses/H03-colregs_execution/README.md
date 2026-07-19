# H03-colregs_execution

TLDR:
- execution-quality harness on the shared `colregs_unit` stem
- checks realized CPA band, relative side outcome, and loose completion time per canonical case
- sits after `H01/H02`: mode selection is already assumed correct and now we check maneuver quality
- current implemented cases: 23 default execution cases covering most H01 canonicals plus the better-fit spacing siblings
- migration signoff: three rolling 23/23 matrices and one serial 23/23 matrix passed on July 16, 2026

Execution-quality harness built on the shared `colregs_unit` stem mission.

Primary intent:
- cover the H01 canonicals that remain defensible as execution tests on the shared stem
- validate that Ben completes the encounter cleanly
- record realized closest range and final relative-side state
- keep one shoreside patch per launch, with the verdict owned by `pMissionEval` as far as the app actually supports it

## How This Harness Works

- reuse the same canonical geometry from `H01`
- let the mission run through completion instead of grading at first
  classification
- launch each case with its own `*-shoreside.xmoos` patch
- let `pMissionEval` own pass/fail expectations for each case:
  - CPA band
  - relative side outcome (`cn_port`, selective `cn_fore`, `cn_crossed=0`)
  - final `colregs_mode` where the case identity should persist through completion
- keep only loose completion time in the shell harness, because `WALL_TIME` is available to `pMissionEval` only as a report macro and not as a logic-test variable

The migrated launcher requires Bash 5.1 or newer, uses isolated mission copies
in serial and rolling modes, refills open job slots immediately, assigns a
distinct three-MOOSDB/three-pShare port block to every case, aggregates rows
in selected-case order, and performs root-scoped cleanup. It does not alter
any case geometry, stimulus, evaluator condition, or timing ceiling.

## Cases

- `head_on_execution_pass`: Runs the centerline head-on geometry through Ben's arrival. Passes within 12 wall seconds when `16<=UCD_CLOSEST_RANGE_EVER<=20`, Ben finishes on ownship's port and aft sides (`cn_port=1`, `cn_fore=0`), no side crossing occurs, and timeout and collisions remain false/zero.
- `head_on_cpa_fallback_execution_pass`: Runs the heading-28 near-head-on CPA fallback through Ben's arrival. Passes within 12 wall seconds on the same 16-to-20-meter CPA, port/aft, no-crossing, no-timeout, and zero-collision envelope.
- `head_on_port_offset_execution_pass`: Runs the crossing 12-meter offset head-on geometry from ownship `(-6,-40)` and Ben `(6,-130)` through completion. Passes within 12 wall seconds on the 16-to-20-meter CPA, `cn_port=1`, `cn_fore=0`, `cn_crossed=0`, no-timeout, zero-collision envelope.
- `head_on_starboard_offset_execution_pass`: Mirrors the offset starts to ownship `(6,-40)` and Ben `(-6,-130)`. Passes within 12 wall seconds on the same 16-to-20-meter CPA and final port/aft/no-crossing envelope with no timeout or collision.
- `crossing_starboard_giveway_execution_pass`: Runs eastbound ownship from `(-70,-80)` against northbound Ben from `(10,-130)` through Ben's arrival. Passes within 12 wall seconds when closest range is 18–24 meters, Ben finishes port and aft (`1,0`), `cn_crossed=0`, and no timeout or collision occurs.
- `crossing_starboard_giveway_far_execution_pass`: Moves the same crossing starts outward to ownship `(-80,-80)` and Ben `(10,-150)`. Passes within 13 wall seconds when closest range is 17.7–24 meters with the same port/aft/no-crossing and no-timeout/no-collision outcome.
- `crossing_starboard_giveway_close_execution_pass`: Moves the starts inward to ownship `(-60,-80)` and Ben `(10,-115)`. Passes within 12 wall seconds when closest range is 18–24 meters with the same port/aft/no-crossing and no-timeout/no-collision outcome.
- `crossing_port_standon_execution_pass`: Runs eastbound ownship from `(-70,-80)` against southbound Ben from `(10,-20)` through arrival. Passes within 12 wall seconds when closest range is 14–20 meters, Ben finishes starboard and aft (`cn_port=0`, `cn_fore=0`), `cn_crossed=0`, and no timeout or collision occurs.
- `crossing_port_standon_unsure_bow_execution_pass`: Uses the same geometry as the preceding case, whose classification history includes `standon:unsure_bow`. Passes on the same 14–20-meter, starboard/aft/no-crossing completion envelope within 12 wall seconds with no timeout or collision; the transient mode is not graded here.
- `crossing_port_standon_stern_execution_pass`: Starts southbound Ben farther north at `(10,15)` and 1.2 m/s. Passes within 16 wall seconds when closest range is 18.9–21 meters, Ben finishes starboard and aft without crossing, and no timeout or collision occurs.
- `crossing_port_standon_far_execution_pass`: Uses the execution-tuned far fixture with ownship `(-76,-80)` and Ben `(10,-12)`. Passes within 13 wall seconds when closest range is 14–20 meters and the final relationship is starboard/aft with no crossing, timeout, or collision.
- `crossing_port_standon_close_execution_pass`: Uses the closer starts ownship `(-60,-80)` and Ben `(10,-30)`. Passes within 12 wall seconds on the 14–20-meter, starboard/aft/no-crossing envelope with no timeout or collision.
- `crossing_port_standon_close_unsure_bow_execution_pass`: Uses the same close geometry but identifies its earlier `standon:unsure_bow` transition. Passes on the same 14–20-meter, starboard/aft/no-crossing completion envelope within 12 wall seconds; the transient mode itself is not graded.
- `crossing_port_standon_unsure_execution_pass`: Uses ownship `(-55,-72)` and Ben `(10,-18)`, whose classification history includes `standon:unsure`. Passes within 12.1 wall seconds when closest range is 10–13 meters and Ben finishes starboard and aft without crossing, timeout, or collision; the transient mode is not graded.
- `overtaking_starboard_execution_pass`: Sends 1.6 m/s ownship from `(-90,-80)` past 1.0 m/s Ben at `(-35,-84)`. Passes within 12 wall seconds when closest range is 17–21 meters, Ben finishes starboard (`cn_port=0`), `cn_crossed=0`, and arrival occurs without timeout or collision; fore/aft state is not graded.
- `overtaking_starboard_range_far_execution_pass`: Moves ownship back to `(-105,-80)` while retaining Ben at `(-35,-84)`. Passes within 12 wall seconds when closest range is 20–24 meters and Ben finishes starboard and fore (`cn_port=0`, `cn_fore=1`) without crossing, timeout, or collision.
- `overtaking_starboard_small_gap_execution_pass`: Uses the long-range sibling, not the compact case: ownship starts `(-105,-80)` and Ben `(-35,-82)` with a two-meter gap. Passes within 12 wall seconds when closest range is 21.7–22.8 meters and Ben finishes starboard/fore without crossing, timeout, or collision.
- `overtaking_starboard_mirror_execution_pass`: Mirrors the compact tracks with ownship at `(-90,-84)` and Ben at `(-35,-80)`. Passes within 12 wall seconds when closest range is 17–21 meters and Ben finishes port/fore (`1,1`) without crossing, timeout, or collision.
- `overtaking_starboard_mirror_range_far_execution_pass`: Moves mirrored ownship back to `(-105,-84)`. Passes within 12 wall seconds when closest range is 20–24 meters and Ben finishes port/fore without crossing, timeout, or collision.
- `overtaking_starboard_mirror_small_gap_execution_pass`: Uses long-range ownship at `(-105,-81)` and Ben at `(-35,-80)`, a one-meter gap. Passes within 12 wall seconds when closest range is 21.8–22.8 meters and Ben finishes port/fore without crossing, timeout, or collision.
- `overtaking_starboard_mirror_large_gap_execution_pass`: Uses long-range ownship at `(-105,-88)` and Ben at `(-35,-80)`, an eight-meter gap. Passes within 12 wall seconds when closest range is 20.7–22.1 meters and Ben finishes port/fore without crossing, timeout, or collision.
- `overtaken_port_standon_midrange_execution_pass`: Sends 1.6 m/s Ben from `(-75,-76)` past 1.0 m/s ownship at `(-35,-80)`. Passes within 12 wall seconds when closest range is 17.7–18.4 meters, Ben finishes port/fore without crossing, final mode remains `standon_ot:port`, and no timeout or collision occurs.
- `overtaken_starboard_standon_midrange_execution_pass`: Sends 1.6 m/s Ben from `(-85,-85)` past 1.0 m/s ownship at `(-35,-80)`. Passes within 12 wall seconds when closest range is 18.15–18.30 meters, Ben finishes starboard/fore without crossing, final mode remains `standon_ot:starboard`, and no timeout or collision occurs.
- `overtaken_port_standon_execution_pass`: This explicitly selected compact comparison sends Ben from `(-70,-76)` past ownship at `(-35,-80)`. Passes within 12 wall seconds when closest range is 17–21 meters, Ben finishes port/fore without crossing, final mode remains `standon_ot:port`, and no timeout or collision occurs.

Current mission-side assertions:
- every H03 case patch still requires `ARRIVED_ben = true`, `COLLISION_TOTAL = 0`, and `MISSION_TIMEOUT = false`
- each case patch now also owns its own CPA/side/mode envelope inside `pMissionEval`
- all supported H03 cases require `cn_crossed=0`
- head_on family (`head_on_execution_pass`, `head_on_cpa_fallback_execution_pass`, `head_on_port_offset_execution_pass`, `head_on_starboard_offset_execution_pass`): `closest_range_ever` in `[16,20]`, `cn_port=1`, `cn_fore=0`
- crossing-starboard-giveway family (`crossing_starboard_giveway_execution_pass`, `crossing_starboard_giveway_close_execution_pass`): `closest_range_ever` in `[18,24]`, `cn_port=1`, `cn_fore=0`
- crossing-starboard-giveway far family (`crossing_starboard_giveway_far_execution_pass`): `closest_range_ever` in `[17.7,24]`, `cn_port=1`, `cn_fore=0`
- crossing-port-standon family (`crossing_port_standon_execution_pass`, `crossing_port_standon_unsure_bow_execution_pass`, `crossing_port_standon_far_execution_pass`, `crossing_port_standon_close_execution_pass`, `crossing_port_standon_close_unsure_bow_execution_pass`): `closest_range_ever` in `[14,20]`, `cn_port=0`, `cn_fore=0`
- crossing-port-standon stern family (`crossing_port_standon_stern_execution_pass`): `closest_range_ever` in `[18.9,21]`, `cn_port=0`, `cn_fore=0`
- crossing-port-standon unsure family (`crossing_port_standon_unsure_execution_pass`): `closest_range_ever` in `[10,13]`, `cn_port=0`, `cn_fore=0`
- overtaking-starboard family (`overtaking_starboard_execution_pass`): `closest_range_ever` in `[17,21]`, `cn_port=0`
- overtaking-starboard long-range family (`overtaking_starboard_range_far_execution_pass`): `closest_range_ever` in `[20,24]`, `cn_port=0`, `cn_fore=1`
- overtaking-starboard long-range spacing family (`overtaking_starboard_small_gap_execution_pass`): `closest_range_ever` in `[21.7,22.8]`, `cn_port=0`, `cn_fore=1`
- overtaking-starboard mirror family (`overtaking_starboard_mirror_execution_pass`): `closest_range_ever` in `[17,21]`, `cn_port=1`, `cn_fore=1`
- overtaking-starboard mirror long-range family (`overtaking_starboard_mirror_range_far_execution_pass`): `closest_range_ever` in `[20,24]`, `cn_port=1`, `cn_fore=1`
- overtaking-starboard mirror long-range spacing family (`overtaking_starboard_mirror_small_gap_execution_pass`, `overtaking_starboard_mirror_large_gap_execution_pass`): `closest_range_ever` in `[21.8,22.8]` for the small-gap variant and `[20.7,22.1]` for the large-gap variant, `cn_port=1`, `cn_fore=1`
- overtaken-port midrange family (`overtaken_port_standon_midrange_execution_pass`): `closest_range_ever` in `[17.7,18.4]`, `cn_port=1`, `cn_fore=1`, `colregs_mode=standon_ot:port`
- overtaken-starboard midrange family (`overtaken_starboard_standon_midrange_execution_pass`): `closest_range_ever` in `[18.15,18.30]`, `cn_port=0`, `cn_fore=1`, `colregs_mode=standon_ot:starboard`

Current shell-side checks:
- wall-time only
- head_on family: `wall_time<=12`
- crossing-starboard-giveway family: `wall_time<=12`, far variant `wall_time<=13`
- crossing-port-standon family: `wall_time<=12`, far variant `wall_time<=13`, stern variant `wall_time<=16`, unsure variant `wall_time<=12.1`
- overtaking families: `wall_time<=12`
- overtaken-port and overtaken-starboard midrange: `wall_time<=12`

Current reported execution metrics:
- `closest_range_ever`
- `cn_port`
- `cn_fore`
- `cn_crossed`
- `near_misses`
- `collisions`
- `wall_time`

## Running

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 --port_base=24000 10
./zlaunch.sh --case=head_on_execution_pass 10
./zlaunch.sh --just_make --jobs=2 --port_base=24000 10
```

Every mode uses isolated mission copies and compact per-case port blocks:
`case_base = port_base + case_idx*PORT_STRIDE`, with pShare ports starting at
`case_base + 15`.

This harness now stays on realized execution quality while still preserving the
case identity where that matters. In practice that means family-level CPA/side
checks for most cases and final-mode checks only for the admitted overtaken
midrange cases.

Side-check strength is not uniform across the suite:
- for the head-on and crossing families, the mission still begins with a substantial live encounter ahead, so `cn_port` is a meaningful execution-outcome check
- for the compact overtaking/overtaken families, the initial geometry is already largely side-resolved, so `cn_port` is better interpreted as "maintained the safe side" than "proved the side choice from an open setup"
- the promoted long-range overtaking siblings strengthen that evidence without changing H03's completion contract: across repeated runs they kept the expected side, widened realized CPA into the low-20s, and still completed on the same timing envelope
- `crossing_port_standon_far_execution_pass` now uses the H03-tuned `crossing_port_standon_exec_far_pass` stem sibling instead of the exact H01 far canonical because the original longer-range stem was not completion-stable enough for this harness
- H03 now uses `cn_crossed=0` across the suite and selective `cn_fore` checks where repeatability data showed that the fore/aft relationship is stable enough to strengthen the physical execution claim
- the spacing siblings now use longer-range stem geometries instead of the compact starts, which makes them better H03 fits even when the final mode still collapses to `cpa`
- the non-mirror `overtaking_starboard_large_gap` sibling is no longer part of the default H03 gate because repeated completion runs overlapped the small-gap sibling too much to serve as a stable discriminator
- the new overtaken-port midrange sibling is the first overtaken execution case in this repo that improves the live-encounter lead-in while still preserving `standon_ot:port` through completion
- a deeper/wider overtaken-starboard midrange sibling now clears the same bar: moving the contact farther back and one meter farther to starboard kept the completion-mode identity stable in repeated H03-style runs without weakening the physical side metrics
- the old compact `overtaken_port_standon_execution_pass` remains available for manual comparison, but it is no longer part of the default H03 gate because the midrange sibling is the cleaner completion-style execution case
- the overtaken long-range siblings did not clear the same bar, so they remain stem-only probes rather than H03 cases

## Current Gaps

- `overtaken_starboard_standon_execution_pass` is intentionally excluded because the shared stem does enter `standon_ot:starboard`, but it leaves that mode well before H03's completion-style evaluation point. In a traced run it first entered `standon_ot:starboard` at mission time `12.39675`, switched to `giveway:stern` at `83.59190`, reached `ARRIVED_ben=true` at `104.91397`, and was evaluated at `105.31343`. Keeping it in H03 would require a dedicated early-exit contract rather than the current "run through completion" model.
- the admitted `overtaken_starboard_standon_midrange_execution_pass` depends on a deeper/wider sibling, not the compact canonical geometry. Endpoint-only tuning was not enough; the stable version required both a one-meter wider starboard lane and a deeper start for the overtaking contact before the final mode stopped drifting at completion.
- `overtaken_port_standon_range_far_pass` remains outside H03 because its longer-range stem no longer preserves the stand-on identity through completion. In a preserved H03-style run it finished `grade=pass` with `closest_range_ever=16.503`, `cn_port=1`, and `wall_time=12.72`, but the final mode had already degraded to `cpa`.
- `overtaken_starboard_standon_range_far_pass` remains outside H03 because it still does not produce a clean completion-grade path under the current H03 overlay, so it is not yet a stable completion-style execution case.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
