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

- `config_static_report_pass`: Configures static `alpha` at `(10,20)` with explicit type, group, color, source, heading, and zero speed; passes when report history contains `NAME=alpha`, `TYPE=heron`, `GROUP=red`, `COLOR=green`, `VSOURCE=ais`, and `HDG=90`.
- `mail_spoof_report_pass`: Posts runtime `SPOOF` for `bravo` at `(5,-3)` with kayak/yellow/heading-180 fields; passes when report history contains `NAME=bravo`, `TYPE=kayak`, `COLOR=yellow`, and `HDG=180`.
- `mail_group_vsource_pass`: Posts runtime `papa` with `group=team`, `type=ship`, `color=yellow`, `vsource=radar`, and heading 270; passes when all six named fields appear in report history.
- `default_fields_pass`: Configures defaults for type, group, color, source, heading, speed, and length, then declares `charlie` with position only; passes when its history contains `TYPE=ship`, `COLOR=orange`, `HDG=135`, `SPD=0.4`, and `LENGTH=8`, while group/source are not graded.
- `generated_name_pass`: Declares an unnamed AUV, testing automatic contact naming; passes when a report contains a name beginning with `C`, plus `TYPE=auv` and `COLOR=blue`.
- `moving_contact_advances_pass`: Configures `delta` at `(0,0)` with heading 90 and speed 2, testing propagated motion; passes when reports contain the configured fields and the last reported X exceeds the first by more than 0.1.
- `duration_expires_pass`: Configures `echo` with `dur=0.7`, testing per-contact expiration; passes when `echo` reports initially and no `NAME=echo` report appears more than 1.4 seconds after the first report.
- `explicit_zero_duration_persists_pass`: Posts runtime `quebec` with `dur=0`, testing zero as non-expiring duration; passes when `NAME=quebec` still appears more than 1.3 seconds after its first report.
- `duplicate_name_update_pass`: Posts `romeo` first at `(1,1)` and later at `(8,8)`, testing same-name replacement; passes when post-update reports contain `X=8,Y=8` and no longer contain `X=1` or `Y=1`.
- `cancel_vname_pass`: Creates `foxtrot` and `golf`, then sends `SPOOF_CANCEL=vname=foxtrot`; passes when both existed before cancellation and no `foxtrot` report appears more than 1.3 seconds after the cancel mail.
- `cancel_group_pass`: Creates red-group `hotel` and blue-group `india`, then cancels `group=red`; passes when both existed initially and no `hotel` report appears after the cancellation window.
- `cancel_contact_alias_pass`: Creates `kilo` and `lima`, then sends the `contact=kilo` cancel alias; passes when both existed initially and no `kilo` report appears after the cancellation window.
- `runtime_defaults_pass`: Configures runtime defaults and then posts position-only `mike`; passes when its history contains `TYPE=ship`, `COLOR=orange`, `HDG=135`, `SPD=0.4`, and `LENGTH=8`, while configured group/source are not graded.
- `default_after_spoof_order_pass`: Places the config-time `orderly` spoof before its default declarations, testing order-independent startup parsing; passes when its report contains ship/orange/heading-222/length-9 defaults.
- `runtime_length_ignored_pass`: Sets `default_length=8` and posts runtime `victor` with `len=12`, testing the current rule that runtime length is ignored; passes when Victor reports `LENGTH=8` and never `LENGTH=12`.
- `default_group_vsource_not_inherited_pass`: Configures `default_group=fleet` and `default_vsource=radar`, then declares `whiskey` without either field; passes when Whiskey reports without `GROUP=fleet` or `VSOURCE=radar`.
- `default_duration_expires_pass`: Sets `default_duration=0.7` and declares `november` without `dur`, testing inherited expiration; passes when November reports initially but not more than 1.4 seconds after its first report.
- `invalid_default_vtype_fallback_pass`: Sets invalid config default `dragon` and declares `xray`, testing validation of configured vehicle types; passes when Xray reports built-in `TYPE=kayak` and not `TYPE=dragon`.
- `invalid_default_color_fallback_pass`: Sets invalid config default `notacolor` and declares `colorbad`, testing validation of configured colors; passes when the report uses built-in `COLOR=purple` and not `notacolor`.
- `mixed_case_request_normalized_pass`: Posts mixed-case name/type/group/color/source fields, testing runtime lowercase normalization; passes when the report contains `mixed`, `heron`, `teama`, `yellow`, and `radar` in lowercase.
- `runtime_color_unvalidated_pass`: Posts runtime `colorfree` with unknown but alphanumeric `color=notacolor`, testing runtime color passthrough; passes when that exact color is reported.
- `unknown_alnum_type_preserved_pass`: Posts runtime `yankee` with unknown but alphanumeric `type=dragon`, testing runtime type passthrough; passes when `TYPE=dragon` is reported.
- `negative_speed_accepted_pass`: Posts runtime `backward` with `spd=-1` and heading 90, testing acceptance of negative numeric speed; passes when both exact fields appear in its report.
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
