# Joust Layouts

These checked-in layouts are the deterministic route seeds used by
`P02-colregs_joust`.

- `baseline_2v.txt`
  - mild two-vehicle crossing
  - intended to be the stable entry-level performance baseline

- `dense_3v.txt`
  - denser three-vehicle continuous joust
  - higher contact pressure than the baseline without the instability of the
    copied four-vehicle swarm layout

- `endurance_3v.txt`
  - three-vehicle longer-running joust
  - milder than the dense case so it can run for a longer evaluation window

At launch time, `launch.sh` selects one of these files from `--mmod` and copies
it into the mission root as `joust.txt`. The launch flow then seeds each
vehicle's `BHV_LegRun` from that chosen file.
