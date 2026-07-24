# testfailure_behavior_unit

Single-community instrumentation mission for `BHV_TestFailure`.

The mission runs `pHelmIvP` with a tiny `BHV_TestFailure` behavior and grades
the outcome with `pMissionEval`. Normal behavior/motion evidence is not the
point of this family: the expected signals are process health from
`uProcessWatch`, direct `pHelmIvP` iteration-gap evidence, and behavior
completion endflags.

## Evaluation

The default stem keeps `BHV_TestFailure` armed but gives it a long duration so
the mission can prove no false failure occurs before evaluation. Harness cases
patch the behavior to trigger crash, burn, hang, or immediate-completion paths.
Crash cases remain self-evaluating missions: process loss is the subject
behavior, while pMissionEval writes a passing harness verdict only after
`uProcessWatch` confirms that loss.

The burn/hang patches additionally wait for the first ordinary
`PHELMIVP_ITER_GAP` sample before activating, so their completion stall occurs
while pHelmIvP load reporting is already live. `uLoadWatch` caches every
iteration-gap message and compares the maximum against broad two- or
three-second bands scaled by the selected time warp. An auxiliary evaluator
retains the exact transient sample when scheduler timing permits; it is
diagnostic rather than required evidence. `uGapDeadline` supplies a short
post-completion absence deadline so a missing stall becomes a normal mission
failure.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=150 10
```

Logging is minimal by default and does not launch `pLogger`. Add
`--log=full` to either launcher to restore the original shoreside logger.
