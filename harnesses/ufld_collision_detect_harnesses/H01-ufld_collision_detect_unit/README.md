# H01-ufld_collision_detect_unit

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

Live `uFldCollisionDetect` harness for contact/contact CPA event reporting.
Each case runs in an isolated copy of the shared uField app stem, and
`pMissionEval` owns the substantive verdict from live MOOS variables.

```sh
./zlaunch.sh --jobs=4 --port_base=4800 10
./zlaunch.sh --case=collision_event_pass --port_base=4800 10
```

## Cases

- `collision_event_pass` Holds Alpha at `x=0` while Bravo closes to 2 meters and opens again with `collision_range=3`; passes when `COLLISION_TOTAL=1`, `UCD_REPORT` is exactly `cpa=2,vname1=bravo,vname2=alpha,rank=collision`, and a range pulse is published.
- `near_miss_event_pass` Sets `collision_range=1` and `near_miss_range=6`, then closes Bravo to 3 meters from Alpha before reopening; passes when `NEAR_MISS_TOTAL=1` and the exact report ranks the 3-meter CPA as `near_miss`.
- `clear_encounter_report_pass` Sets `report_all_encounters=true` and moves Bravo from 30 to 10 meters from Alpha and back outside the 20-meter encounter range; passes when `ENCOUNTER_TOTAL=1` and the exact report ranks the 10-meter CPA as `clear`.
- `closest_range_posts_pass` Posts stationary Alpha and Bravo node reports 8 meters apart without completing an encounter; passes when both `UCD_CLOSEST_RANGE` and `UCD_CLOSEST_RANGE_EVER` equal `8`.
- `reset_closest_range_ever_pass` Establishes an 8-meter closest range, posts `UCD_RESET=true`, then places Bravo 12 meters from Alpha; passes when `UCD_CLOSEST_RANGE_EVER` is `12`, proving the pre-reset minimum was cleared.
- `pulse_render_false_pass` Sets `pulse_render=false` and completes a 2-meter collision; passes when the exact collision report is published and no `VIEW_RANGE_PULSE` is observed.
- `ignore_group_blocks_pass` Sets `ignore_group=red` and drives two red-group contacts through a 2-meter CPA; passes when no `UCD_REPORT` is published.
- `reject_group_blocks_pass` Sets `reject_group=blue` and drives blue Bravo through a 2-meter CPA with red Alpha; passes when no `UCD_REPORT` is published.
- `condition_blocks_pass` Configures `condition=TEST_GATE=true` but never posts `TEST_GATE`, then drives the ordinary 2-meter collision sequence; passes when no `UCD_REPORT` is published.
- `condition_allows_pass` Configures the same condition, posts `TEST_GATE=true` before the node reports, and drives the 2-meter collision sequence; passes when `COLLISION_TOTAL=1` and the exact collision report is published.
- `collision_flag_macro_pass` Configures `collision_flag=UCD_HIT=hit_$V2_$CPA` and completes the 2-meter Alpha/Bravo collision; passes when the expanded flag is exactly `UCD_HIT=hit_alpha_2`.
- `near_miss_flag_macro_pass` Configures `near_miss_flag=UCD_NEAR=near_$V2_$CPA` and completes a 3-meter near miss; passes when the expanded flag is exactly `UCD_NEAR=near_alpha_3`.
- `encounter_flag_density_pass` Configures `encounter_flag=UCD_DENSITY=$[V1@40]` and completes a clear Alpha/Bravo encounter; passes when the contact-density macro posts numeric `UCD_DENSITY=1` for Bravo's contacts within 40 meters.
- `outside_encounter_range_blocks_pass` Sets all three event ranges to 1 meter and drives a 3-meter CPA; passes when no `UCD_REPORT` is published even though `report_all_encounters=true`.
- `closest_posts_disabled_absent_pass` Disables both closest-range publications and completes a 2-meter collision; passes when the exact collision report is still published while neither `UCD_CLOSEST_RANGE` nor `UCD_CLOSEST_RANGE_EVER` appears.
- `clear_encounter_default_suppressed_pass` Completes a 10-meter clear encounter with the default `report_all_encounters=false`; passes when `ENCOUNTER_TOTAL=1` but no `UCD_REPORT` is published.
- `collision_flag_numeric_cpa_pass` Configures `collision_flag=UCD_COLLISION_CPA=$CPA` and completes a 2-meter collision; passes when `UCD_COLLISION_CPA` is posted as numeric value `2`.
- `encounter_rings_true_posts_pass` Leaves encounter rings enabled, posts Alpha at `(0,0)` and Bravo at `(10,0)`, and checks each publication before the next node arrives; passes when the two exact 20-meter white `VIEW_CIRCLE` payloads are published with labels `alpharng` and `bravorng`.
- `encounter_rings_false_absent_pass` Sets `encounter_rings=false` and posts Alpha and Bravo 10 meters apart; passes when no `VIEW_CIRCLE` is published.
- `range_normalization_params_pass` Configures collision, near-miss, and encounter ranges as `6`, `3`, and `4`, then completes a 5-meter CPA; passes when `COLLISION_DETECT_PARAMS` exactly reports normalized ranges `6`, `6`, and `6`, `COLLISION_TOTAL=1`, and the exact event report ranks the CPA as a collision.

Logging is minimal by default and runs without `pLogger`, except that
`range_normalization_params_pass` records only `COLLISION_DETECT_PARAMS` for
its exact serialized-payload supplement. Use `--log=full` for the complete
matrix, or combine it with `--case=NAME` for one fully logged diagnostic case.
