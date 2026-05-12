# H01-ufld_collision_detect_unit

Live `uFldCollisionDetect` harness for contact/contact CPA event reporting.

```sh
./zlaunch.sh --jobs=4 --port_base=4800 10
./zlaunch.sh --case=collision_event_pass --port_base=4800 10
```

## Cases

- `collision_event_pass` Verifies a closing/opening contact pair below the collision range posts collision totals, report, and pulse evidence.
- `near_miss_event_pass` Verifies the same CPA event path ranks a wider miss as `near_miss`.
- `clear_encounter_report_pass` Verifies `report_all_encounters=true` publishes clear encounter reports.
- `closest_range_posts_pass` Verifies closest-range and closest-range-ever telemetry are published from live node reports.
- `reset_closest_range_ever_pass` Verifies `UCD_RESET` clears closest-range-ever before later range tracking.
- `pulse_render_false_pass` Verifies collision reports can be generated while range-pulse visuals are suppressed.
- `ignore_group_blocks_pass` Verifies contacts in an ignored group do not generate encounter reports.
- `reject_group_blocks_pass` Verifies rejected-group contacts are removed before encounter evaluation.
- `condition_blocks_pass` Verifies an unsatisfied logic condition blocks generated CPA events.
- `condition_allows_pass` Verifies a satisfied logic condition allows the same CPA event path.
- `collision_flag_macro_pass` Verifies collision flag variable/value macros expand with contact names and CPA.
- `near_miss_flag_macro_pass` Verifies near-miss flag value macros expand on a non-collision encounter.
- `encounter_flag_density_pass` Verifies encounter flags can expand contact-density macros at encounter completion.
- `outside_encounter_range_blocks_pass` Verifies CPA events outside the configured encounter range are not reported.
- `closest_posts_disabled_absent_pass` Verifies disabling closest-range posting suppresses closest-range mail while reports still occur.
- `clear_encounter_default_suppressed_pass` Verifies clear encounters increment totals but suppress `UCD_REPORT` unless `report_all_encounters=true`.
- `collision_flag_numeric_cpa_pass` Verifies `$CPA` can be posted as a numeric collision flag value.
- `encounter_rings_true_posts_pass` Verifies default encounter-ring rendering posts per-contact `VIEW_CIRCLE` mail.
- `encounter_rings_false_absent_pass` Verifies `encounter_rings=false` suppresses per-contact `VIEW_CIRCLE` mail.
- `range_normalization_params_pass` Verifies invalid range ordering is normalized and advertised in `COLLISION_DETECT_PARAMS`.
