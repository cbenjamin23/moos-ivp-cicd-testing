#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "WormHole.h"
#include "WormHoleSet.h"

namespace {

std::string config(const std::string& tag = "alpha",
                   const std::string& connection = "from_madrid",
                   const std::string& delay = "0")
{
  return "tag=" + tag +
         ",madrid_poly=pts={0,0:10,0:10,10:0,10}" +
         ",weber_poly=pts={100,100:110,100:110,110:100,110}" +
         ",connection=" + connection +
         ",delay=" + delay +
         ",id_change=false";
}

bool Contains(const std::vector<std::string>& lines, const std::string& needle)
{
  for(const std::string& line : lines) {
    if(line.find(needle) != std::string::npos)
      return true;
  }
  return false;
}

}  // namespace

// Covers uSimMarine wormhole behavior: accepts convex polygons and rejects non-convex polygons.
TEST(WormHoleTest, AcceptsConvexPolygonsAndRejectsNonConvexPolygons)
{
  WormHole hole("alpha");
  EXPECT_TRUE(hole.setMadridPoly(makeSquarePoly(0, 0, 10, 10)));
  EXPECT_TRUE(hole.setWeberPoly(makeSquarePoly(100, 100, 110, 110)));
  EXPECT_EQ(hole.getPolys().size(), 2u);

  XYPolygon nonconvex;
  nonconvex.add_vertex(0, 0);
  nonconvex.add_vertex(10, 0);
  nonconvex.add_vertex(5, 5);
  nonconvex.add_vertex(10, 10);
  nonconvex.add_vertex(0, 10);
  EXPECT_FALSE(hole.setMadridPoly(nonconvex));
  EXPECT_EQ(hole.getPolys().size(), 2u);
}

// Covers uSimMarine wormhole behavior: connection type ignores unknown values.
TEST(WormHoleTest, ConnectionTypeAcceptsKnownValuesOnly)
{
  WormHole hole("alpha");
  EXPECT_EQ(hole.getConnectionType(), "from_madrid");

  hole.setConnectionType("from_weber");
  EXPECT_EQ(hole.getConnectionType(), "from_weber");
  hole.setConnectionType("both");
  EXPECT_EQ(hole.getConnectionType(), "both");
  hole.setConnectionType("invalid");
  EXPECT_EQ(hole.getConnectionType(), "both");
}

// Covers uSimMarine wormhole behavior: crossing preserves centroid-relative offset.
TEST(WormHoleTest, CrossPositionPreservesCentroidRelativeOffset)
{
  WormHole hole("alpha");
  ASSERT_TRUE(hole.setMadridPoly(makeSquarePoly(0, 0, 10, 10)));
  ASSERT_TRUE(hole.setWeberPoly(makeSquarePoly(100, 100, 110, 110)));

  double wx = 0;
  double wy = 0;
  hole.crossPositionMadridToWeber(7, 5, wx, wy);
  EXPECT_NEAR(wx, 107, kGeomTol);
  EXPECT_NEAR(wy, 105, kGeomTol);

  double mx = 0;
  double my = 0;
  hole.crossPositionWeberToMadrid(103, 108, mx, my);
  EXPECT_NEAR(mx, 3, kGeomTol);
  EXPECT_NEAR(my, 8, kGeomTol);
}

// Covers uSimMarine wormhole behavior: crossing clips output to destination polygon when shapes differ.
TEST(WormHoleTest, CrossPositionClipsToDestinationPolygon)
{
  WormHole hole("alpha");
  ASSERT_TRUE(hole.setMadridPoly(makeSquarePoly(0, 0, 20, 20)));
  ASSERT_TRUE(hole.setWeberPoly(makeSquarePoly(100, 100, 104, 104)));

  double wx = 0;
  double wy = 0;
  hole.crossPositionMadridToWeber(20, 20, wx, wy);
  EXPECT_TRUE(hole.getWeberPoly().contains(wx, wy));
}

// Covers uSimMarine wormhole-set behavior: config parser normalizes tags and rejects malformed entries.
TEST(WormHoleSetTest, ConfigParserNormalizesTagsAndRejectsMalformedEntries)
{
  WormHoleSet set;
  EXPECT_FALSE(set.addWormHoleConfig("madrid_poly=pts={0,0:1,0:1,1:0,1}"));
  EXPECT_FALSE(set.addWormHoleConfig(config("bad", "sideways")));
  EXPECT_FALSE(set.addWormHoleConfig(config("bad", "from_madrid", "-1")));
  EXPECT_FALSE(set.addWormHoleConfig(config("bad") + ",unknown=value"));

  ASSERT_TRUE(set.addWormHoleConfig(config("ALPHA")));
  EXPECT_EQ(set.size(), 2u);
  EXPECT_EQ(set.getWormHole("bad").getTag(), "bad");
  EXPECT_EQ(set.getWormHole("alpha").getTag(), "alpha");
  EXPECT_EQ(set.getWormHole(99).getTag(), "");
  EXPECT_TRUE(Contains(set.getConfigSummary(), "Total WormHoles: 2"));
}

// Covers uSimMarine wormhole-set behavior: repeated tag updates the existing wormhole.
TEST(WormHoleSetTest, RepeatedTagUpdatesExistingWormHole)
{
  WormHoleSet set;
  ASSERT_TRUE(set.addWormHoleConfig(config("alpha", "from_madrid")));
  ASSERT_TRUE(set.addWormHoleConfig(config("alpha", "from_weber")));

  EXPECT_EQ(set.size(), 1u);
  EXPECT_EQ(set.getWormHole("alpha").getConnectionType(), "from_weber");
}

// Covers uSimMarine wormhole-set behavior: tunnel lifecycle fades, transports, then emerges.
TEST(WormHoleSetTest, ApplyFadesTransportsAndEmergesWithTransparencyLifecycle)
{
  WormHoleSet set;
  set.setTunnelTime(4);
  ASSERT_TRUE(set.addWormHoleConfig(config("alpha", "from_madrid")));

  double newx = -1;
  double newy = -1;
  EXPECT_FALSE(set.apply(10, 5, 5, newx, newy));
  EXPECT_NEAR(set.getTransparency(), 1, kGeomTol);

  EXPECT_FALSE(set.apply(11, 5, 5, newx, newy));
  EXPECT_NEAR(set.getTransparency(), 0.5, kGeomTol);

  EXPECT_TRUE(set.apply(12, 5, 5, newx, newy));
  EXPECT_NEAR(newx, 105, kGeomTol);
  EXPECT_NEAR(newy, 105, kGeomTol);
  EXPECT_NEAR(set.getTransparency(), 0, kGeomTol);

  EXPECT_FALSE(set.apply(13, newx, newy, newx, newy));
  EXPECT_NEAR(set.getTransparency(), 0.5, kGeomTol);

  EXPECT_FALSE(set.apply(15, newx, newy, newx, newy));
  EXPECT_NEAR(set.getTransparency(), 1, kGeomTol);
}

// Covers uSimMarine wormhole-set behavior: connection direction controls which polygon is an entry.
TEST(WormHoleSetTest, ConnectionDirectionControlsEntryPolygon)
{
  WormHoleSet set;
  set.setTunnelTime(0);
  ASSERT_TRUE(set.addWormHoleConfig(config("alpha", "from_weber")));

  double newx = -1;
  double newy = -1;
  EXPECT_FALSE(set.apply(10, 5, 5, newx, newy));
  EXPECT_TRUE(set.apply(11, 105, 105, newx, newy));
  EXPECT_NEAR(newx, 5, kGeomTol);
  EXPECT_NEAR(newy, 5, kGeomTol);
}

// Covers uSimMarine wormhole-set behavior: tunnel and transparency setters clamp to valid ranges.
TEST(WormHoleSetTest, SettersClampTunnelTimeAndTransparency)
{
  WormHoleSet set;
  set.setTunnelTime(-5);
  EXPECT_DOUBLE_EQ(set.getTunnelTime(), 0);

  set.setTransparency(-1);
  EXPECT_DOUBLE_EQ(set.getTransparency(), 0);
  set.setTransparency(2);
  EXPECT_DOUBLE_EQ(set.getTransparency(), 1);
  set.setTransparency(0.25);
  EXPECT_DOUBLE_EQ(set.getTransparency(), 0.25);
}
