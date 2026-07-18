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

- `head_on_execution_pass`
  Head-on encounter used to check the realized head-on maneuver through
  completion.
- `head_on_cpa_fallback_execution_pass`
  Near-head-on geometry expected to complete through the CPA fallback path
  rather than true head-on.
- `head_on_port_offset_execution_pass`
  Offset head-on encounter on the port side; expected to preserve the head-on
  execution shape through completion.
- `head_on_starboard_offset_execution_pass`
  Offset head-on encounter on the starboard side; expected to preserve the
  head-on execution shape through completion.
- `crossing_starboard_giveway_execution_pass`
  Starboard crossing where ownship should take the give-way role and complete
  on the expected side.
- `crossing_starboard_giveway_far_execution_pass`
  Longer-range starboard crossing where ownship should still give way and
  complete inside the expected CPA/side envelope.
- `crossing_starboard_giveway_close_execution_pass`
  Tighter starboard crossing where ownship should still give way and complete
  cleanly.
- `crossing_port_standon_execution_pass`
  Port-side crossing where ownship should stand on and complete with the
  expected side relationship.
- `crossing_port_standon_unsure_bow_execution_pass`
  Port-side crossing that enters the stand-on unsure-bow branch before
  completion; expected to preserve a clean stand-on execution outcome.
- `crossing_port_standon_stern_execution_pass`
  Port-side crossing that settles to the stand-on stern branch and completes in
  the expected CPA/side envelope.
- `crossing_port_standon_far_execution_pass`
  Longer-range port-side stand-on setup tuned for completion-style execution
  grading.
- `crossing_port_standon_close_execution_pass`
  Tighter port-side stand-on setup expected to complete without crossing or
  collision.
- `crossing_port_standon_close_unsure_bow_execution_pass`
  Tighter port-side setup that enters unsure-bow before completing with a clean
  stand-on outcome.
- `crossing_port_standon_unsure_execution_pass`
  Port-side crossing that enters the generic stand-on unsure branch and
  completes inside its narrower CPA envelope.
- `overtaking_starboard_execution_pass`
  Ownship overtakes on the starboard passing side and should maintain the
  expected safe-side execution shape.
- `overtaking_starboard_range_far_execution_pass`
  Longer-range overtaking setup on the starboard passing side, used to
  strengthen completion-side evidence.
- `overtaking_starboard_small_gap_execution_pass`
  Overtaking setup with a smaller lateral gap; expected to keep the same safe
  side and CPA band.
- `overtaking_starboard_mirror_execution_pass`
  Mirrored overtaking setup on the opposite passing side; expected to preserve
  the mirrored safe-side relationship.
- `overtaking_starboard_mirror_range_far_execution_pass`
  Longer-range mirrored overtaking setup, used to strengthen completion-side
  evidence on the mirrored side.
- `overtaking_starboard_mirror_small_gap_execution_pass`
  Mirrored overtaking setup with a smaller lateral gap; expected to keep the
  mirrored safe side and CPA band.
- `overtaking_starboard_mirror_large_gap_execution_pass`
  Mirrored overtaking setup with a larger lateral gap; expected to remain in
  the supported mirrored execution envelope.
- `overtaken_port_standon_midrange_execution_pass`
  Ownship is being overtaken on its port side with a midrange live-encounter
  setup; expected to preserve the stand-on overtaken role through completion.
- `overtaken_starboard_standon_midrange_execution_pass`
  Ownship is being overtaken on its starboard side with a midrange
  live-encounter setup; expected to preserve the stand-on overtaken role
  through completion.

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
