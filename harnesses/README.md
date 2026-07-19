# Harness Execution Policy

Every harness invocation creates a unique run root beneath its harness
directory, and every selected case runs from its own copied mission directory
and assigned port block. Harness launchers do not use a filesystem lock to
serialize separate invocations.

Callers are responsible for choosing non-overlapping MOOSDB and pShare port
ranges. Do not run the same harness directory concurrently: invocations share
the top-level `results.txt`, so their aggregate output can race even when their
mission directories and ports are distinct. Performance harnesses also should
not overlap other load that would invalidate their wall-clock gates.

Cleanup is scoped to the invocation run root. Once cleanup begins, launchers
ignore further `INT`, `TERM`, and `PIPE` signals so repeated interrupts
cannot stop teardown. A cleanup failure makes the run fail and preserves its
run root for diagnosis. Do not use global `ktm`, `pkill`, or machine-wide
cleanup as the normal harness path.

## Shared Mission-Utility Runner

The three launchers under `mission_utility_harnesses/H01-*`, `H02-*`, and
`H03-*` are intentionally thin wrappers. Each declares its own case list and
then sources `mission_utility_harnesses/mission_utility_common.sh`, which owns
their shared argument parsing, scheduling, result aggregation, and cleanup
against the common mission-utility stem.

This avoids maintaining three copies of the same large runner, but it also
means a change to `mission_utility_common.sh` changes all three harnesses.
Validate all three wrappers whenever that shared file changes.
