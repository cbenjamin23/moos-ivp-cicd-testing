#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"

namespace {

constexpr double kProjectionToleranceMeters = 0.25;
constexpr double kInverseToleranceDegrees = 2e-5;
constexpr double kLocalGridRelativeTolerance = 0.001;
// The legacy spherical grid loses more east-axis precision near the poles.
constexpr double kDiverseLocalGridRelativeTolerance = 0.005;

struct GeographicFix {
  double latitude;
  double longitude;
};

struct LocalFix {
  double east;
  double north;
};

LocalFix projectUTM(CMOOSGeodesy& geodesy, const GeographicFix& fix)
{
  double north = 0;
  double east = 0;
  EXPECT_TRUE(geodesy.LatLong2LocalUTM(fix.latitude, fix.longitude,
                                      north, east));
  EXPECT_TRUE(std::isfinite(north));
  EXPECT_TRUE(std::isfinite(east));
  return {east, north};
}

GeographicFix unprojectUTM(CMOOSGeodesy& geodesy, const LocalFix& fix)
{
  double latitude = 0;
  double longitude = 0;
  EXPECT_TRUE(geodesy.UTM2LatLong(fix.east, fix.north,
                                 latitude, longitude));
  EXPECT_TRUE(std::isfinite(latitude));
  EXPECT_TRUE(std::isfinite(longitude));
  return {latitude, longitude};
}

void expectGeographicNear(const GeographicFix& actual,
                          const GeographicFix& expected,
                          double tolerance = kInverseToleranceDegrees)
{
  EXPECT_NEAR(actual.latitude, expected.latitude, tolerance);
  EXPECT_NEAR(actual.longitude, expected.longitude, tolerance);
}

void expectLocalNear(const LocalFix& actual,
                     const LocalFix& expected,
                     double tolerance)
{
  EXPECT_NEAR(actual.east, expected.east, tolerance);
  EXPECT_NEAR(actual.north, expected.north, tolerance);
}

std::string utmZone(CMOOSGeodesy& geodesy)
{
  const char* zone = geodesy.GetUTMZone();
  return zone == nullptr ? std::string() : std::string(zone);
}

double wrappedLongitudeDifference(double lhs, double rhs)
{
  double difference = std::fmod(lhs - rhs, 360.0);
  if(difference > 180.0)
    difference -= 360.0;
  else if(difference < -180.0)
    difference += 360.0;
  return difference;
}

}  // namespace

// Covers datum initialization: stores the WGS-84 origin and derives its UTM zone.
TEST(MOOSGeodesyInitializationTest, StoresOriginEllipsoidAndZone)
{
  CMOOSGeodesy geodesy;

  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  EXPECT_DOUBLE_EQ(geodesy.GetOriginLatitude(), 43.8253);
  EXPECT_DOUBLE_EQ(geodesy.GetOriginLongitude(), -70.3304);
  EXPECT_EQ(geodesy.GetRefEllipsoid(), 23);
  EXPECT_EQ(utmZone(geodesy), "19T");
  EXPECT_TRUE(std::isfinite(geodesy.GetOriginEasting()));
  EXPECT_TRUE(std::isfinite(geodesy.GetOriginNorthing()));
}

// Covers datum initialization: selects standard, exceptional, and edge UTM zones.
TEST(MOOSGeodesyInitializationTest, SelectsHemisphereAndExceptionalUTMZones)
{
  struct ZoneCase {
    GeographicFix origin;
    const char* expected_zone;
  };

  const std::vector<ZoneCase> cases = {
      {{-33.8688, 151.2093}, "56H"},
      {{0.0, 3.0}, "31N"},
      {{60.0, 4.0}, "32V"},       // Norway exception.
      {{75.0, 20.0}, "33X"},      // Svalbard exception.
      {{0.0, 179.9}, "60N"},      // Eastern edge of the zone range.
      {{0.0, -179.9}, "1N"},      // Western edge of the zone range.
  };

  for(const ZoneCase& test_case : cases) {
    SCOPED_TRACE(test_case.expected_zone);
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(test_case.origin.latitude,
                                   test_case.origin.longitude));
    EXPECT_EQ(utmZone(geodesy), test_case.expected_zone);
  }
}

// Covers UTM initialization at every supported latitude-band lower boundary.
TEST(MOOSGeodesyInitializationTest, SelectsEverySupportedUTMLatitudeBand)
{
  struct BandCase {
    double latitude;
    const char* expected_zone;
  };

  // Longitude -3 lies in zone 30 and avoids the Norway/Svalbard exceptions,
  // isolating the latitude-band designator under test.
  const std::vector<BandCase> cases = {
      {-80, "30C"}, {-72, "30D"}, {-64, "30E"}, {-56, "30F"},
      {-48, "30G"}, {-40, "30H"}, {-32, "30J"}, {-24, "30K"},
      {-16, "30L"}, {-8, "30M"},  {0, "30N"},   {8, "30P"},
      {16, "30Q"},  {24, "30R"}, {32, "30S"}, {40, "30T"},
      {48, "30U"},  {56, "30V"}, {64, "30W"}, {72, "30X"},
      {84, "30X"},
  };

  for(const BandCase& test_case : cases) {
    SCOPED_TRACE(test_case.latitude);
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(test_case.latitude, -3.0));
    EXPECT_EQ(utmZone(geodesy), test_case.expected_zone);
  }
}

// Covers longitude normalization and exact six-degree UTM zone boundaries.
TEST(MOOSGeodesyInitializationTest, NormalizesLongitudeAtUTMZoneEdges)
{
  struct ZoneCase {
    double longitude;
    const char* expected_zone;
  };

  const std::vector<ZoneCase> cases = {
      {-180.0, "1N"}, {-174.0, "2N"}, {-0.000001, "30N"},
      {0.0, "31N"},   {6.0, "32N"},   {179.999999, "60N"},
      {180.0, "1N"},
  };

  for(const ZoneCase& test_case : cases) {
    SCOPED_TRACE(test_case.longitude);
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(0.0, test_case.longitude));
    EXPECT_EQ(utmZone(geodesy), test_case.expected_zone);
  }
}

// Covers reinitialization: replacing the datum reanchors the local UTM frame.
TEST(MOOSGeodesyInitializationTest, ReinitializationReanchorsLocalFrame)
{
  const GeographicFix first_origin{43.8253, -70.3304};
  const GeographicFix second_origin{42.3584, -71.08745};
  CMOOSGeodesy geodesy;

  ASSERT_TRUE(geodesy.Initialise(first_origin.latitude,
                                 first_origin.longitude));
  const LocalFix before = projectUTM(geodesy, second_origin);
  EXPECT_GT(std::hypot(before.east, before.north), 1000.0);

  ASSERT_TRUE(geodesy.Initialise(second_origin.latitude,
                                 second_origin.longitude));
  expectLocalNear(projectUTM(geodesy, second_origin), {0, 0}, 1e-6);
  EXPECT_DOUBLE_EQ(geodesy.GetOriginLatitude(), second_origin.latitude);
  EXPECT_DOUBLE_EQ(geodesy.GetOriginLongitude(), second_origin.longitude);
  EXPECT_EQ(utmZone(geodesy), "19T");
}

// Covers UTM projection: the configured geographic datum is local zero.
TEST(MOOSGeodesyUTMTest, DatumProjectsToLocalZero)
{
  const GeographicFix origin{43.8253, -70.3304};
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(origin.latitude, origin.longitude));

  const LocalFix local = projectUTM(geodesy, origin);

  expectLocalNear(local, {0, 0}, 1e-6);
  EXPECT_NEAR(geodesy.GetMetersEast(), 0, 1e-6);
  EXPECT_NEAR(geodesy.GetMetersNorth(), 0, 1e-6);
}

// Covers absolute UTM state against WGS-84 origins in both hemispheres.
TEST(MOOSGeodesyUTMTest, ReportsKnownAbsoluteOriginEastingsAndNorthings)
{
  struct OriginCase {
    GeographicFix origin;
    LocalFix absolute_utm;
  };

  const std::vector<OriginCase> cases = {
      {{43.8253, -70.3304}, {393023.475096860, 4853329.704409766}},
      {{-33.8688, 151.2093}, {334368.633648097, 6250948.345385009}},
      {{0.0, 3.0}, {500000.000000001, 0.0}},
      {{84.0, -3.0}, {500000.000000000, 9328093.830560509}},
      {{-80.0, -3.0}, {500000.000000000, 1118414.184011905}},
  };

  for(const OriginCase& test_case : cases) {
    SCOPED_TRACE(test_case.origin.latitude);
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(test_case.origin.latitude,
                                   test_case.origin.longitude));
    EXPECT_NEAR(geodesy.GetOriginEasting(), test_case.absolute_utm.east,
                kProjectionToleranceMeters);
    EXPECT_NEAR(geodesy.GetOriginNorthing(), test_case.absolute_utm.north,
                kProjectionToleranceMeters);
  }
}

// Covers UTM projection against independently generated WGS-84 reference vectors.
TEST(MOOSGeodesyUTMTest, MatchesKnownWGS84ReferenceVectors)
{
  // Expected deltas were generated with PROJ using WGS-84 and each datum's
  // fixed UTM zone. They pin degree/radian conversion, east/north ordering,
  // hemisphere handling, and local-origin subtraction.
  struct ProjectionCase {
    GeographicFix origin;
    GeographicFix target;
    LocalFix expected;
  };

  const std::vector<ProjectionCase> cases = {
      {{43.8253, -70.3304}, {43.82536, -70.33046},
       {-4.717421778, 6.741529793}},
      {{-33.8688, 151.2093}, {-33.8680, 151.2100},
       {63.209659229, 89.845059326}},
      {{0.0, 3.0}, {0.001, 3.001},
       {111.274962986, 110.530046128}},
  };

  for(const ProjectionCase& test_case : cases) {
    SCOPED_TRACE(test_case.origin.latitude);
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(test_case.origin.latitude,
                                   test_case.origin.longitude));
    expectLocalNear(projectUTM(geodesy, test_case.target),
                    test_case.expected, kProjectionToleranceMeters);
  }
}

// Covers UTM projection: longitude controls east and latitude controls north.
TEST(MOOSGeodesyUTMTest, PreservesAxisOrderAndCardinalSigns)
{
  const GeographicFix origin{43.8253, -70.3304};
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(origin.latitude, origin.longitude));

  const LocalFix east = projectUTM(geodesy, {origin.latitude, -70.3294});
  const LocalFix west = projectUTM(geodesy, {origin.latitude, -70.3314});
  const LocalFix north = projectUTM(geodesy, {43.8263, origin.longitude});
  const LocalFix south = projectUTM(geodesy, {43.8243, origin.longitude});

  EXPECT_GT(east.east, 0);
  EXPECT_LT(west.east, 0);
  EXPECT_GT(north.north, 0);
  EXPECT_LT(south.north, 0);
  EXPECT_LT(std::abs(east.north), std::abs(east.east));
  EXPECT_LT(std::abs(north.east), std::abs(north.north));
}

// Covers points close to a zone seam while both remain in the datum's zone.
TEST(MOOSGeodesyUTMTest, RemainsContinuousApproachingUTMZoneBoundary)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(0.0, -0.0001));
  ASSERT_EQ(utmZone(geodesy), "30N");

  const LocalFix local = projectUTM(geodesy, {0.0, -0.000001});

  EXPECT_NEAR(local.east, 11.031441808, kProjectionToleranceMeters);
  EXPECT_NEAR(local.north, 0.0, kProjectionToleranceMeters);
}

// Covers forward/inverse UTM conversion over mission-scale and long offsets.
TEST(MOOSGeodesyUTMTest, RoundTripsGeographicFixesAcrossScalesAndHemispheres)
{
  struct RoundTripCase {
    GeographicFix origin;
    std::vector<GeographicFix> targets;
  };

  const std::vector<RoundTripCase> cases = {
      {{43.8253, -70.3304},
       {{43.8253, -70.3304}, {43.834, -70.318}, {44.5, -69.5}}},
      {{-33.8688, 151.2093},
       {{-33.8680, 151.2100}, {-33.0, 152.0}}},
      {{0.0, 3.0},
       {{0.001, 3.001}, {0.7, 3.8}}},
      {{60.0, 4.0},
       {{60.2, 4.5}}},
      {{75.0, 20.0},
       {{75.2, 20.5}}},
  };

  for(const RoundTripCase& test_case : cases) {
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(test_case.origin.latitude,
                                   test_case.origin.longitude));
    for(const GeographicFix& target : test_case.targets) {
      SCOPED_TRACE(target.latitude);
      const LocalFix local = projectUTM(geodesy, target);
      expectGeographicNear(unprojectUTM(geodesy, local), target);
    }
  }
}

// Covers inverse/forward UTM conversion for explicit local mission coordinates.
TEST(MOOSGeodesyUTMTest, RoundTripsLocalOffsetsThroughGeographicCoordinates)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  const std::vector<LocalFix> offsets = {
      {0, 0}, {1, -1}, {1000, 2000}, {-5000, 7500}, {100000, -100000},
  };

  for(const LocalFix& expected : offsets) {
    SCOPED_TRACE(expected.east);
    const GeographicFix global = unprojectUTM(geodesy, expected);
    expectLocalNear(projectUTM(geodesy, global), expected, 1.1);
  }
}

// Covers projection state: results do not depend on prior conversions.
TEST(MOOSGeodesyUTMTest, RepeatedProjectionIsIndependentOfCallHistory)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  const GeographicFix target{43.82536, -70.33046};
  const LocalFix first = projectUTM(geodesy, target);
  projectUTM(geodesy, {44.5, -69.5});
  const LocalFix repeated = projectUTM(geodesy, target);

  expectLocalNear(repeated, first, 1e-9);
  EXPECT_NEAR(geodesy.GetMetersEast(), repeated.east, 1e-9);
  EXPECT_NEAR(geodesy.GetMetersNorth(), repeated.north, 1e-9);
}

// Covers local-grid projection: datum and cardinal directions use local meters.
TEST(MOOSGeodesyLocalGridTest, ProjectsDatumAndCardinalDirections)
{
  const GeographicFix origin{43.8253, -70.3304};
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(origin.latitude, origin.longitude));

  double north = 0;
  double east = 0;
  ASSERT_TRUE(geodesy.LatLong2LocalGrid(origin.latitude, origin.longitude,
                                       north, east));
  EXPECT_NEAR(east, 0, 1e-9);
  EXPECT_NEAR(north, 0, 1e-9);

  ASSERT_TRUE(geodesy.LatLong2LocalGrid(origin.latitude, -70.3294,
                                       north, east));
  EXPECT_GT(east, 0);
  EXPECT_NEAR(north, 0, 1e-9);

  ASSERT_TRUE(geodesy.LatLong2LocalGrid(43.8263, origin.longitude,
                                       north, east));
  EXPECT_NEAR(east, 0, 1e-9);
  EXPECT_GT(north, 0);
}

// Covers local-grid forward/inverse conversion for representative mission offsets.
TEST(MOOSGeodesyLocalGridTest, RoundTripsRepresentativeLocalOffsets)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  const std::vector<LocalFix> offsets = {
      {0, 0}, {10, -20}, {1000, 2000}, {-5000, 7500},
  };

  for(const LocalFix& expected : offsets) {
    SCOPED_TRACE(expected.east);
    double latitude = 0;
    double longitude = 0;
    ASSERT_TRUE(geodesy.LocalGrid2LatLong(expected.east, expected.north,
                                         latitude, longitude));

    double north = 0;
    double east = 0;
    ASSERT_TRUE(geodesy.LatLong2LocalGrid(latitude, longitude, north, east));
    const double tolerance = std::max(
        0.05, std::hypot(expected.east, expected.north) *
                  kLocalGridRelativeTolerance);
    expectLocalNear({east, north}, expected, tolerance);
    EXPECT_NEAR(geodesy.GetLocalGridX(), east, 1e-9);
    EXPECT_NEAR(geodesy.GetLocalGridY(), north, 1e-9);
  }
}

// Covers local-grid round trips at equatorial, southern, and high latitudes.
TEST(MOOSGeodesyLocalGridTest, RoundTripsOffsetsAtDiverseLatitudes)
{
  const std::vector<GeographicFix> origins = {
      {0.0, 3.0}, {-33.8688, 151.2093}, {75.0, 20.0},
  };
  const std::vector<LocalFix> offsets = {
      {10, -20}, {1000, 2000}, {-5000, 7500},
  };

  for(const GeographicFix& origin : origins) {
    CMOOSGeodesy geodesy;
    ASSERT_TRUE(geodesy.Initialise(origin.latitude, origin.longitude));
    for(const LocalFix& expected : offsets) {
      SCOPED_TRACE(origin.latitude);
      SCOPED_TRACE(expected.east);
      double latitude = 0;
      double longitude = 0;
      ASSERT_TRUE(geodesy.LocalGrid2LatLong(expected.east, expected.north,
                                           latitude, longitude));

      double north = 0;
      double east = 0;
      ASSERT_TRUE(geodesy.LatLong2LocalGrid(latitude, longitude,
                                           north, east));
      const double tolerance = std::max(
          0.05, std::hypot(expected.east, expected.north) *
                    kDiverseLocalGridRelativeTolerance);
      expectLocalNear({east, north}, expected, tolerance);
    }
  }
}

// Covers the local-grid representation of a short antimeridian crossing.
TEST(MOOSGeodesyLocalGridTest, WrapsShortDistanceAcrossAntimeridian)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(0.0, 179.999));

  double north = 0;
  double east = 0;
  ASSERT_TRUE(geodesy.LatLong2LocalGrid(0.0, -179.999, north, east));
  EXPECT_NEAR(east, 222.638981545, kProjectionToleranceMeters);
  EXPECT_NEAR(north, 0.0, 1e-9);

  double latitude = 0;
  double longitude = 0;
  ASSERT_TRUE(geodesy.LocalGrid2LatLong(east, north, latitude, longitude));
  EXPECT_NEAR(latitude, 0.0, kInverseToleranceDegrees);
  EXPECT_NEAR(wrappedLongitudeDifference(longitude, -179.999), 0.0,
              kInverseToleranceDegrees);
}

// Covers local-grid inverse validation: out-of-domain offsets fail safely.
TEST(MOOSGeodesyLocalGridTest, RejectsOffsetsOutsideInverseTrigonometricDomain)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  double latitude = 12;
  double longitude = 34;
  EXPECT_FALSE(geodesy.LocalGrid2LatLong(EARTH_RADIUS * 2, 0,
                                        latitude, longitude));
  EXPECT_DOUBLE_EQ(latitude, 0);
  EXPECT_DOUBLE_EQ(longitude, 0);

  latitude = 12;
  longitude = 34;
  EXPECT_FALSE(geodesy.LocalGrid2LatLong(0, EARTH_RADIUS * 2,
                                        latitude, longitude));
  EXPECT_DOUBLE_EQ(latitude, 0);
  EXPECT_DOUBLE_EQ(longitude, 0);
}

// Migration contract: a local frame must stay in its datum's fixed UTM zone.
// The handwritten implementation currently reselects a zone for each point,
// so enable this test with the PROJ-backed fixed-zone implementation.
TEST(MOOSGeodesyMigrationContractTest,
     DISABLED_RemainsContinuousAcrossAdjacentUTMZones)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(0.0, -0.0001));

  const LocalFix local = projectUTM(geodesy, {0.0, 0.0001});

  EXPECT_NEAR(local.east, 22.285740412, kProjectionToleranceMeters);
  EXPECT_NEAR(local.north, 0.0, kProjectionToleranceMeters);
}

// Migration contract: longitude wrapping must not introduce a zone-width jump.
TEST(MOOSGeodesyMigrationContractTest,
     DISABLED_RemainsContinuousAcrossAntimeridian)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(0.0, 179.999));

  const LocalFix local = projectUTM(geodesy, {0.0, -179.999});

  EXPECT_NEAR(local.east, 222.857404132, kProjectionToleranceMeters);
  EXPECT_NEAR(local.north, 0.0, kProjectionToleranceMeters);
  const GeographicFix global = unprojectUTM(geodesy, local);
  EXPECT_NEAR(global.latitude, 0.0, kInverseToleranceDegrees);
  EXPECT_NEAR(wrappedLongitudeDifference(global.longitude, -179.999), 0.0,
              kInverseToleranceDegrees);
}

// Migration contract: UTM initialization accepts only finite coordinates in
// the supported UTM domain and leaves an existing datum unchanged on failure.
TEST(MOOSGeodesyMigrationContractTest,
     DISABLED_RejectsInvalidInitializationWithoutChangingDatum)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  EXPECT_FALSE(geodesy.Initialise(84.000001, -70.3304));
  EXPECT_FALSE(geodesy.Initialise(-80.000001, -70.3304));
  EXPECT_FALSE(geodesy.Initialise(0.0, 180.000001));
  EXPECT_FALSE(geodesy.Initialise(
      std::numeric_limits<double>::quiet_NaN(), -70.3304));
  EXPECT_FALSE(geodesy.Initialise(
      43.8253, std::numeric_limits<double>::infinity()));

  EXPECT_DOUBLE_EQ(geodesy.GetOriginLatitude(), 43.8253);
  EXPECT_DOUBLE_EQ(geodesy.GetOriginLongitude(), -70.3304);
  EXPECT_EQ(utmZone(geodesy), "19T");
}

// Migration contract: failed forward conversions return false without
// replacing caller-owned output values or the last valid projection state.
TEST(MOOSGeodesyMigrationContractTest,
     DISABLED_RejectsNonFiniteProjectionWithoutChangingOutputs)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));
  const LocalFix valid = projectUTM(geodesy, {43.82536, -70.33046});

  double north = 123;
  double east = 456;
  EXPECT_FALSE(geodesy.LatLong2LocalUTM(
      std::numeric_limits<double>::quiet_NaN(), -70.3304, north, east));
  EXPECT_DOUBLE_EQ(north, 123);
  EXPECT_DOUBLE_EQ(east, 456);
  EXPECT_DOUBLE_EQ(geodesy.GetMetersNorth(), valid.north);
  EXPECT_DOUBLE_EQ(geodesy.GetMetersEast(), valid.east);
}

// Covers value semantics: copy construction preserves an independent usable datum.
TEST(MOOSGeodesyValueSemanticsTest, CopyConstructionPreservesProjection)
{
  CMOOSGeodesy original;
  ASSERT_TRUE(original.Initialise(43.8253, -70.3304));
  const GeographicFix target{43.82536, -70.33046};
  const LocalFix expected = projectUTM(original, target);

  CMOOSGeodesy copy(original);

  expectLocalNear(projectUTM(copy, target), expected, 1e-9);
  expectLocalNear(projectUTM(original, target), expected, 1e-9);
  EXPECT_EQ(utmZone(copy), "19T");
}

// Covers value semantics: an assigned copy remains valid after its source dies.
TEST(MOOSGeodesyValueSemanticsTest, CopyAssignmentOutlivesSource)
{
  CMOOSGeodesy copy;
  ASSERT_TRUE(copy.Initialise(-33.8688, 151.2093));

  {
    CMOOSGeodesy source;
    ASSERT_TRUE(source.Initialise(43.8253, -70.3304));
    copy = source;
  }

  const LocalFix projected = projectUTM(copy, {43.82536, -70.33046});
  expectLocalNear(projected, {-4.717421778, 6.741529793},
                  kProjectionToleranceMeters);
  EXPECT_EQ(utmZone(copy), "19T");
}

// Covers value semantics under container copies and destruction ordering.
TEST(MOOSGeodesyValueSemanticsTest, ContainerCopiesRemainUsable)
{
  CMOOSGeodesy source;
  ASSERT_TRUE(source.Initialise(43.8253, -70.3304));
  const GeographicFix target{43.82536, -70.33046};
  const LocalFix expected = projectUTM(source, target);

  std::vector<CMOOSGeodesy> copies(4, source);
  for(CMOOSGeodesy& copy : copies)
    expectLocalNear(projectUTM(copy, target), expected, 1e-9);

  copies.erase(copies.begin() + 1);
  copies.push_back(source);
  for(CMOOSGeodesy& copy : copies)
    expectLocalNear(projectUTM(copy, target), expected, 1e-9);
}

// Covers utility conversion from degree-minute notation to decimal degrees.
TEST(MOOSGeodesyUtilityTest, ConvertsDegreeMinutesToDecimalDegrees)
{
  CMOOSGeodesy geodesy;

  EXPECT_DOUBLE_EQ(geodesy.DMS2DecDeg(0), 0);
  EXPECT_NEAR(geodesy.DMS2DecDeg(4230.0), 42.5, 1e-12);
  EXPECT_NEAR(geodesy.DMS2DecDeg(7045.5), 70.7583333333333, 1e-12);
  EXPECT_NEAR(geodesy.DMS2DecDeg(-7030.0), -70.5, 1e-12);
}
