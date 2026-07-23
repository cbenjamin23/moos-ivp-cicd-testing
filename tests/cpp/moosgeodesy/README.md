# MOOSGeodesy C++ Tests

This family specifies the behavior of `CMOOSGeodesy`, including the behavior
that a future PROJ-backed implementation must preserve. It is deliberately a
library-level test family: it exercises coordinate conversion directly and
does not launch a MOOS mission.

Run the active family with:

```bash
./tests/cpp/ctests.sh --moosgeodesy
```

The family has active coverage for initialization, ordinary UTM and local-grid
conversions, round trips, representative global locations, value semantics,
utilities, invalid local-grid inversions, and the PROJ migration contracts.
The migration tests are discovered normally: they run with the PROJ backend
and report as skipped with the intentionally retained handwritten backend.

## PROJ Migration Contracts

The contracts live in the `MOOSGeodesyMigrationContractTest` suite in
`Core/MOOSGeodesyTest.cpp`.

### `RemainsContinuousAcrossAdjacentUTMZones`

A local coordinate frame must retain the projection selected from its datum.
The current implementation calls `LLtoUTM` independently for every point, so
the point's longitude can select a different UTM zone from the origin. It then
subtracts eastings belonging to different zones, creating a zone-width jump.

The PROJ-backed implementation must construct and retain the datum's fixed UTM
projection during successful initialization. A point just across an adjacent
zone boundary must consequently remain only its true local distance from the
origin.

### `RemainsContinuousAcrossAntimeridian`

Longitude wrapping at `-180/180` must represent the short path across the
antimeridian. The current per-point UTM selection puts the origin and projected
point in zones 60 and 1 and makes their easting difference discontinuous.

The PROJ-backed implementation must use the datum's retained projection and
normalize or unwrap longitude consistently so a short antimeridian crossing
stays short. The inverse conversion must return the equivalent wrapped global
longitude.

### `RejectsInvalidInitializationWithoutChangingDatum`

Initialization must reject non-finite coordinates, longitude outside
`[-180, 180]`, and latitude outside the supported UTM domain of `[-80, 84]`.
Validation and projection construction must happen before changing observable
object state. If reinitialization fails, the last valid origin, projection,
UTM zone, and conversion state must remain usable.

The current implementation writes the origin, calls the handwritten converter,
and returns `true` unconditionally, so it cannot provide this transactional
failure behavior.

### `RejectsNonFiniteProjectionWithoutChangingState`

A forward conversion with a non-finite input must return `false`, set its
caller-owned output arguments to NaN, and preserve the object's last valid
northing/easting state. Deterministic invalid outputs prevent callers that
neglect the return value from consuming stale or uninitialized coordinates.

The current implementation performs the conversion, updates internal and
caller-visible values, and returns `true` without validating the input or the
projection result.

## Running The Contracts

Run the active migration suite directly with:

```bash
./bin/test_moosgeodesy_core \
  --gtest_filter='MOOSGeodesyMigrationContractTest.*'
```

The contracts require the PROJ backend. With the handwritten backend they are
reported as skipped because that implementation is retained specifically for
legacy compatibility and is known not to provide fixed-zone or transactional
validation behavior.

The replacement implementation must:

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
6. Run the migration suite directly, then run
   `./tests/cpp/ctests.sh --moosgeodesy`.

Do not weaken seam tolerances or change the expected short-distance direction
merely to accommodate zone reselection. Those assertions are the acceptance
boundary for replacing the handwritten projection behavior.
