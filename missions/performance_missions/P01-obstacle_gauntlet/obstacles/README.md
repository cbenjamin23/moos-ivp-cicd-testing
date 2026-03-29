# Obstacle Layouts

This folder stores the checked-in fixed obstacle layouts for `P01-obstacle_gauntlet`.

- `baseline_field.txt`: default repeatable fixed field
- `dense_field.txt`: tighter fixed field with more central clutter

At launch time, `init_field.sh` copies the selected fixed layout into the mission root as `obstacles.txt`, which is the file consumed by `uFldObstacleSim`.

Even though these layouts are hand-authored, they still include positive
`min_range`, `min_size`, and `max_size` metadata because `uFldObstacleSim`
expects those fields to be present when parsing an obstacle file.

The endurance mode does not use one of these files directly. It generates a fresh runtime `obstacles.txt` in the mission root with `gen_obstacles`.
