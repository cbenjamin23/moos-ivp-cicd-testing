# nspatch Notes

This harness generates temporary sidecars in the stem mission:

- `meta_shoreside.moosx`
- `meta_vehicle.moosx`
- `meta_vehicle.bhvx`

The stem launchers use `nsplug -x`, so the sidecars are consumed without
modifying the stem files.

Use full-block patches for `pMissionEval`, `uTimerScript`, and behavior blocks.
Those blocks contain repeated keys such as `event`, `pass_condition`, and
`visual_hints`, where line patches are too blunt.
