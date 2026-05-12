# pechovar_unit

Headless single-community stem for exercising live `pEchoVar` runtime behavior.
The harness writes case-specific echo/flip mappings, scripted source mail, and
evaluation rules before each isolated launch.

The case matrix covers direct string and numeric echoing, boolean inversion,
one-source-to-many-target output, latest-only pruning, EFlipper component
mapping, filters, custom separators, cycle suppression, and invalid flipper
suppression.

Typical harness run:

```sh
../../../harnesses/pechovar_harnesses/H01-pechovar_unit/zlaunch.sh --jobs=4 --port_base=17600 10
```

Run one inspectable case:

```sh
../../../harnesses/pechovar_harnesses/H01-pechovar_unit/zlaunch.sh --case=flip_filter_suppresses_pass --port_base=17600 10
```
