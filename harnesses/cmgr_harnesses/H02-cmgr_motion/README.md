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
- `no_detect_clear_pass`
  A benign far-off contact should remain fully irrelevant. This case is useful
  because it proves the moving mission can stay clean without contact manager
  ever triggering at all.
- `two_contact_pass`
  One avoid-worthy contact plus one irrelevant background contact should still
  result in a clean arrival. This extends the motion layer into multi-contact
  territory without making the grade depend on brittle exact count/list
  assertions. The case still reports `count` and `list`.
- `tight_alert_fail`
  `pContactMgrV20` is patched to alert too late. In the tuned geometry this
  means the mission still arrives, but it no longer satisfies the required
  contact-detected safe-encounter rule and therefore grades `fail`.
- `avoid_disabled_fail`
  Contact manager still detects the contact, but the alert variable is patched
  away from `CONTACT_INFO`, so the avoid-collision behavior never spawns. In
  the tuned case the vehicle still arrives, but it no longer stays
  encounter-free, which is the mission-level failure this case is meant to
  catch.
- `fast_intruder_fail`
  A faster head-on intruder creates a harsher encounter. This case is graded as
  a failure when the mission can no longer remain encounter-free under that
  geometry, even if it still avoids an actual collision.

## Results Lines

Typical pass line:

```text
case=baseline_crossing_pass  expected=pass  actual=pass  status=ok  grade=pass  form=cmgr_motion_tests  mmod=baseline_crossing_pass  eval=true  arrived=true  detected=true  closest=intruder  range=34  encounters=0  near_misses=0  collisions=0  mhash=[COLD-DUKE]
```

Typical fail line:

```text
case=avoid_disabled_fail  expected=fail  actual=fail  status=ok  grade=fail  form=cmgr_motion_tests  mmod=avoid_disabled_fail  eval=true  arrived=true  detected=true  closest=intruder  range=81  encounters=1  near_misses=0  collisions=0  mhash=[VICE-SLUG]
```

Field anatomy:

- `case`: harness case name
- `expected`: what the harness expected the mission to grade
- `actual`: what the mission actually graded
- `status`: `ok` when expected equals actual
- `grade`: mission-level pass/fail from `pMissionEval`
- `form`: mission family name
- `mmod`: case-specific mission mode
- `eval`: whether the evaluation checkpoint fired
- `arrived`: whether the ownship reached the goal waypoint
- `detected`: whether contact manager produced the expected contact alert path
- `closest`: closest reported contact when included
- `range`: closest reported range when included
- `count`: alerted-contact count, when included
- `list`: known-contact list, when included
- `encounters`: `uFldCollisionDetect` encounter total
- `near_misses`: `uFldCollisionDetect` near-miss total
- `collisions`: `uFldCollisionDetect` collision total
- `mhash`: short mission hash

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=head_on_pass 10
./zlaunch.sh --just_make 10
```

This harness defaults to headless runs and uses the shared `xlaunch.sh`
wrapper, just like the other CI harnesses in this repo.

Latest validation:

- March 20, 2026
- full matrix: `9/9` passing
- warp: `10`
- wall clock: about `97` seconds
