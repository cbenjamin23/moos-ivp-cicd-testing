# shadow_behavior_motion

Deterministic two-vehicle motion stem for `BHV_Shadow`.

`abe` runs `BHV_Shadow` against contact `ben`. `ben` runs waypoint transits
that provide controlled contact heading and speed. The default case is an
eastbound shadowing run where `abe` should match `ben`'s reported contact
heading and speed.

The stem grades on mission-owned outputs bridged to shoreside:

- `SHADOW_RELEVANCE`
- `SHADOW_CONTACT_SPEED`
- `SHADOW_CONTACT_HEADING`
- `ABE_NAV_SPEED`
- `ABE_NAV_HEADING`
- `BEN_NAV_SPEED`
- `BEN_NAV_HEADING`
- `BHV_WARNING`
- `BHV_ERROR`

Default pass conditions are:

- `TEST_EVAL_READY = true`
- `SHADOW_RELEVANCE > 0`
- contact heading and both vehicle headings are eastbound
- `ABE_NAV_SPEED` and `BEN_NAV_SPEED` show active motion
- `BHV_WARNING_SEEN = false`
- `BHV_ERROR_SEEN = false`

Run from this directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_shadow_pass 10
./zlaunch.sh --case=turn_north_shadow_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=9000 10
```

Named cases are implemented in the paired harness:

[`/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion)
