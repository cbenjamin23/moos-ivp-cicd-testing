# C++ Tests

## TLDR

This directory contains fast C++ tests for MOOS-IvP libraries and helpers. They
do not launch MOOS missions. Use them when you want to check a parser, geometry
object, math helper, app utility, or other C++ component directly.

The setup has two parts:

- GoogleTest is what you write in `*Test.cpp`: `TEST(SuiteName, CaseName)`.
- CTest is what you run from the build tree. CI uses CTest family selectors to
  run all tests, one component family, or a batch of component families.

To add tests, put the test file under `tests/cpp/<library>/<area>`, register it
with `add_moos_cpp_test` in that area's `CMakeLists.txt`, and give it the
right family label.

Run a focused family:

```bash
ctest --test-dir build -L geometry --output-on-failure
```

Run one GoogleTest suite while editing:

```bash
./bin/test_geometry_arcutils --gtest_filter='ArcUtilsIntersectionTest.*'
```

The sections below explain the structure and conventions in more detail. The
suite is designed for targeted regression work. A developer should be able to
run:

- all C++ tests with `ctest --test-dir build --output-on-failure`
- one component family with a label, such as `ctest --test-dir build -L geometry`
- one executable directly, such as `./bin/test_geometry_arcutils`
- one GoogleTest suite with an executable filter, such as
  `./bin/test_geometry_arcutils --gtest_filter='ArcUtilsIntersectionTest.*'`

## Mental Model

Each test target has three layers:

- CMake target: one executable, created with `add_moos_cpp_test`
- GoogleTest suites: source-level groups such as `XYPointTest` or
  `ZAICPeakTest`
- CTest cases: one registered test per GoogleTest test case, with labels

CTest is the main runner because it understands labels and is what CI uses.
GoogleTest is the assertion framework inside each executable.

## CTest And GoogleTest

GoogleTest is where tests are written. A test source usually contains cases
like this:

```cpp
TEST(XYPointTest, ParsesViewerPointWithLabelAndColor)
{
  XYPoint point = string2Point("x=10,y=20,label=alpha,vertex_color=red");

  EXPECT_TRUE(point.valid());
  EXPECT_DOUBLE_EQ(point.x(), 10);
  EXPECT_DOUBLE_EQ(point.y(), 20);
  EXPECT_EQ(point.get_label(), "alpha");
}
```

The first argument, `XYPointTest`, is the GoogleTest suite. The second argument
is the case name. Together they form the GoogleTest filter name:

```text
XYPointTest.ParsesViewerPointWithLabelAndColor
```

CTest is the runner around those GoogleTest cases. CMake discovers each
GoogleTest case and registers it as a separate CTest case with the executable
name prepended:

```text
test_geometry_xyformatutils_point.XYPointTest.ParsesViewerPointWithLabelAndColor
```

That means:

- use CTest when you want CI-equivalent behavior, labels, or broad subsets
- use `--gtest_filter` on the executable when iterating on one suite or case
- use CTest family labels when the boundary is a source-level component group

## Run Tests

Build and run everything:

```bash
./build.sh
ctest --test-dir build --output-on-failure
```

If the build cache points at the wrong MOOS-IvP checkout, reconfigure once:

```bash
cmake -S . -B build -DMOOSIVP_SOURCE_TREE_BASE=/path/to/moos-ivp
cmake --build build
ctest --test-dir build --output-on-failure
```

Run a focused CTest family:

```bash
ctest --test-dir build -L geometry --output-on-failure
ctest --test-dir build -L ivpbuild --output-on-failure
ctest --test-dir build -L mbutil --output-on-failure
```

CTest label matching is regex-based. Anchor broad labels when needed:

```bash
ctest --test-dir build -L '^behaviors$' --output-on-failure
```

Run one executable directly:

```bash
./bin/test_geometry_arcutils
./bin/test_ivpbuild_zaic
```

Run one GoogleTest suite or case directly:

```bash
./bin/test_geometry_arcutils --gtest_filter='ArcUtilsIntersectionTest.*'
./bin/test_ivpbuild_zaic --gtest_filter='ZAICPeakTest.CoursePeakWrapsAcrossNorthForHelmHeadingDomains'
```

List labels and discovered tests:

```bash
ctest --test-dir build --print-labels
ctest --test-dir build -N
```

## Layout

Use this structure:

```text
tests/cpp/<library>/CMakeLists.txt
tests/cpp/<library>/<area>/CMakeLists.txt
tests/cpp/<library>/<area>/*Test.cpp
tests/cpp/support/*
```

Examples:

```text
tests/cpp/geometry/XYFormatUtilsPoint/XYFormatUtilsPointTest.cpp
tests/cpp/ivpbuild/ZAIC/ZAICTest.cpp
tests/cpp/mbutil/CoreString/MBUtilsTest.cpp
```

The top-level library directory should match the upstream library or practical
repo bucket. The `<area>` directory should group a coherent component family,
not a random collection of unrelated tests.

Shared helpers live in `tests/cpp/support` and are automatically included for
every target created with `add_moos_cpp_test`.

## CMake Pattern

Every leaf test target should use `add_moos_cpp_test`. It is this repo's CMake
wrapper for a C++ test executable. It creates the executable, links GoogleTest,
adds shared test includes, asks CMake to discover the GoogleTest cases, and
attaches CTest labels/environment settings.

Use it instead of hand-writing `add_executable`, `target_link_libraries`, and
`gtest_discover_tests` for normal C++ tests:

```cmake
add_moos_cpp_test(test_geometry_xyformatutils_point
  SOURCES
    XYFormatUtilsPointTest.cpp
  LIBS
    geometry
    mbutil
  LABELS
    geometry
    formats
  SUITE_LABELS
    XYPointTest=XYPoint
    XYFormatUtilsPointTest=XYFormatUtilsPoint
)
```

Supported arguments:

- `SOURCES`: test files and any extra app/library source files
- `LIBS`: libraries to link
- `INCLUDES`: target-specific include directories
- `DEFINES`: target-specific compile definitions
- `LABELS`: CTest labels applied to every discovered case in the executable
- `SUITE_LABELS`: labels applied only to matching GoogleTest suites, written as
  `SuiteName=LabelA,LabelB`
- `WORKING_DIRECTORY`: optional CTest working directory
- `DISCOVERY_MODE`: optional GoogleTest discovery mode, such as `PRE_TEST`
- `ENVIRONMENT`: optional CTest environment entries such as `NAME=value`

Keep target setup inside `add_moos_cpp_test` when possible. Avoid follow-up
`target_link_libraries`, `target_include_directories`, or
`set_tests_properties` blocks unless there is a clear local reason.

For external MOOS-IvP dependencies, use CMake discovery. Prefer imported
targets or variables from `find_package`; otherwise use `find_library` and
`find_path`. Do not link directly to hardcoded `.dylib`, `.so`, or platform
build-output paths from test `CMakeLists.txt` files.

## Labels

Top-level family labels are the public CTest interface for developers and CI.
Treat them as carefully as test names.

Use target-wide `LABELS` only when the label truthfully applies to every CTest
case in the executable. Every target should include exactly one top-level family
label such as:

- `geometry`
- `mbutil`
- `ivpbuild`
- `behaviors_marine`
- `apputil`

Additional labels may be useful as CTest registry metadata, but they should not
be treated as workflow selectors. Use `SUITE_LABELS` when one executable contains
multiple GoogleTest suites that need suite-specific labels for reporting.
Use `SUITE_LABELS` for concrete component labels in mixed executables:

```cmake
LABELS
  geometry
  formats
SUITE_LABELS
  XYRangePulseTest=XYRangePulse
  XYFormatUtilsRangePulseTest=XYFormatUtilsRangePulse
  XYCommsPulseTest=XYCommsPulse
  XYFormatUtilsCommsPulseTest=XYFormatUtilsCommsPulse
```

Rules:

- every CTest case must have at least one useful label
- no duplicate labels on a case
- concrete class/tool labels should not spill across unrelated suites
- family labels may be broad when the whole executable is the family
- app-impact labels mean the test protects behavior used by that app; they do
  not mean the app itself is launched
- prefer canonical MOOS-IvP names, including existing capitalization such as
  `XYPolygon`, `ZAIC_PEAK`, `BHV_Waypoint`, or `VIEW_POLYGON`

## Test Quality Standard

The goal is not a smoke test per class. Each suite should be deep enough that a
changed MOOS-IvP checkout produces a specific, actionable failure.

Tests are expected to pass on both the local macOS development build and the
Linux GitHub Actions build. Treat the Linux `--nogui` build as a first-class
target, not a reduced mode where failures can be ignored.

For parsers and format utilities, cover:

- canonical MOOS-IvP payloads, such as `VIEW_POINT`, `VIEW_POLYGON`,
  `VIEW_SEGLIST`, `VIEW_GRID`, `NODE_REPORT`, and behavior config strings
- abbreviated and standard forms when both are supported
- labels, colors, render hints, source/type metadata, duration, active flags,
  and other viewer-facing fields
- malformed input, empty input, duplicate fields, unknown fields, numeric
  quirks, and tolerated legacy behavior
- serialization, spec output, or round-trip behavior when stable

For geometry and math tools, cover:

- nominal cases plus boundaries
- degenerate inputs: empty objects, one-point objects, zero radius, zero
  length, zero speed, zero duration, invalid domains
- wraparound headings and angle intervals across north
- crossing and non-crossing cases
- distance/value outputs as well as boolean classifiers
- ownership/copy/cache behavior when objects maintain internal state
- MOOS-IvP use cases such as waypoint lanes, loiter regions, OpRegion
  polygons, obstacle polygons, turn arcs, contact CPA, Dubins lookahead,
  search grids, viewer shapes, and IvP course/speed domains

For app-facing helpers, cover the payloads and state transitions used by the
app, but keep the test local unless the target is a mission harness.

## Portability Standard

Keep C++ tests portable across macOS and Linux:

- include the standard headers used by the test directly, such as `<cmath>` for
  math helpers, even if a platform happens to compile through transitive
  includes
- do not assume Apple linker behavior; order static libraries so dependents
  come before the libraries that satisfy their symbols
- do not require GUI-only MOOS-IvP libraries in normal library tests because
  CI builds MOOS-IvP with `--nogui`
- if a test needs logic from a GUI-adjacent library, prefer compiling the small
  non-GUI source files needed by the test over linking an unavailable GUI
  archive
- assert stable contracts, not incidental parser fallback colors, exception
  text, allocation side effects, or other platform-dependent details
- avoid malformed-input tests that can crash inside upstream MOOS-IvP code; if
  the crash itself is important, keep it outside the normal CTest path and tie
  it to an explicit upstream bug or decision

Platform guards should be rare and narrow. Prefer guarding one unstable
assertion over skipping an entire test case, and include a comment explaining
the upstream behavior that makes the guard necessary. A guarded or skipped test
should be treated as temporary technical debt unless it documents a deliberate
compatibility boundary.

## Naming

Use suite names that identify the unit under test:

```cpp
TEST(XYConvexGridTest, ParsesViewGridSpecWithCellUpdatesAndLimits)
TEST(ZAICPeakTest, CoursePeakWrapsAcrossNorthForHelmHeadingDomains)
```

Good test case names state the condition and expected behavior. Prefer:

```text
ToolOrObjectTest.ContextAndExpectedBehavior
```

Avoid names like `Basic`, `Works`, `Test1`, or names that only restate the API
call. A failure should tell the reader what contract was broken.

## Assertions

Use direct, narrow assertions. Prefer:

- exact string checks for stable serialized specs
- tolerance checks for floating-point geometry/math values
- explicit false-path checks for malformed input
- separate assertions for independent behavior when a failure needs a precise
  location

When testing a known upstream quirk, name it clearly in the test case. These
tests intentionally protect current behavior until the project decides to
change it.

## MOOS-IvP Grounding

Tests should include abstract edge cases when they improve coverage, but each
library family should also include examples grounded in actual MOOS-IvP usage.

Good grounded inputs include:

- viewer postings: `VIEW_POINT`, `VIEW_POLYGON`, `VIEW_SEGLIST`, `VIEW_GRID`,
  `VIEW_RANGE_PULSE`, `VIEW_COMMS_PULSE`
- contact reports: `NODE_REPORT`, `CONTACT_INFO`, CPA-related geometry
- behavior configs: waypoint, loiter, op-region, obstacle, cut-range, shadow,
  and avoid-collision style parameters
- IvP domains and functions: course/speed domains, ZAICs, AOFs, PDMaps, IPF
  strings, and function encoder packets
- log/app utilities: `.alog`-style lines, helm summaries, appcast/realmcast
  strings, command posts, and mission-eval conditions

Do not require a launched mission just to test a pure library contract. Use a
mission harness only when the behavior depends on live MOOS process interaction.

## Adding Coverage

Before adding a new test family:

1. Find the upstream library or app source under `moos-ivp/ivp/src`.
2. Check nearby tests for the closest pattern.
3. Choose the smallest coherent `<area>` bucket.
4. Add or extend a leaf `CMakeLists.txt` with precise `LABELS` and
   `SUITE_LABELS`.
5. Add focused GoogleTest suites with grounded and edge-case examples.
6. Run the focused label first, then the invariant checker.

Useful local commands:

```bash
ctest --test-dir build -L XYFormatUtilsPoint --output-on-failure
ctest --test-dir build -L geometry --output-on-failure
python3 scripts/check_cpp_tests.py \
  --build-dir build \
  --moos-src /path/to/moos-ivp/ivp/src
```

Run the full suite before handing off broad changes:

```bash
ctest --test-dir build --output-on-failure
```

## Invariants

`scripts/check_cpp_tests.py` enforces repo-level C++ test invariants. It checks
registration, labels, source-file references, stale discovery placeholders,
platform-specific link literals, and optional upstream `lib_*` to `tests/cpp`
bucket mapping.

The GitHub Actions workflow uses the same CTest registry and checker. Its C++
dispatch inputs mirror the harness inputs:

- `cpp_test_mode=none`: run no C++ unit tests
- `cpp_test_mode=all`: run the full C++ suite
- `cpp_test_mode=family_run`: run one family from the `cpp_test_family`
  dropdown, such as `geometry`, `ivpbuild`, or `mbutil`
- `cpp_test_mode=batch_family_run`: run comma-separated families from
  `cpp_test_families`, such as `geometry,ivpbuild,mbutil`

At least one harness target or one CTest family must be selected. Use
`dispatch_mode=none` for C++-only runs, or `cpp_test_mode=none` for
harness-only runs.

The workflow runs selected C++ coverage areas as separate `ctest / <family>`
matrix jobs after the shared build job. Each C++ job writes a unit test table
to the GitHub Actions step summary. If a target fails, the summary includes the
failing CTest case names and the per-target JUnit XML report is uploaded as a
`cpp-unit-test-report-<family>` artifact.

If a new convention cannot be expressed in this README and enforced or audited
reasonably, reconsider the convention.
