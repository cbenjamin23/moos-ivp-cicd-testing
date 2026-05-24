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
case=baseline_crossing_pass  grade=pass  form=cmgr_motion_tests  mmod=baseline_crossing_pass  eval=true  arrived=true  detected=true  closest=intruder  range=34  encounters=0  near_misses=0  collisions=0  mhash=[COLD-DUKE]
```

Expected-negative pass line:

```text
case=avoid_disabled_fail  grade=pass  form=cmgr_motion_tests  mmod=avoid_disabled_fail  eval=true  expected=encounter_without_avoid  arrived=true  detected=true  closest=intruder  range=80  encounters=1  near_misses=0  collisions=0  mhash=[DARK-HYMM]
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
./zlaunch.sh --jobs=4 --port_base=21000 10
./zlaunch.sh --case=head_on_pass 10
./zlaunch.sh --just_make 10
```

This harness defaults to headless runs and uses the shared `xlaunch.sh`
wrapper, just like the other CI harnesses in this repo.

Wave mode notes:

- `--jobs=<N>` runs the matrix in waves of up to `N` isolated case copies
- each live case in a wave gets its own temp mission directory and unique port
  block
- the harness uses scoped teardown between waves, not global process cleanup
- this mode is intended for CI wall-clock reduction, not for interactive use
  alongside other MOOS missions

Latest validation:

- May 20, 2026
- focused expected-negative cases:
  `tight_alert_fail`, `avoid_disabled_fail`, `runtime_alert_disable_fail`,
  `fast_intruder_fail`
- grouped full matrix: `./zlaunch.sh --jobs=4 --port_base=25700 10`
- serial full matrix: `./zlaunch.sh --port_base=26250 10`
- compact port blocks: MOOSDB at `case_base + 0..1`, pShare at
  `case_base + 10..11`
