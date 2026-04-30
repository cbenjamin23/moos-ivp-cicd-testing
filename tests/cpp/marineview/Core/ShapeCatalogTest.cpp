#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>

#include "Shape_AUV.h"
#include "Shape_CRay.h"
#include "Shape_Circle.h"
#include "Shape_Diamond.h"
#include "Shape_EField.h"
#include "Shape_Gateway.h"
#include "Shape_Glider.h"
#include "Shape_Heron.h"
#include "Shape_Kayak.h"
#include "Shape_Kelp.h"
#include "Shape_LongShip.h"
#include "Shape_MOKAI.h"
#include "Shape_SMR.h"
#include "Shape_Ship.h"
#include "Shape_ShipX.h"
#include "Shape_Skywalker.h"
#include "Shape_Square.h"
#include "Shape_ThinAUV.h"
#include "Shape_Triangle.h"
#include "Shape_WAMV.h"

namespace {

struct Bounds {
  double min_x;
  double max_x;
  double min_y;
  double max_y;
};

template <size_t N>
Bounds expectPointArray(const double (&points)[N],
                        unsigned int declared_size,
                        const std::string& name)
{
  EXPECT_EQ(N, static_cast<size_t>(declared_size) * 2u) << name;
  EXPECT_GE(declared_size, 3u) << name;

  Bounds bounds{std::numeric_limits<double>::max(),
                -std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(),
                -std::numeric_limits<double>::max()};

  for(size_t i = 0; i < N; i += 2) {
    EXPECT_TRUE(std::isfinite(points[i])) << name << " x[" << (i / 2) << "]";
    EXPECT_TRUE(std::isfinite(points[i + 1])) << name << " y[" << (i / 2) << "]";
    bounds.min_x = std::min(bounds.min_x, points[i]);
    bounds.max_x = std::max(bounds.max_x, points[i]);
    bounds.min_y = std::min(bounds.min_y, points[i + 1]);
    bounds.max_y = std::max(bounds.max_y, points[i + 1]);
  }

  EXPECT_LT(bounds.min_x, bounds.max_x) << name;
  EXPECT_LT(bounds.min_y, bounds.max_y) << name;
  return bounds;
}

void expectSymmetricX(const Bounds& bounds, const std::string& name)
{
  EXPECT_NEAR(bounds.min_x + bounds.max_x, 0.0, 1e-9) << name;
}

template <size_t N>
Bounds expectCoordinateCountArray(const double (&points)[N],
                                  unsigned int declared_size,
                                  const std::string& name)
{
  EXPECT_EQ(N, static_cast<size_t>(declared_size)) << name;
  return expectPointArray(points, declared_size / 2, name);
}

template <size_t N>
void expectClosed(const double (&points)[N], const std::string& name)
{
  ASSERT_GE(N, 4u);
  EXPECT_DOUBLE_EQ(points[0], points[N - 2]) << name;
  EXPECT_DOUBLE_EQ(points[1], points[N - 1]) << name;
}

}  // namespace

// Covers marine view shape catalog behavior: core vehicle bodies have consistent point counts.
TEST(MarineViewShapeCatalogTest, CoreVehicleBodiesHaveConsistentPointCounts)
{
  expectSymmetricX(expectPointArray(g_shipBody, g_shipBodySize, "ship"), "ship");
  expectSymmetricX(expectPointArray(g_shipFatBody, g_shipFatBodySize, "ship_fat"),
                  "ship_fat");
  expectSymmetricX(expectPointArray(g_longShipBody, g_longShipBodySize, "long_ship"),
                  "long_ship");
  expectSymmetricX(expectCoordinateCountArray(g_shipXBody, g_shipXBodySize, "shipx"),
                  "shipx");
  expectSymmetricX(expectPointArray(g_kayakBody, g_kayakBodySize, "kayak"), "kayak");
  expectSymmetricX(expectPointArray(g_wamvBody, g_wamvBodySize, "wamv"), "wamv");
  expectSymmetricX(expectPointArray(g_heronBody, g_heronBodySize, "heron"), "heron");
  expectSymmetricX(expectPointArray(g_SMR_Body, g_SMR_BodySize, "smr"), "smr");
  expectSymmetricX(expectPointArray(g_auvBody, g_auvBodySize, "auv"), "auv");
  expectSymmetricX(expectPointArray(g_thin_auvBody, g_thin_auvBodySize, "thin_auv"),
                  "thin_auv");
  expectSymmetricX(expectPointArray(g_gliderBody, g_gliderBodySize, "glider"), "glider");
  expectSymmetricX(expectPointArray(g_mokaiBody, g_mokaiBodySize, "mokai"), "mokai");
  expectSymmetricX(expectPointArray(g_crayBody, g_crayBodySize, "cray"), "cray");
  expectSymmetricX(expectPointArray(g_skywBody, g_skywBodySize, "skywalker"),
                  "skywalker");
}

// Covers marine view shape catalog behavior: multipart vehicle components remain drawable polygons.
TEST(MarineViewShapeCatalogTest, MultipartVehicleComponentsRemainDrawablePolygons)
{
  expectPointArray(g_propUnit, g_propUnitSize, "auv_prop");
  expectPointArray(g_thin_propUnit, g_thin_propUnitSize, "thin_auv_prop");
  expectPointArray(g_wamvpropUnit, g_wamvpropUnitSize, "wamv_prop");
  expectPointArray(g_wamvPontoonConnector, g_pontoonConnectorSize,
                   "wamv_pontoon_connector");
  expectPointArray(g_heronBack, g_heronBackSize, "heron_back");
  expectPointArray(g_heronFront, g_heronFrontSize, "heron_front");
  expectPointArray(g_SMR_IBody, g_SMR_IBodySize, "smr_inner");
  expectPointArray(g_SMR_IMBody, g_SMR_IMBodySize, "smr_mid");
  expectPointArray(g_SMR_MotorA, g_SMR_MotorASize, "smr_motor_a");
  expectPointArray(g_SMR_MotorB, g_SMR_MotorBSize, "smr_motor_b");
  expectPointArray(g_SMR_Slash, g_SMR_SlashSize, "smr_slash");
  expectPointArray(g_gliderWing, g_gliderWingSize, "glider_wing");
  expectPointArray(g_mokaiStern, g_mokaiSternSize, "mokai_stern");
  expectPointArray(g_mokaiSlice1, g_mokaiSlice1Size, "mokai_slice1");
  expectPointArray(g_mokaiSlice2, g_mokaiSlice2Size, "mokai_slice2");
  expectPointArray(g_mokaiSlice3, g_mokaiSlice3Size, "mokai_slice3");
  expectPointArray(g_mokaiBow, g_mokaiBowSize, "mokai_bow");
  expectPointArray(g_mokaiMidOpen, g_mokaiMidOpenSize, "mokai_mid_open");
  expectPointArray(g_crayBaseFinR, g_crayBaseFinRSize, "cray_base_fin_r");
  expectPointArray(g_crayBaseFinL, g_crayBaseFinLSize, "cray_base_fin_l");
  expectPointArray(g_crayFinR1, g_crayFinR1Size, "cray_fin_r1");
  expectPointArray(g_crayFinL1, g_crayFinL1Size, "cray_fin_l1");
  expectPointArray(g_crayFinR2, g_crayFinR2Size, "cray_fin_r2");
  expectPointArray(g_crayFinL2, g_crayFinL2Size, "cray_fin_l2");
  expectPointArray(g_crayFinR3, g_crayFinR3Size, "cray_fin_r3");
  expectPointArray(g_crayFinL3, g_crayFinL3Size, "cray_fin_l3");
  expectPointArray(g_gatewayMidBody, g_gatewayMidBodySize, "gateway_mid");
  expectPointArray(g_efieldMidBody, g_efieldMidBodySize, "efield_mid");
  expectPointArray(g_kayakMidOpen, g_kayakMidOpenSize, "kayak_mid_open");
  expectPointArray(g_skywMid, g_skywMidSize, "skywalker_mid");
}

// Covers marine view shape catalog behavior: symbolic viewer shapes have stable closed outlines.
TEST(MarineViewShapeCatalogTest, SymbolicViewerShapesHaveStableClosedOutlines)
{
  expectClosed(g_shipBody, "ship");
  expectClosed(g_shipFatBody, "ship_fat");
  expectClosed(g_shipXBody, "shipx");
  expectClosed(g_circleBody, "circle");
  expectClosed(g_diamondBody, "diamond");
  expectClosed(g_gatewayBody, "gateway");
  expectClosed(g_squareBody, "square");
  expectClosed(g_triangleBody, "triangle");
  expectClosed(g_skywBody, "skywalker");

  expectPointArray(g_circleBody, g_circleBodySize, "circle");
  expectPointArray(g_diamondBody, g_diamondBodySize, "diamond");
  expectPointArray(g_gatewayBody, g_gatewayBodySize, "gateway");
  expectPointArray(g_efieldBody, g_efieldBodySize, "efield");
  expectPointArray(g_squareBody, g_squareBodySize, "square");
  expectPointArray(g_triangleBody, g_triangleBodySize, "triangle");
  expectPointArray(g_kelpBody, g_kelpBodySize, "kelp");
}

// Covers marine view shape catalog behavior: nominal lengths centers and widths stay positive.
TEST(MarineViewShapeCatalogTest, NominalLengthsCentersAndWidthsStayPositive)
{
  EXPECT_DOUBLE_EQ(g_shipLength, 212.0);
  EXPECT_DOUBLE_EQ(g_shipFatLength, 212.0);
  EXPECT_DOUBLE_EQ(g_auvLength, 100.0);
  EXPECT_DOUBLE_EQ(g_wamvLength, 100.0);
  EXPECT_DOUBLE_EQ(g_shipXLength, 31.5);
  EXPECT_DOUBLE_EQ(g_heronLength, 24.0);
  EXPECT_DOUBLE_EQ(g_gliderLength, 70.5);
  EXPECT_DOUBLE_EQ(g_crayLength, 33.5);
  EXPECT_DOUBLE_EQ(g_circleWidth, 10.0);
  EXPECT_DOUBLE_EQ(g_diamondWidth, 16.0);
  EXPECT_DOUBLE_EQ(g_gatewayWidth, 10.0);
  EXPECT_DOUBLE_EQ(g_squareWidth, 20.0);
  EXPECT_DOUBLE_EQ(g_triangleWidth, 16.0);
  EXPECT_DOUBLE_EQ(g_skywLength, 25.0);

  EXPECT_DOUBLE_EQ(g_shipCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_shipFatCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_auvCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_wamvCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_shipXCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_heronCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_thin_auvCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_gliderCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_gatewayCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_diamondCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_circleCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_squareCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_triangleCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_skywCtrX, 0.0);

  EXPECT_GT(g_shipCtrY, 0.0);
  EXPECT_GT(g_auvCtrY, 0.0);
  EXPECT_GT(g_wamvCtrY, 0.0);
  EXPECT_GT(g_shipXCtrY, 0.0);
  EXPECT_GT(g_heronCtrY, 0.0);
  EXPECT_GT(g_thin_auvCtrY, 0.0);
  EXPECT_GT(g_gliderCtrY, 0.0);
  EXPECT_GT(g_gatewayCtrY, 0.0);
  EXPECT_GT(g_triangleCtrY, 0.0);
  EXPECT_GT(g_skywCtrY, 0.0);
}
