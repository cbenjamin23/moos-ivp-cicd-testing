# C++ Unit Tests

This tree contains MOOS-IvP library-level tests that do not launch missions.
They are built with the repo through CMake, registered with CTest, and written
with GoogleTest.

## Run Everything

```bash
./build.sh
ctest --test-dir build --output-on-failure
```

If the local build cache has an old `MOOSIVP_SOURCE_TREE_BASE`, reconfigure once:

```bash
cmake -S . -B build -DMOOSIVP_SOURCE_TREE_BASE=/path/to/moos-ivp
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run One Tool

Each suite has CTest labels for the library area and the specific tool. Examples:

```bash
ctest --test-dir build -L ArcUtils --output-on-failure
ctest --test-dir build -L ArcUtilsToDo --output-on-failure
ctest --test-dir build -L XYArc --output-on-failure
ctest --test-dir build -L CircularUtils --output-on-failure
ctest --test-dir build -L XYPrimitiveShapes --output-on-failure
ctest --test-dir build -L Dubins --output-on-failure
ctest --test-dir build -L TrackProximity --output-on-failure
ctest --test-dir build -L GeomUtils --output-on-failure
ctest --test-dir build -L XYFormatUtilsCircle --output-on-failure
ctest --test-dir build -L XYFormatUtilsVector --output-on-failure
ctest --test-dir build -L XYFormatUtilsMarker --output-on-failure
ctest --test-dir build -L XYFormatUtilsPulse --output-on-failure
ctest --test-dir build -L XYFormatUtilsSeglr --output-on-failure
ctest --test-dir build -L XYFormatUtilsWedge --output-on-failure
ctest --test-dir build -L XYViewerShapes --output-on-failure
ctest --test-dir build -L XYConvexGrid --output-on-failure
ctest --test-dir build -L XYGridUpdate --output-on-failure
ctest --test-dir build -L PathUtils --output-on-failure
ctest --test-dir build -L CPAEngine --output-on-failure
ctest --test-dir build -L CPAPlatform --output-on-failure
ctest --test-dir build -L CPAVariants --output-on-failure
ctest --test-dir build -L FieldArtifacts --output-on-failure
ctest --test-dir build -L PolygonGeneration --output-on-failure
ctest --test-dir build -L ViewPlugSettings --output-on-failure
ctest --test-dir build -L MiscGeometryTools --output-on-failure
ctest --test-dir build -L IOAndPopulators --output-on-failure
ctest --test-dir build -L ObjectBaseBuild --output-on-failure
ctest --test-dir build -L GeoShapesLifecycle --output-on-failure
ctest --test-dir build -L ivpbuild --output-on-failure
ctest --test-dir build -L ZAIC --output-on-failure
ctest --test-dir build -L AOF --output-on-failure
ctest --test-dir build -L PDMapBuilder --output-on-failure
ctest --test-dir build -L FunctionEncoder --output-on-failure
ctest --test-dir build -L OF_Coupler --output-on-failure
ctest --test-dir build -L OF_Rater --output-on-failure
ctest --test-dir build -L bhvutil --output-on-failure
ctest --test-dir build -L ExFilterSet --output-on-failure
ctest --test-dir build -L FixedTurn --output-on-failure
ctest --test-dir build -L WaypointEngine --output-on-failure
ctest --test-dir build -L LoiterEngine --output-on-failure
ctest --test-dir build -L formats --output-on-failure
ctest --test-dir build -L mbutil --output-on-failure
ctest --test-dir build -L MBUtilsParsing --output-on-failure
ctest --test-dir build -L MBUtilsMisc --output-on-failure
ctest --test-dir build -L VarDataPair --output-on-failure
ctest --test-dir build -L NodeMessage --output-on-failure
ctest --test-dir build -L JsonUtils --output-on-failure
ctest --test-dir build -L StringTree --output-on-failure
ctest --test-dir build -L FColorMap --output-on-failure
ctest --test-dir build -L logic --output-on-failure
ctest --test-dir build -L LogicCondition --output-on-failure
ctest --test-dir build -L InfoBuffer --output-on-failure
ctest --test-dir build -L LedgerSnap --output-on-failure
ctest --test-dir build -L contacts --output-on-failure
ctest --test-dir build -L NodeRecord --output-on-failure
ctest --test-dir build -L NodeRider --output-on-failure
ctest --test-dir build -L evalutil --output-on-failure
ctest --test-dir build -L VCheck --output-on-failure
ctest --test-dir build -L LogicTestSequence --output-on-failure
ctest --test-dir build -L obstacles --output-on-failure
ctest --test-dir build -L ObstacleFieldGenerator --output-on-failure
ctest --test-dir build -L ucommand --output-on-failure
ctest --test-dir build -L CommandFolio --output-on-failure
```

You can also run one test executable directly:

```bash
./bin/test_geometry_arcutils
./bin/test_geometry_xyarc
./bin/test_geometry_circularutils
./bin/test_geometry_xyprimitive_shapes
./bin/test_geometry_dubins
./bin/test_geometry_trackproximity
./bin/test_geometry_xyformatutils_circle
./bin/test_geometry_xyformatutils_vector
./bin/test_geometry_xyformatutils_marker
./bin/test_geometry_xyformatutils_pulse
./bin/test_geometry_xyformatutils_seglr
./bin/test_geometry_xyformatutils_wedge
./bin/test_geometry_xyviewer_shapes
./bin/test_geometry_xyconvexgrid
./bin/test_geometry_xygrid_family
./bin/test_geometry_pathutils
./bin/test_geometry_cpaengine
./bin/test_geometry_cpaplatform
./bin/test_geometry_cpavariants
./bin/test_geometry_field_artifacts
./bin/test_geometry_polygon_generation
./bin/test_geometry_viewplug_settings
./bin/test_geometry_misc_geometry_tools
./bin/test_geometry_io_and_populators
./bin/test_geometry_objectbasebuild
./bin/test_geometry_geoshapes_lifecycle
./bin/test_ivpbuild_aof
./bin/test_ivpbuild_zaic
./bin/test_ivpbuild_pdmap_encoding
./bin/test_bhvutil_exfilterset
./bin/test_bhvutil_fixedturn
./bin/test_bhvutil_waypoint_engine
./bin/test_bhvutil_loiter_engine
./bin/test_mbutil_mbutils_parsing
./bin/test_mbutil_mbutils_tokens
./bin/test_mbutil_mbutils_validation
./bin/test_mbutil_mbutils_formatting
./bin/test_mbutil_mbutils_misc
./bin/test_mbutil_vardatapair
./bin/test_mbutil_nodemessage
./bin/test_mbutil_color
./bin/test_mbutil_macro_hash
./bin/test_mbutil_misc_objects
./bin/test_mbutil_jsonutils
./bin/test_mbutil_latlon_format_utils
./bin/test_mbutil_vquals
./bin/test_mbutil_stringtree
./bin/test_mbutil_fcolormap
./bin/test_mbutil_figlog
./bin/test_mbutil_scopeentry
./bin/test_mbutil_bundleout
./bin/test_mbutil_mbtimer
./bin/test_logic_conditions
./bin/test_logic_infobuffer
./bin/test_logic_conditional_ledger
./bin/test_contacts_noderecord
./bin/test_contacts_noderider
./bin/test_evalutil_vcheck
./bin/test_evalutil_logic_sequence
./bin/test_obstacles_obstacle
./bin/test_ucommand_command_folio
```

## Run In GitHub Actions

The `Build And Check Active Harnesses` workflow has a `cpp_test_filter` dispatch
input. Use `all` for the full C++ suite, or pass any CTest label such as
`ArcUtils`, `ArcUtilsToDo`, `XYArc`, `CircularUtils`, `XYPrimitiveShapes`,
`Dubins`, `TrackProximity`, `XYFormatUtilsPoly`, `XYFormatUtilsVector`,
`XYFormatUtilsMarker`, `XYFormatUtilsPulse`, `XYFormatUtilsSeglr`,
`XYFormatUtilsWedge`, `XYViewerShapes`, `XYGridUpdate`, `PathUtils`,
`XYConvexGrid`, `CPAEngine`, `CPAPlatform`, `CPAVariants`,
`FieldArtifacts`, `PolygonGeneration`, `ViewPlugSettings`,
`MiscGeometryTools`, `IOAndPopulators`, `ObjectBaseBuild`,
`GeoShapesLifecycle`, `ivpbuild`, `ZAIC`, `AOF`, `PDMapBuilder`,
`FunctionEncoder`, `OF_Coupler`, `OF_Rater`, `bhvutil`, `ExFilterSet`,
`FixedTurn`, `FixedTurnSet`, `WaypointEngine`, `LoiterEngine`, `formats`, `math`,
`mbutil`, `MBUtils`, `MBUtilsParsing`, `MBUtilsTokens`,
`MBUtilsValidation`, `MBUtilsFormatting`, `MBUtilsMisc`, `VarDataPair`,
`NodeMessage`, `ColorPack`, `ColorParse`, `MacroUtils`, `HashUtils`,
`AckMessage`, `MailFlagSet`, `TStamp`, `Odometer`, `FileBuffer`,
`JsonUtils`, `LatLonFormatUtils`, `VQuals`, `StringTree`, `FColorMap`,
`Figlog`, `ScopeEntry`, `BundleOut`, `MBTimer`, `logic`,
`LogicCondition`, `LogicUtils`, `ParseNode`, `InfoBuffer`, `LogicBuffer`,
`ConditionalParam`, or `LedgerSnap`.
`NodeReport`, `NodeRecord`, `NodeRecordUtils`, `NodeRider`, `NodeRiderSet`,
`evalutil`, `VCheck`, `VCheckSet`, `LogicAspect`, or `LogicTestSequence`.
`obstacles`, `Obstacle`, or `ObstacleFieldGenerator`.
`ucommand`, `CommandItem`, `CommandFolio`, `CommandPost`, or
`CommandSummary`.

## Current Seed Suites

- `AngleUtils`
- `GeomUtils`
- `ArcUtils` / `ArcUtilsToDo`
- `XYArc`
- `CircularUtils`
- `XYPrimitiveShapes` / `XYArrow` / `XYSegment` / `XYSquare`
- `Dubins` / `DubinsTurn` / `DubinsCache`
- `TrackProximity` / `LinearExtrapolator` / `WrapDetector` / `ProxPoint`
- `XYFormatUtilsPoint` / `XYPoint`
- `XYFormatUtilsPoly` / `XYPolygon`
- `XYFormatUtilsSegl` / `XYSegList`
- `XYFormatUtilsSeglr` / `XYSeglr` / `Seglr`
- `XYFormatUtilsCircle` / `XYCircle`
- `XYFormatUtilsVector` / `XYVector`
- `XYFormatUtilsMarker` / `XYMarker`
- `XYFormatUtilsPulse` / `XYRangePulse` / `XYCommsPulse`
- `XYFormatUtilsWedge` / `XYWedge`
- `XYViewerShapes` / `XYOval` / `XYTextBox` / `XYVessel`
- `XYConvexGrid`
- `XYGrid` / `XYHexGrid` / `XYHexagon` / `XYGridUpdate`
- `PathUtils` / `EdgeTag` / `EdgeTagSet`
- `CPAEngine`
- `CPAPlatform` / `BNGEngine` / `CPA_Utils` / `PlatModel`
- `CPAVariants` / `CPAEngineRoot` / `CPAEngineThin` / `CPAEngineV15`
- `FieldArtifacts` / `ArtifactUtils` / `CurrentField` / `OpField`
- `PolygonGeneration` / `ConvexHullGenerator` / `XYPolyExpander` /
  `XYPatternBlock`
- `ViewPlugSettings` / `HintHolder` / `VPlug_GeoSettings` /
  `VPlug_VehiSettings` / `VPlug_DropPoints` / `VPlug_GeoShapesMap`
- `MiscGeometryTools` / `SeglrUtils` / `WallEngine` / `XYEncoders` /
  `XYFormatUtilsConvexGrid` / `XYArtifactGrid` / `XYFieldGenerator`
- `IOAndPopulators` / `IO_GeomUtils` / `Populator_OpField`
- `ObjectBaseBuild` / `XYObject` / `XYBuildUtils`
- `GeoShapesLifecycle` / `VPlug_GeoShapes`
- `AOF` / `AOF_Linear` / `AOF_Quadratic` / `AOF_Gaussian` /
  `AOF_MGaussian` / `AOF_Ring` / `AOF_Rings`
- `ZAIC` / `ZAIC_PEAK` / `ZAIC_SPD` / `ZAIC_HDG` / `ZAIC_LEQ` /
  `ZAIC_HEQ` / `ZAIC_HLEQ` / `ZAIC_Vector`
- `PDMapBuilder` / `FunctionEncoder` / `OF_Coupler` / `OF_Rater`
- `ExFilterSet`
- `FixedTurn` / `FixedTurnSet`
- `WaypointEngine`
- `LoiterEngine`
- `MBUtils` / `MBUtilsParsing` / `MBUtilsTokens` / `MBUtilsValidation` /
  `MBUtilsFormatting` / `MBUtilsMisc`
- `VarDataPair` / `NodeMessage`
- `ColorPack` / `ColorParse`
- `MacroUtils` / `HashUtils`
- `AckMessage` / `MailFlagSet` / `TStamp` / `Odometer` / `FileBuffer`
- `JsonUtils` / `LatLonFormatUtils` / `VQuals` / `StringTree`
- `FColorMap` / `Figlog` / `ScopeEntry` / `BundleOut` / `MBTimer`
- `LogicCondition` / `LogicUtils` / `ParseNode`
- `InfoBuffer` / `LogicBuffer`
- `ConditionalParam` / `LedgerSnap`
- `NodeRecord` / `NodeRecordUtils` / `NodeRider` / `NodeRiderSet`
- `VCheck` / `VCheckSet` / `LogicAspect` / `LogicTestSequence`
- `Obstacle` / `ObstacleFieldGenerator`
- `CommandItem` / `CommandFolio` / `CommandPost` / `CommandSummary`

Use tool-centric directories for new tests. Prefer names that explain the
MOOS-IvP use case or format being protected, for example
`ParsesViewPolygonPtsSpecUsedByOpRegionAndObstacles`.

## Per-Tool Coverage Standard

The goal is not one smoke test per tool. Each tool suite should cover the
shape of that tool thoroughly enough that a changed MOOS-IvP checkout gives a
specific failure.

For format parsers, include:

- canonical payloads posted by apps or behaviors, such as `VIEW_POLYGON`,
  `VIEW_SEGLIST`, `VIEW_POINT`, and `VIEW_GRID`
- abbreviated and standard forms when both are supported
- render hints and object metadata such as labels, colors, source, type, and
  sizes
- malformed inputs and edge inputs accepted by the current implementation
- serialization or round-trip checks where the API exposes a stable spec

For geometry objects and algorithms, include:

- nominal geometry and boundary cases
- degenerate inputs such as empty objects, one-point objects, zero radius, or
  zero turn angle
- wraparound heading/angle cases
- both crossing/intersection and non-crossing/non-intersection cases
- distance/value-returning functions as well as boolean classifiers
- MOOS-IvP use cases when applicable, such as waypoint lanes, loiter regions,
  OpRegion polygons, obstacle polygons, turn arcs, contact CPA, and search
  grids

For each test name, prefer `ToolArea.Context.ExpectedBehavior`, for example:

```text
ArcUtilsIntersectionTest.ReportsSeglistCrossingPointsAndClearsOutputVectors
XYConvexGridTest.ParsesViewGridSpecWithCellUpdatesAndLimits
```
