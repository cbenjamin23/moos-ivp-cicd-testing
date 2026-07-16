# H01-pspoofnode_unit

Live `pSpoofNode` harness for structured `NODE_REPORT` publication.

```sh
./zlaunch.sh --jobs=4 --port_base=9000 10
./zlaunch.sh --case=config_static_report_pass --port_base=9000 10
```

The launcher requires Bash 5.1 or newer for rolling scheduling. Every case,
including serial runs, executes in its own mission copy and port block.
pMissionEval defines the evaluation boundary and writes the authoritative
mission row. It requires a `NODE_REPORT` in the twenty-three positive-output
cases and requires no `NODE_REPORT` in the ten rejected-request cases. Because
the remaining assertions test exact structured fields or publication history
rather than only the final MOOS value, the launcher supplements that grade with
the existing field, movement, expiration, and cancellation checks from the
`.alog`. Those checks consider only reports at or before the first
`MISSION_EVALUATED=true`; later shutdown traffic cannot change the verdict. No
extra test application or grading variable is used.

## Cases

- `config_static_report_pass` Verifies a config-time spoof produces a named report with explicit type, group, color, source, and heading fields.
- `mail_spoof_report_pass` Verifies a runtime `SPOOF` mail request produces a contact report.
- `mail_group_vsource_pass` Verifies runtime `SPOOF` mail preserves explicit group and source fields.
- `default_fields_pass` Verifies configured default type, color, heading, speed, and length are applied when omitted from the request.
- `generated_name_pass` Verifies unnamed spoof requests receive an auto-generated contact name.
- `moving_contact_advances_pass` Verifies a moving spoofed contact produces multiple reports with advancing position.
- `duration_expires_pass` Verifies a finite-duration contact stops reporting after expiration.
- `explicit_zero_duration_persists_pass` Verifies an explicit zero-duration runtime spoof keeps reporting instead of expiring.
- `duplicate_name_update_pass` Verifies a second spoof request with the same name replaces the contact position.
- `cancel_vname_pass` Verifies `SPOOF_CANCEL` by contact name removes one contact while another continues.
- `cancel_group_pass` Verifies `SPOOF_CANCEL` by group removes grouped contacts while other groups continue.
- `cancel_contact_alias_pass` Verifies the `contact=` cancel alias removes only the named contact.
- `runtime_defaults_pass` Verifies runtime `SPOOF` mail inherits configured default type, color, heading, speed, and length.
- `default_after_spoof_order_pass` Verifies config defaults apply to config-time spoof requests even when the spoof line appears before the defaults.
- `runtime_length_ignored_pass` Verifies an explicit runtime `len` field does not override the configured default length.
- `default_group_vsource_not_inherited_pass` Verifies configured default group/source fields are not inherited by spoof requests that omit those fields.
- `default_duration_expires_pass` Verifies `default_duration` expires a config-time spoof when no per-request duration is supplied.
- `invalid_default_vtype_fallback_pass` Verifies an invalid configured default vehicle type is ignored and the built-in kayak default remains active.
- `invalid_default_color_fallback_pass` Verifies an invalid configured default color is ignored and the built-in purple default remains active.
- `mixed_case_request_normalized_pass` Verifies runtime spoof requests are normalized to lowercase before field validation and reporting.
- `runtime_color_unvalidated_pass` Verifies runtime color tokens are passed through even when they are not recognized color names.
- `unknown_alnum_type_preserved_pass` Verifies an alphanumeric runtime type token is preserved even when it is not a known vehicle type.
- `negative_speed_accepted_pass` Verifies numeric negative runtime speeds are accepted and reflected in node reports.
- `invalid_request_absent_pass` Verifies malformed spoof requests do not create a node report.
- `invalid_x_absent_pass` Verifies nonnumeric X coordinates are rejected without creating reports.
- `invalid_name_absent_pass` Verifies non-alphanumeric spoof names are rejected without creating reports.
- `invalid_group_absent_pass` Verifies non-alphanumeric spoof groups are rejected without creating reports.
- `invalid_type_absent_pass` Verifies non-alphanumeric spoof type tokens are rejected without creating reports.
- `invalid_vsource_absent_pass` Verifies non-alphanumeric spoof source tokens are rejected without creating reports.
- `invalid_heading_absent_pass` Verifies nonnumeric headings are rejected without creating reports.
- `invalid_speed_absent_pass` Verifies nonnumeric spoof speeds are rejected without creating reports.
- `invalid_length_absent_pass` Verifies nonnumeric spoof lengths are rejected without creating reports.
- `invalid_duration_absent_pass` Verifies nonnumeric duration requests are rejected without creating reports.
