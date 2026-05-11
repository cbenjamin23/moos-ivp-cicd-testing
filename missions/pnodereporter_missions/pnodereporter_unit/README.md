# pnodereporter_unit

Headless single-community mission for app-level `pNodeReporter` checks. The stem
runs `pNodeReporter` with scripted navigation, helm, platform, and rider mail,
then evaluates whether the expected report stream is produced.

The mission-owned grade confirms report publication through `pMissionEval`.
The harness supplements that grade with narrow token checks against the generated
report payload because `NODE_REPORT_LOCAL` contains dynamic timestamp fields.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=30 --nogui 10
```
