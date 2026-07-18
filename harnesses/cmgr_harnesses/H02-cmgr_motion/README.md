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

- `baseline_crossing_pass`
  Default on-lane contact. The vehicle should detect it early enough to route
  around it, arrive cleanly, and finish with zero collisions.
- `offset_clear_pass`
  A farther north stationary contact should stay clear enough that the mission
  arrives cleanly without registering an encounter.
- `delayed_crossing_pass`
  A similar on-lane contact appears later in the transit. This keeps the
  motion suite from depending only on a very early contact appearance.
- `head_on_pass`
  The intruder approaches head-on along the lane. This is a stronger geometry
  than the default crossing case but should still finish safely.
- `runtime_alert_add_pass`
  The moving mission starts without a static alert and posts a
  `BCM_ALERT_REQUEST` at runtime. The dynamic alert should create the spawned
  avoid-collision update, detect the contact, and finish without collision or
  encounter.
- `runtime_alert_reenable_pass`
  The configured alert is disabled and then re-enabled by runtime
  `BCM_ALERT_REQUEST` messages before the contact arrives. The re-enabled alert
  should behave like the static path and support a clean detected arrival.
- `hold_alerts_for_helm_pass`
  This case sets `hold_alerts_for_helm=true` in contact manager. The alert should
  remain held until the helm reaches `DRIVE`, then release in time for the
  spawned avoid behavior to complete the transit safely.
- `no_detect_clear_pass`
  A benign far-off contact should remain fully irrelevant. This case is useful
  because it proves the moving mission can stay clean without contact manager
  ever triggering at all.
- `filter_match_type_clear_pass`
  The contact is a default kayak while the alert requires `match_type=ship`.
  The vehicle should still arrive cleanly, and the mission grades this case by
  confirming the filtered contact does not trip `CONTACT_DETECTED`.
- `stale_reappear_pass`
  A short-lived contact with the same name is retired through
  `contact_max_age`, then reappears on the lane. The mission requires both the
  retire flag and the later successful alert/avoid path.
- `two_contact_pass`
  One avoid-worthy contact plus one irrelevant background contact should still
  result in a clean arrival. This extends the motion layer into multi-contact
  territory without making the grade depend on brittle exact count/list
  assertions. The case still reports `count` and `list`.
- `tight_alert_fail`
  This case delays the `pContactMgrV20` alert. In the tuned geometry this
  should still arrive cleanly, but without the contact alert path firing in
  time. The case grades `pass` when that expected no-detection evidence is
  observed.
- `avoid_disabled_fail`
  Contact manager still detects the contact, but this case redirects the alert
  away from `CONTACT_INFO`, so the avoid-collision behavior never spawns. In
  the tuned case the vehicle still arrives without collision, and the case
  grades `pass` when the expected encounter-without-avoid evidence appears.
- `runtime_alert_disable_fail`
  The configured alert is disabled by a runtime `BCM_ALERT_REQUEST` before a
  harsh contact arrives. The case grades `pass` when the vehicle still arrives
  without collision, the encounter occurs, and the contact alert path remains
  disabled.
- `fast_intruder_fail`
  A faster head-on intruder creates a harsher encounter. The case grades `pass`
  when contact manager detects the intruder and the expected encounter evidence
  is observed without an actual collision.

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
