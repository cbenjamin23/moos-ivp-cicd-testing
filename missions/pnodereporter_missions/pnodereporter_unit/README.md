# pnodereporter_unit

Headless single-community mission for app-level `pNodeReporter` checks. The stem
runs `pNodeReporter` with scripted navigation, helm, platform, and rider mail,
then evaluates whether the expected report stream is produced.

The mission-owned grade confirms report publication through `pMissionEval`.
The harness supplements that grade with narrow token checks against the
generated report payload because exact report representation is the test
subject and `NODE_REPORT_LOCAL` contains dynamic timestamp fields.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=35 --nogui 10
```

The automation wrapper forwards `--mmod`, the one-community MOOSDB port, the
reserved shoreside pShare port, time warp, display mode, and `--max_time` to
`xlaunch.sh`. It validates target generation under `--just_make`, requires
exactly one valid pMissionEval result row during live runs, and uses the
repository's canonical root-scoped teardown helper.

Logging is minimal by default. It retains only asynchronous
`NODE_REPORT_LOCAL` evidence needed by the history-sensitive harness cases.
Use `--log=full` with `launch.sh` or `zlaunch.sh` to restore the complete
pre-migration explicit logger configuration.
