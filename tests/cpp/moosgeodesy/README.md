# MOOSGeodesy C++ Tests

This family specifies the behavior of `CMOOSGeodesy`, including the behavior
that a future PROJ-backed implementation must preserve. It is deliberately a
library-level test family: it exercises coordinate conversion directly and
does not launch a MOOS mission.

Run the active family with:

```bash
./tests/cpp/ctests.sh --moosgeodesy
```

The family currently has active coverage for initialization, ordinary UTM and
local-grid conversions, round trips, representative global locations, value
semantics, utilities, and invalid local-grid inversions. Four additional
GoogleTest cases are disabled migration contracts. Disabled means they state
required replacement behavior that the current handwritten implementation
does not yet satisfy; it does not mean the behavior is optional.

## PROJ Migration Contracts

The contracts live in the `MOOSGeodesyMigrationContractTest` suite in
`Core/MOOSGeodesyTest.cpp`.

### `DISABLED_RemainsContinuousAcrossAdjacentUTMZones`

A local coordinate frame must retain the projection selected from its datum.
The current implementation calls `LLtoUTM` independently for every point, so
the point's longitude can select a different UTM zone from the origin. It then
subtracts eastings belonging to different zones, creating a zone-width jump.

The PROJ-backed implementation must construct and retain the datum's fixed UTM
projection during successful initialization. A point just across an adjacent
zone boundary must consequently remain only its true local distance from the
origin.

### `DISABLED_RemainsContinuousAcrossAntimeridian`

Longitude wrapping at `-180/180` must represent the short path across the
antimeridian. The current per-point UTM selection puts the origin and projected
point in zones 60 and 1 and makes their easting difference discontinuous.

The PROJ-backed implementation must use the datum's retained projection and
normalize or unwrap longitude consistently so a short antimeridian crossing
stays short. The inverse conversion must return the equivalent wrapped global
longitude.

### `DISABLED_RejectsInvalidInitializationWithoutChangingDatum`

Initialization must reject non-finite coordinates, longitude outside
`[-180, 180]`, and latitude outside the supported UTM domain of `[-80, 84]`.
Validation and projection construction must happen before changing observable
object state. If reinitialization fails, the last valid origin, projection,
UTM zone, and conversion state must remain usable.

The current implementation writes the origin, calls the handwritten converter,
and returns `true` unconditionally, so it cannot provide this transactional
failure behavior.

### `DISABLED_RejectsNonFiniteProjectionWithoutChangingOutputs`

A forward conversion with a non-finite input must return `false`. It must not
replace caller-owned output arguments and must not replace the object's last
valid northing/easting state.

The current implementation performs the conversion, updates internal and
caller-visible values, and returns `true` without validating the input or the
projection result.

## Enabling The Contracts

Run the disabled suite explicitly while developing the replacement:

```bash
./bin/test_moosgeodesy_core \
  --gtest_also_run_disabled_tests \
  --gtest_filter='MOOSGeodesyMigrationContractTest.*'
```

Before enabling any contract:

1. Build the PROJ projection or transformation in temporary state during
   initialization and commit it to the object only after every validation and
   PROJ operation succeeds.
2. Retain the datum-selected projection for subsequent forward and inverse
   conversions instead of selecting a UTM zone independently for each point.
3. Define antimeridian normalization consistently for forward and inverse
   conversions.
4. Check every input and PROJ result for finiteness and failure before changing
   output arguments or the object's last valid conversion state.
5. Preserve copy construction and copy assignment behavior; the active value
   semantics tests must continue to pass after projection state is added.
6. Run the disabled migration suite directly, then run
   `./tests/cpp/ctests.sh --moosgeodesy`.
7. Remove the `DISABLED_` prefix only from contracts that now pass. Once
   enabled, verify that CTest discovers them as active cases and that the
   `MigrationContract` suite label still applies.

Do not weaken seam tolerances or change the expected short-distance direction
merely to accommodate zone reselection. Those assertions are the acceptance
boundary for replacing the handwritten projection behavior.
