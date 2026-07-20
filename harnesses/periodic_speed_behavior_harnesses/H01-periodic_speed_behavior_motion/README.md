# H01-periodic_speed_behavior_motion

Harness for `BHV_PeriodicSpeed` using the stem mission at
`missions/periodic_speed_behavior_missions/periodic_speed_behavior_motion`.

The stem keeps a single simulated vehicle on a constant heading while
`BHV_PeriodicSpeed` alternates between lazy and busy speed windows. The mission
grades behavior-owned status variables and observed vehicle speed.

The drawn track segment is only a visual reference. Because the behavior under
test is a clock, positive cases sample its state at a scheduled time rather
than waiting for the vehicle to reach the end of the line; every sample must
occur before its fallback deadline with the helm still in `DRIVE` and no
`BHV_ERROR`.

## Current Matrix

- `baseline_cycle_pass`: Runs the stock four-second lazy/busy cycle at 1.4 m/s, testing repeated state transitions; at 18.5 seconds it requires at least two busy entries, `NAV_SPEED>0.4`, `ABE_IVPHELM_STATE=DRIVE`, no behavior error, and no deadline.
- `lazy_wait_pass`: Samples the stock initial lazy window before its first four-second transition, requiring zero busy entries, positive `PS_PENDING_ACTIVE`, zero `PS_PENDING_INACTIVE`, `NAV_SPEED<0.3`, a `DRIVE` helm, no behavior error, and no deadline.
- `first_busy_pass`: Samples the stock first busy window after the four-second transition, requiring exactly one busy entry, positive `PS_PENDING_INACTIVE`, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.
- `busy_to_lazy_pass`: Starts busy with `reset_upon_running=false`, a two-second busy period, and a six-second lazy period, testing the initial busy-to-lazy transition; at 5.5 seconds it requires zero counted lazy-to-busy transitions, at least three seconds pending active, zero pending inactive, a `DRIVE` helm, no behavior error, and no deadline.
- `deprecated_zaic_aliases_pass`: Supplies the deprecated `period_basewidth`, `zaic_peakwidth`, and `zaic_summit_delta` spellings, testing that all three are accepted and produce a usable 1.5 m/s busy objective; the 6.5-second sample requires one busy entry, positive pending-inactive time, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.
- `short_cycle_three_count_pass`: Sets both periods to 1.5 seconds, testing repeated short-cycle bookkeeping; at 12.5 seconds it requires at least three busy entries, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.
- `explicit_initially_lazy_pass`: Explicitly sets `initially_busy=false`, testing acceptance of the Boolean false branch and the resulting lazy start; at 3.5 seconds it requires zero busy entries, positive pending-active time, zero pending-inactive time, `NAV_SPEED<0.3`, a `DRIVE` helm, no behavior error, and no deadline.
- `zaic_base_period_peak_aliases_pass`: Supplies the deprecated `zaic_basewidth` and `period_peakwidth` spellings with preferred `summit_delta`, testing acceptance of the mixed alias set and a usable 1.5 m/s busy objective; the 6.5-second sample requires one busy entry, positive pending-inactive time, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.
- `reset_on_reactivation_pass`: Disables the default-reset behavior at one second and re-enables it at 5.5 seconds, testing that reactivation starts a fresh lazy period; at 7.2 seconds it requires zero busy entries, more than one second pending active, zero pending inactive, `NAV_SPEED<0.3`, a `DRIVE` helm, no behavior error, and no deadline.
- `reset_false_continuous_pass`: Sets `reset_upon_running=false`, disables the behavior from three through 12 seconds, and tests that its eight-second lazy/ten-second busy clock continues while inactive; at 15 seconds it requires at least one busy entry, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.
- `initially_busy_pass`: Sets `initially_busy=true` with `reset_upon_running=false`, testing the nondefault busy start; at 3.5 seconds it requires zero later busy entries, positive pending-inactive time, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.
- `zero_speed_pass`: Starts busy with `period_speed=0`, testing that a valid zero-speed objective commands a stop rather than causing configuration rejection; at 3.5 seconds it requires zero later busy entries, positive pending-inactive time, `NAV_SPEED<0.3`, a `DRIVE` helm, no behavior error, and no deadline.
- `bad_period_lazy_fail`: Sets `period_lazy=0`, testing rejection of a nonpositive lazy period; it passes only when Abe's helm reaches exact `MALCONFIG` after the two-second arm barrier and before the 15-second deadline.
- `bad_period_lazy_nonnumeric_fail`: Sets `period_lazy=soon`, testing rejection of a nonnumeric lazy period; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_period_busy_fail`: Sets `period_busy=0`, testing rejection of a nonpositive busy period; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_period_speed_fail`: Sets `period_speed=-1`, testing rejection of a negative target speed while preserving zero as valid; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_initially_busy_fail`: Sets `initially_busy=maybe`, testing rejection outside the Boolean forms; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_reset_upon_running_fail`: Sets `reset_upon_running=maybe`, testing rejection outside the Boolean forms; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_basewidth_fail`: Sets preferred `basewidth=-0.1`, testing rejection of a negative ZAIC base width; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_zaic_basewidth_fail`: Sets deprecated `zaic_basewidth=-0.1`, testing the alias's equivalent negative-width rejection; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_peakwidth_fail`: Sets preferred `peakwidth=-0.1`, testing rejection of a negative ZAIC peak width; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_period_peakwidth_fail`: Sets deprecated `period_peakwidth=-0.1`, testing the alias's equivalent negative-width rejection; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_summit_delta_fail`: Sets preferred `summit_delta=-1`, testing rejection of a negative ZAIC summit delta; it passes only on the same exact armed `MALCONFIG` state before the deadline.
- `bad_zaic_summit_delta_fail`: Sets deprecated `zaic_summit_delta=-1`, testing the alias's equivalent negative-delta rejection; it passes only on the same exact armed `MALCONFIG` state before the deadline.

## Manual Inspection

- `reset_false_visual_pass`: Runs a manual-viewer version of the inactive-clock test with 14-second lazy and 16-second busy periods, disabling the behavior from five through 20 seconds; at 27 seconds it requires at least one busy entry, `NAV_SPEED>0.4`, a `DRIVE` helm, no behavior error, and no deadline.

## Manual Commands

```bash
./zlaunch.sh --case=baseline_cycle_pass --port_base=15000 10
./zlaunch.sh --case=initially_busy_pass --gui --port_base=15000 10
./zlaunch.sh --case=reset_false_visual_pass --gui --port_base=15000 1
./zlaunch.sh --jobs=4 --port_base=34000 --port_stride=12 10
```

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
