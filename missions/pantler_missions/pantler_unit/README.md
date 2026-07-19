# pAntler Unit Mission

This mission keeps pAntler launch composition small and directly observable.
Each case uses pAntler to start the apps that publish the graded flags.
The direct launchers accept `--log=minimal|full` and default to `minimal`;
both modes preserve the selected case's ANTLER topology because that topology
is the mission subject.

Typical commands:

```sh
./launch.sh --just_make --nogui 10
./zlaunch.sh --max_time=20 --nogui 10
./zlaunch.sh --log=full --max_time=20 --nogui 10
```

`zlaunch.sh` forwards explicit shoreside ports and mission labels to the shared
xlaunch lifecycle, validates the single pMissionEval result row, and applies
repository-scoped cleanup as a standalone backstop.
