#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "Shape_LongShipOrig.h"
#include "Shape_WAMV2.h"

namespace {

struct Bounds {
  double min_x;
  double max_x;
  double min_y;
  double max_y;
};

template <size_t N>
Bounds expectPointArray(const double (&points)[N], double declared_size)
{
  EXPECT_EQ(N, static_cast<size_t>(declared_size) * 2u);
  EXPECT_GE(declared_size, 3.0);

  Bounds bounds{std::numeric_limits<double>::max(),
                -std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(),
                -std::numeric_limits<double>::max()};

  for(size_t i = 0; i < N; i += 2) {
    EXPECT_TRUE(std::isfinite(points[i]));
    EXPECT_TRUE(std::isfinite(points[i + 1]));
    bounds.min_x = std::min(bounds.min_x, points[i]);
    bounds.max_x = std::max(bounds.max_x, points[i]);
    bounds.min_y = std::min(bounds.min_y, points[i + 1]);
    bounds.max_y = std::max(bounds.max_y, points[i + 1]);
  }

  EXPECT_LT(bounds.min_x, bounds.max_x);
  EXPECT_LT(bounds.min_y, bounds.max_y);
  return bounds;
}

template <size_t N>
void expectClosed(const double (&points)[N])
{
  ASSERT_GE(N, 4u);
  EXPECT_DOUBLE_EQ(points[0], points[N - 2]);
  EXPECT_DOUBLE_EQ(points[1], points[N - 1]);
}

}  // namespace

// Covers marine view legacy shape catalog behavior: legacy long ship header keeps original hull.
TEST(MarineViewLegacyShapeCatalogTest, LegacyLongShipHeaderKeepsOriginalHull)
{
  Bounds bounds = expectPointArray(g_longShipBody, g_longShipBodySize);
  EXPECT_NEAR(bounds.min_x + bounds.max_x, 0.0, 1e-9);
  EXPECT_DOUBLE_EQ(g_longShipCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_longShipCtrY, 15.6);
  EXPECT_DOUBLE_EQ(g_longShipLength, 51.2);
}

// Covers marine view legacy shape catalog behavior: wamv2 pontoon and top remain closed drawable polygons.
TEST(MarineViewLegacyShapeCatalogTest, Wamv2PontoonAndTopRemainClosedDrawablePolygons)
{
  EXPECT_DOUBLE_EQ(g_wamvSize, 5);
  EXPECT_DOUBLE_EQ(g_wamvLength, 200.0);
  EXPECT_DOUBLE_EQ(g_wamvCtrX, 0.0);
  EXPECT_DOUBLE_EQ(g_wamvCtrY, 0.0);
  EXPECT_DOUBLE_EQ(g_wamvBase, 60.0);

  expectClosed(g_wamvPontoon);
  expectClosed(g_wamvTop);
  Bounds pontoon = expectPointArray(g_wamvPontoon, g_wamvPontoonSize);
  Bounds top = expectPointArray(g_wamvTop, g_wamvTopSize);
  EXPECT_NEAR(pontoon.min_x + pontoon.max_x, 0.0, 1e-9);
  EXPECT_GT(top.max_x, top.min_x);
  EXPECT_GT(top.max_y, top.min_y);
}
