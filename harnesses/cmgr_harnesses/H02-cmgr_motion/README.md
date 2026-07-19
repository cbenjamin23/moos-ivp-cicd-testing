# H02-cmgr_motion

Patch-driven moving encounter harness for
[`missions/cmgr_missions/cmgr_motion`](../../../missions/cmgr_missions/cmgr_motion).

This harness is the moving integration companion to `H01-cmgr_unit`. The unit
suite proves `pContactMgrV20` output behavior directly. This suite proves a
full moving mission can still complete transit safely when `pContactMgrV20`
feeds a spawned `BHV_AvoidCollision` behavior. The stem mission uses the
lower-band Forrest 19 / MIT_SP framing with the ownship starting at `(0,-60)`
and running a short `70m` eastbound corridor.

## Current Matrix

- `baseline_crossing_pass`: Posts a stationary `intruder` at `(39,-60)` two
  seconds after deploy under the static 20-meter range and 28-meter CPA alert,
  testing the normal `CONTACT_INFO` spawn path; passes when `ARRIVED=true`,
  `CONTACT_DETECTED=true`, and `COLLISION_TOTAL=0`.
- `offset_clear_pass`: Moves the stationary contact to `(45,-42)`, 18 meters
  north of the route, testing the clear trajectory outcome for off-lane
  geometry; passes when `ARRIVED=true`, `ENCOUNTER_TOTAL=0`, and
  `COLLISION_TOTAL=0`.
- `no_detect_clear_pass`: Moves the contact to `(45,-34)` and reduces the alert
  thresholds to 10 and 12 meters, exercising out-of-range non-detection;
  passes when `ARRIVED=true`, `ENCOUNTER_TOTAL=0`,
  `COLLISION_TOTAL=0`, and `CONTACT_DETECTED` remains false.
- `delayed_crossing_pass`: Posts a stationary on-lane contact at `(51,-60)`
  after 12 seconds and evaluates at 52 seconds, testing late alert and avoidance
  during transit; passes when `ARRIVED=true`, `CONTACT_DETECTED=true`, and
  `COLLISION_TOTAL=0`.
- `head_on_pass`: Posts `intruder` at `(71,-60)` moving west at `1.4`, testing
  alert-driven avoidance against an oncoming contact; passes when
  `ARRIVED=true`, `CONTACT_DETECTED=true`, and `COLLISION_TOTAL=0`.
- `runtime_alert_add_pass`: Starts without a static alert, adds the `avoid`
  alert by `BCM_ALERT_REQUEST` at one second, and posts the on-lane contact at
  two seconds, testing runtime alert creation; passes when `ARRIVED=true`,
  `CONTACT_DETECTED=true`, and `COLLISION_TOTAL=0`.
- `runtime_alert_reenable_pass`: Disables the static alert at one second,
  enables it at two seconds, and posts the contact at three seconds, exercising
  runtime alert re-enable; the current evaluator requires only the final
  `ARRIVED=true`, `CONTACT_DETECTED=true`, and `COLLISION_TOTAL=0` outcome and
  does not prove the disable step took effect.
- `hold_alerts_for_helm_pass`: Sets `hold_alerts_for_helm=true` for the baseline
  contact, exercising delayed alert release until the helm is driving; the
  current evaluator passes on `ARRIVED=true`, `CONTACT_DETECTED=true`, and
  `COLLISION_TOTAL=0` without checking when the alert was released.
- `filter_match_type_clear_pass`: Requires `match_type=ship` while posting the
  default `kayak` contact at `(45,-42)`, exercising type-filtered
  non-detection; passes when `ARRIVED=true`, `ENCOUNTER_TOTAL=0`,
  `COLLISION_TOTAL=0`, and `CONTACT_DETECTED` remains false.
- `stale_reappear_pass`: Sets `contact_max_age=5`, posts a three-second
  `intruder` at one second, then posts the same name on the route at 12 seconds,
  exercising retirement and reappearance; the evaluator passes when
  `ARRIVED=true`, `CONTACT_RETIRED=intruder`, `CONTACT_DETECTED=true`, and
  `COLLISION_TOTAL=0`, but does not tie the detection flag specifically to the
  second report.
- `two_contact_pass`: Posts avoid-worthy `alpha` at `(39,-56)` and background
  `bravo` at `(53,-34)`, exercising multi-contact tracking during avoidance;
  the current evaluator passes when `ARRIVED=true`, `CONTACT_DETECTED=true`,
  `ENCOUNTER_TOTAL=0`, and `COLLISION_TOTAL=0` while recording, but not grading,
  `CONTACTS_COUNT` and `CONTACTS_LIST`.
- `tight_alert_fail`: Reduces the baseline alert thresholds to 8 and 10 meters,
  exercising the late-alert/no-detection outcome; passes when `ARRIVED=true`,
  `COLLISION_TOTAL=0`, and `CONTACT_DETECTED` remains false.
- `avoid_disabled_fail`: Sends the alert update to `DISABLED_INFO` instead of
  the behavior's `CONTACT_INFO` variable and posts an oncoming contact at
  `(67,-60)` moving west at `1.8`, testing detection without behavior spawn;
  passes when `ARRIVED=true`, `CONTACT_DETECTED=true`,
  `ENCOUNTER_TOTAL=1`, and `COLLISION_TOTAL=0`.
- `runtime_alert_disable_fail`: Disables the static alert at one second before
  posting the same oncoming contact at two seconds, testing runtime alert
  suppression; passes when `ARRIVED=true`, `ENCOUNTER_TOTAL=1`,
  `COLLISION_TOTAL=0`, and `CONTACT_DETECTED` remains false.
- `fast_intruder_fail`: Posts an oncoming contact at `(57,-60)` moving west at
  `3.0`, testing detection under a faster encounter geometry; passes when
  `ARRIVED=true`, `CONTACT_DETECTED=true`, `ENCOUNTER_TOTAL>=1`, and
  `COLLISION_TOTAL=0`.

## Results Lines

Typical pass line:

```text
case=baseline_crossing_pass grade=pass form=cmgr_motion_tests mmod=baseline_crossing_pass eval=true arrived=true detected=true closest=intruder range=34 encounters=0 near_misses=0 collisions=0 mhash=[COLD-DUKE]
```

Expected-negative pass line:

```text
case=avoid_disabled_fail grade=pass form=cmgr_motion_tests mmod=avoid_disabled_fail eval=true expected=encounter_without_avoid arrived=true detected=true closest=intruder range=80 encounters=1 near_misses=0 collisions=0 mhash=[DARK-HYMM]
```

Field anatomy:

- `case`: harness case name
- `grade`: mission-level pass/fail from `pMissionEval`
- `form`: mission family name
- `mmod`: mission mode for the selected case
- `eval`: whether the evaluation checkpoint fired
- `expected`: optional evidence label for expected-negative cases
- `arrived`: whether the ownship reached the goal waypoint
- `detected`: whether contact manager produced the expected contact alert path
- `closest`: closest reported contact when included
- `range`: closest reported range when included
- `count`: alerted-contact count, when included
- `list`: known-contact list, when included
- `retired`: contact retired by `pContactMgrV20`, when included
- `encounters`: `uFldCollisionDetect` encounter total
- `near_misses`: `uFldCollisionDetect` near-miss total
- `collisions`: `uFldCollisionDetect` collision total
- `mhash`: short mission hash

## Running

```bash
./zlaunch.sh
./zlaunch.sh --log=full
./zlaunch.sh --jobs=2 --port_base=21000 10
./zlaunch.sh --case=head_on_pass 10
./zlaunch.sh --just_make 10
```

The harness defaults to headless runs and requires Bash 5.1 or newer for its
rolling scheduler. It safely re-executes Homebrew or Linuxbrew Bash when
available on systems whose default Bash is older.

Execution contract:

- logging defaults to `minimal` for every selected case; `--log=full` restores
  the stem's previous logger configuration before case patches
- every selected case, including serial and one-case runs, gets its own mission
  copy beneath a harness-owned `.harness_runs` root
- every case gets a unique 30-port block: MOOSDB at `case_base + 0..1` and
  pShare at `case_base + 15..16`
- `--jobs=<N>` is rolling: a pending case starts as soon as any active case
  finishes
- the copied stem `zlaunch.sh` receives `--mmod`, `--max_time`, and all four
  explicit ports
- during live runs, `pMissionEval` writes the authoritative grade; the harness
  prepends `case=` and only synthesizes failures for runner or cleanup errors
- cleanup uses the repository's canonical root-scoped teardown helper; no
  global `ktm`, `pkill`, or process-group extension is used

## Harness-skill migration validation

Validation on July 14, 2026 preserved all 15 case tokens, all 24 patch files,
the case-to-patch mapping, mission geometry, and every `pMissionEval`
condition.

Untouched legacy measurements at warp 10 were:

- one serial matrix: 15/15 passing in 158.32 seconds
- three `--jobs=2` batch-wave matrices: 45/45 passing in 93, 93, and 95
  seconds, for a 93.67-second mean
- `stale_reappear_pass`: 5/5 focused passes in 11-12 seconds after one full-run
  sample reached the outer `uMayFinish` ceiling but still produced its valid
  mission-owned passing row during shutdown

Migrated validation included:

- both skill static checkers, Bash syntax, old-Bash re-execution, invalid
  argument and unknown-case probes
- a retained 15-case `--just_make` run with 30 unique MOOSDB ports, 30 unique
  pShare ports, 24 intended `.moosx` sidecars, and no source-stem leakage
- standalone stem, nominal, expected-negative, and stale/reappear live passes
- a definitive isolated serial matrix with 15/15 passing rows in 171 seconds
- three rolling matrices with 45/45 passing rows in 90, 91, and 90 seconds,
  for a 90.33-second mean, about 3.6 percent faster than the matched legacy
  rolling mean
- a separate final checkout-state rolling matrix after runner hardening with
  15/15 passing rows in 89.58 seconds
- explicit missing-result, duplicate-result, launch-error, preparation-error,
  and teardown-error injection, including one result row for every selected
  case, stopped refill after infrastructure failure, and preserved diagnostics
  on teardown failure
- zero leftover scoped processes and zero listeners on the assigned retained-run
  ports

The first migrated serial matrix produced one strict case failure when
`fast_intruder_fail` reported two encounters instead of the required one. The
unchanged case then passed 5/5 focused repetitions with exactly one encounter,
and the definitive serial plus all three rolling matrices passed. This is
recorded as a non-reproducing timing/trajectory outlier; the case assertion was
not weakened.

Every retained vehicle log contained the same advisory `BHV_WARNING` that the
finite `waypt_transit` behavior had no remaining waypoints. The mission and
behavior configuration were unchanged by this migration, so the warning is
recorded for later case-quality work rather than added to grading or suppressed.
