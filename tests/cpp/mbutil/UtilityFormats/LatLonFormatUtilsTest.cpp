#include <gtest/gtest.h>

#include "LatLonFormatUtils.h"

// Covers lat lon format utils behavior: converts decimal degrees to NMEA style fields.
TEST(LatLonFormatUtilsTest, ConvertsDecimalDegreesToNmeaStyleFields)
{
  EXPECT_EQ(latDDtoDDMM(39.9054895), "3954.32937");
  EXPECT_EQ(lonDDDtoDDDMM(-116.395443), "11623.72658");

  EXPECT_EQ(latDDtoDDMM(-23.82530), "2349.518");
  EXPECT_EQ(lonDDDtoDDDMM(7.5), "00730.00");
  EXPECT_EQ(latDDtoDDMM(0), "0000.00");
  EXPECT_EQ(lonDDDtoDDDMM(0), "00000.00");
}

// Covers lat lon format utils behavior: handles boundary coordinates used in GPS reports.
TEST(LatLonFormatUtilsTest, HandlesBoundaryCoordinatesUsedInGpsReports)
{
  EXPECT_EQ(latDDtoDDMM(90.0), "9000.00");
  EXPECT_EQ(latDDtoDDMM(-90.0), "9000.00");
  EXPECT_EQ(lonDDDtoDDDMM(180.0), "18000.00");
  EXPECT_EQ(lonDDDtoDDDMM(-180.0), "18000.00");

  EXPECT_EQ(latDDtoDDMM(8.5), "0830.00");
  EXPECT_EQ(lonDDDtoDDDMM(8.5), "00830.00");
  EXPECT_EQ(latDDtoDDMM(8.0125), "0800.75");
  EXPECT_EQ(lonDDDtoDDDMM(8.0125), "00800.75");
}

// Covers lat lon format utils behavior: rejects out of range and parses NMEA style fields.
TEST(LatLonFormatUtilsTest, RejectsOutOfRangeAndParsesNmeaStyleFields)
{
  EXPECT_EQ(latDDtoDDMM(90.00001), "");
  EXPECT_EQ(latDDtoDDMM(-90.00001), "");
  EXPECT_EQ(lonDDDtoDDDMM(180.00001), "");
  EXPECT_EQ(lonDDDtoDDDMM(-180.00001), "");

  EXPECT_NEAR(latDDMMtoDD("4849.518"), 48.8253, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("04849.518"), 48.8253, 1e-9);
  EXPECT_DOUBLE_EQ(latDDMMtoDD(""), 0);
  EXPECT_DOUBLE_EQ(lonDDDMMtoDDD("12"), 0);
}

// Covers lat lon format utils behavior: pins loose parsing for malformed NMEA fields.
TEST(LatLonFormatUtilsTest, PinsLooseParsingForMalformedNmeaFields)
{
  EXPECT_NEAR(latDDMMtoDD("9000.00"), 90.0, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("18000.00"), 180.0, 1e-9);
  EXPECT_NEAR(latDDMMtoDD("0830.00"), 8.5, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("00830.00"), 8.5, 1e-9);

  EXPECT_DOUBLE_EQ(latDDMMtoDD("ABCD"), 0);
  EXPECT_DOUBLE_EQ(lonDDDMMtoDDD("ABCDEF"), 0);
  EXPECT_NEAR(latDDMMtoDD("1260.00"), 13.0, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("18160.00"), 182.0, 1e-9);
}

// Covers lat lon format utils behavior: round trips GPS like fields without hemisphere designators.
TEST(LatLonFormatUtilsTest, RoundTripsGpsLikeFieldsWithoutHemisphereDesignators)
{
  const double lat = 42.358456;
  const double lon = -71.087589;

  const std::string lat_field = latDDtoDDMM(lat);
  const std::string lon_field = lonDDDtoDDDMM(lon);

  EXPECT_EQ(lat_field, "4221.50736");
  EXPECT_EQ(lon_field, "07105.25534");
  EXPECT_NEAR(latDDMMtoDD(lat_field), lat, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD(lon_field), -lon, 1e-9);
}

// Covers lat lon format utils behavior: pins caller owned hemisphere and loose string semantics.
TEST(LatLonFormatUtilsTest, PinsCallerOwnedHemisphereAndLooseStringSemantics)
{
  EXPECT_EQ(latDDtoDDMM(-0.5), "0030.00");
  EXPECT_EQ(lonDDDtoDDDMM(-0.5), "00030.00");
  EXPECT_NEAR(latDDMMtoDD("0030.00"), 0.5, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("00030.00"), 0.5, 1e-9);

  EXPECT_NEAR(latDDMMtoDD("-1230.00"), 2.833333333333333, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("-11623.72658"), -0.604557, 1e-9);
  EXPECT_NEAR(latDDMMtoDD("48 49.518"), 48.8253, 1e-9);
  EXPECT_NEAR(lonDDDMMtoDDD("048 49.518"), 48.8253, 1e-9);
}
