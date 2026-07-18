# nspatch Notes

The stem mission owns the default `fixed_field_publish_pass` case. Each of the
other 48 overlays replaces only the blocks needed for that case, including its
substantive `pMissionEval` contract.

The harness applies exactly one selected overlay:

```sh
nspatch --stem="$workdir/meta_shoreside.moos" \
  "$CASE_SHORE_PATCH" --targ="$workdir/meta_shoreside.moosx"
```

The resulting `.moosx` is then expanded with `nsplug -x`. The fixed baseline
has no overlay and expands the copied `meta_shoreside.moos` directly.

Every generated case must contain exactly one `pMissionEval`, the selected
`MMOD`, its intended timer and `uFldObstacleSim` settings, and no shell-owned
grading contract. Overlays that replace `uTimerScript` retain the common
`block_on` startup barrier so early publications cannot escape the evaluator.

Structured expected payloads are not embedded as `pMissionEval` literals, even
when quoted, because relation characters such as `=` are part of the condition
grammar. An overlay needing exact equality posts its expected string before the
subject input and uses the right-variable form `$ (UFOS_EXPECT_*)` on the
condition RHS. Field- or history-specific assertions use narrowly filtered
stock `pEchoVar` destinations checked as plain values or nonempty presence
sentinels, except for the exact inactive-record composite compared through a
runtime expected variable. No structured payload is embedded as a condition
literal, and no custom grading application is involved.

The `block_on` barrier protects mission-time ordering. The harness independently
bounds each stem invocation at `--max_time + 10` real seconds so a startup that
never clears cannot wait forever. This conservative wall ceiling is separate
from `uMayFinish`'s warped `DB_UPTIME` interpretation of `--max_time`.

There is no second evaluator patch or intermediate mission file. That keeps
scenario and grading changes together and avoids competing replacements of the
same `pMissionEval` block.
