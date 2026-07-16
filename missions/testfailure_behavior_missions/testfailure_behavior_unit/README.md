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

## Running

```bash
./zlaunch.sh
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=150 10
```
