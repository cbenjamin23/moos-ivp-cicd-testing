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

Logging defaults to `NODE_REPORT`, `SPOOF_CANCEL`, and the evaluation cutoff.
Use `--log=full` to restore the original five-variable diagnostic logger.

## Cases

- `config_static_report_pass`: Configures stationary `alpha` at `(10,20)` with explicit heron/red/green/AIS fields and heading 90; passes when one Alpha report contains the exact `X=10`, `Y=20`, `SPD=0`, `HDG=90`, `TYPE=heron`, `GROUP=red`, `COLOR=green`, and `VSOURCE=ais` fields together.
- `mail_spoof_report_pass`: Posts runtime `SPOOF` for stationary `bravo` at `(5,-3)` with kayak/yellow/heading-180 fields; passes when one Bravo report contains the exact position, zero speed, heading, type, and color together.
- `mail_group_vsource_pass`: Posts runtime `papa` at `(4,6)` with zero speed, heading 270, `group=team`, `type=ship`, `color=yellow`, and `vsource=radar`; passes when one Papa report contains every supplied field together.
- `default_fields_pass`: Configures ship/orange/heading-135/speed-0.4/length-8 defaults, then declares `charlie` with position only; passes when one Charlie report contains all five inherited fields together.
- `generated_name_pass`: Declares unnamed blue AUVs at `(1,2)` and `(3,4)`, testing automatic name generation and counter advancement; passes when both position-specific reports exist under two distinct names matching `C[0-9]{4}`.
- `moving_contact_advances_pass`: Configures `delta` at `(0,0)` with heading 90 and speed 2, testing propagated motion; passes when reports contain the configured fields and the last reported X exceeds the first by more than 0.1.
- `duration_expires_pass`: Configures `echo` with `dur=0.7`, testing per-contact expiration; passes when `echo` reports initially and no `NAME=echo` report appears more than 1.4 seconds after the first report.
- `explicit_zero_duration_persists_pass`: Posts runtime `quebec` with `dur=0`, testing zero as non-expiring duration; passes when `NAME=quebec` still appears more than 1.3 seconds after its first report.
- `duplicate_name_update_pass`: Posts `romeo` first at `(1,1)` and later at `(8,8)`, testing same-name replacement; passes when post-update reports contain `X=8,Y=8` and no longer contain `X=1` or `Y=1`.
- `cancel_vname_pass`: Creates red-group `foxtrot` and `golf`, then sends `SPOOF_CANCEL=vname=foxtrot`; passes when Foxtrot is absent 0.4 seconds after cancellation while Golf still reports in that same post-cancel window.
- `cancel_group_pass`: Creates red-group `hotel` and blue-group `india`, then cancels `group=red`; passes when Hotel is absent 0.4 seconds after cancellation while India still reports in that same post-cancel window.
- `cancel_contact_alias_pass`: Creates `kilo` and `lima`, then sends the `contact=kilo` cancellation alias; passes when Kilo is absent 0.4 seconds after cancellation while Lima still reports in that same post-cancel window.
- `runtime_defaults_pass`: Configures ship/orange/heading-135/speed-0.4/length-8 defaults and then posts position-only `mike`; passes when one Mike report contains all five inherited fields together.
- `default_after_spoof_order_pass`: Places the config-time `orderly` spoof before its default declarations, testing order-independent startup parsing; passes when its report contains ship/orange/heading-222/length-9 defaults.
- `runtime_length_ignored_pass`: Sets `default_length=8` and posts runtime `victor` with `len=12`, testing the current rule that runtime length is ignored; passes when Victor reports `LENGTH=8` and never `LENGTH=12`.
- `default_group_vsource_not_inherited_pass`: Configures `default_group=fleet` and `default_vsource=radar`, then declares `whiskey` without either field; passes when Whiskey reports without `GROUP=fleet` or `VSOURCE=radar`.
- `default_duration_expires_pass`: Sets `default_duration=0.7` and declares `november` without `dur`, testing inherited expiration; passes when November reports initially but not more than 1.4 seconds after its first report.
- `invalid_default_vtype_fallback_pass`: Sets invalid config default `dragon` and declares `xray`, testing validation of configured vehicle types; passes when Xray reports built-in `TYPE=kayak` and not `TYPE=dragon`.
- `invalid_default_color_fallback_pass`: Sets invalid config default `notacolor` and declares `colorbad`, testing validation of configured colors; passes when the report uses built-in `COLOR=purple` and not `notacolor`.
- `mixed_case_request_normalized_pass`: Posts mixed-case name/type/group/color/source fields, testing runtime lowercase normalization; passes when the report contains `mixed`, `heron`, `teama`, `yellow`, and `radar` in lowercase.
- `runtime_color_unvalidated_pass`: Posts runtime `colorfree` with unknown but alphanumeric `color=notacolor`, testing runtime color passthrough; passes when that exact color is reported.
- `unknown_alnum_type_preserved_pass`: Posts runtime `yankee` with unknown but alphanumeric `type=dragon`, testing runtime type passthrough; passes when `TYPE=dragon` is reported.
- `negative_speed_accepted_pass`: Posts runtime `backward` from `(3,3)` with `spd=-1` and heading 90, testing acceptance and westward propagation of a negative speed; passes when its first report contains `Y=3`, `SPD=-1`, and `HDG=90` together with X between 2.4 and 3.0.
- `invalid_request_absent_pass`: Posts `juliet` with X but no Y, testing required-position validation; passes when pMissionEval sees no `NODE_REPORT` at all through evaluation.
- `invalid_x_absent_pass`: Posts `zulu` with nonnumeric `x=west`, testing coordinate parsing; passes when no `NODE_REPORT` is published.
- `invalid_name_absent_pass`: Posts name `bad-name`, testing alphanumeric name validation; passes when no `NODE_REPORT` is published.
- `invalid_group_absent_pass`: Posts `sierra` with non-alphanumeric `group=bad-team`; passes when the malformed request produces no `NODE_REPORT`.
- `invalid_type_absent_pass`: Posts `alpha2` with non-alphanumeric `type=bad-type`; passes when the malformed request produces no `NODE_REPORT`.
- `invalid_vsource_absent_pass`: Posts `bravo2` with non-alphanumeric `vsource=bad-source`; passes when the malformed request produces no `NODE_REPORT`.
- `invalid_heading_absent_pass`: Posts `charlie2` with nonnumeric `hdg=east`; passes when the malformed request produces no `NODE_REPORT`.
- `invalid_speed_absent_pass`: Posts `tango` with nonnumeric `spd=fast`; passes when the malformed request produces no `NODE_REPORT`.
- `invalid_length_absent_pass`: Posts `uniform` with nonnumeric `len=long`; passes when the malformed request produces no `NODE_REPORT`.
- `invalid_duration_absent_pass`: Posts `oscar` with nonnumeric `dur=soon`; passes when the malformed request produces no `NODE_REPORT`.

## Coverage-Strengthening Validation

The July 2026 strengthening pass made explicit-field checks row-specific,
separated the general default cases from the dedicated group/source
non-inheritance case, expanded generated-name coverage to two contacts, and
required each cancellation case's untargeted peer to keep reporting. Mutations
to Alpha's X position, Charlie's inherited length, one generated request's
missing-name contract, and Foxtrot's targeted cancellation all failed their
artifact checks while the mission-owned grade remained pass.

The final four-job family run passed 33/33 cases in 54 seconds. Its retained
workdirs contained 33 distinct MOOSDB ports; generated names were `C0001` and
`C0002`, the Foxtrot post-cancel window contained only Golf, and no scoped MOOS
process or listener remained. Generated-name and targeted-cancellation cases
also passed with `--log=full`.
