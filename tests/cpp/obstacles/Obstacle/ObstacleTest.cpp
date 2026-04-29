#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Obstacle.h"
#include "XYFormatUtilsPoly.h"
#include "XYPoint.h"
#include "XYPolygon.h"

namespace {

bool contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

XYPolygon squarePoly(const std::string& label = "")
{
  XYPolygon poly;
  poly.add_vertex(0, 0);
  poly.add_vertex(20, 0);
  poly.add_vertex(20, 20);
  poly.add_vertex(0, 20);
  poly.set_label(label);
  return poly;
}

}  // namespace

TEST(ObstacleTest, TracksDefaultStateRangeMinimumAndTimeToLive)
{
  Obstacle obstacle;
  EXPECT_EQ(obstacle.size(), 0u);
  EXPECT_EQ(obstacle.getPtsTotal(), 0u);
  EXPECT_FALSE(obstacle.hasChanged());
  EXPECT_FALSE(obstacle.isGiven());
  EXPECT_DOUBLE_EQ(obstacle.getRange(), 0);
  EXPECT_DOUBLE_EQ(obstacle.getMinRange(), -1);
  EXPECT_DOUBLE_EQ(obstacle.getDuration(), -1);
  EXPECT_DOUBLE_EQ(obstacle.getTimeToLive(100), -1);
  EXPECT_TRUE(contains(obstacle.getInfo(100), "given=false"));
  EXPECT_TRUE(contains(obstacle.getInfo(100), "duration=-1"));

  obstacle.setRange(75);
  obstacle.setRange(50);
  obstacle.setRange(60);
  EXPECT_DOUBLE_EQ(obstacle.getRange(), 60);
  EXPECT_DOUBLE_EQ(obstacle.getMinRange(), 50);

  obstacle.setDuration(30);
  obstacle.setTStamp(100);
  EXPECT_DOUBLE_EQ(obstacle.getTimeToLive(110), 20);
  EXPECT_DOUBLE_EQ(obstacle.getTimeToLive(140), 0);
}

TEST(ObstacleTest, StoresRecentObstaclePointsNewestFirstAndPrunesByAge)
{
  Obstacle obstacle;
  obstacle.setMaxPts(2);

  XYPoint old(1, 1, "old");
  old.set_time(10);
  XYPoint mid(2, 2, "mid");
  mid.set_time(20);
  XYPoint newest(3, 3, "newest");
  newest.set_time(30);

  EXPECT_TRUE(obstacle.addPoint(old));
  EXPECT_TRUE(obstacle.addPoint(mid));
  EXPECT_TRUE(obstacle.addPoint(newest));
  EXPECT_TRUE(obstacle.hasChanged());
  EXPECT_EQ(obstacle.size(), 2u);
  EXPECT_EQ(obstacle.getPtsTotal(), 3u);
  EXPECT_DOUBLE_EQ(obstacle.getDuration(), 0);

  std::vector<XYPoint> points = obstacle.getPoints();
  ASSERT_EQ(points.size(), 2u);
  EXPECT_EQ(points[0].get_label(), "newest");
  EXPECT_EQ(points[1].get_label(), "mid");

  obstacle.setChanged(false);
  EXPECT_FALSE(obstacle.pruneByAge(15, 34));
  EXPECT_FALSE(obstacle.hasChanged());
  EXPECT_EQ(obstacle.size(), 2u);

  EXPECT_FALSE(obstacle.pruneByAge(10, 40));
  EXPECT_TRUE(obstacle.hasChanged());
  ASSERT_EQ(obstacle.getPoints().size(), 1u);
  EXPECT_EQ(obstacle.getPoints()[0].get_label(), "newest");

  EXPECT_TRUE(obstacle.pruneByAge(1, 42));
  EXPECT_EQ(obstacle.size(), 0u);
}

TEST(ObstacleTest, AcceptsGivenConvexViewPolygonAndSkipsRoundedNoopUpdates)
{
  Obstacle obstacle;
  XYPolygon poly = squarePoly("obstacle_1");

  EXPECT_TRUE(obstacle.setPoly(poly));
  EXPECT_TRUE(obstacle.isGiven());
  EXPECT_TRUE(obstacle.hasChanged());
  EXPECT_EQ(obstacle.getUpdatesTotal(), 1u);
  EXPECT_EQ(obstacle.getPoly().get_label(), "obstacle_1");

  obstacle.setChanged(false);
  XYPolygon nearly_same = poly;
  nearly_same.alter_vertex(20.02, 20.02, 0);
  EXPECT_TRUE(obstacle.setPoly(nearly_same));
  EXPECT_FALSE(obstacle.hasChanged());
  EXPECT_EQ(obstacle.getUpdatesTotal(), 1u);

  XYPolygon changed = poly;
  changed.grow_by_amt(2);
  EXPECT_TRUE(obstacle.setPoly(changed));
  EXPECT_TRUE(obstacle.hasChanged());
  EXPECT_EQ(obstacle.getUpdatesTotal(), 2u);
}

TEST(ObstacleTest, RejectsNonConvexGivenPolygons)
{
  Obstacle obstacle;
  XYPolygon concave;
  concave.add_vertex(0, 0, false);
  concave.add_vertex(10, 0, false);
  concave.add_vertex(5, 5, false);
  concave.add_vertex(10, 10, false);
  concave.add_vertex(0, 10, false);
  concave.determine_convexity();
  ASSERT_FALSE(concave.is_convex());

  EXPECT_FALSE(obstacle.setPoly(concave));
  EXPECT_FALSE(obstacle.isGiven());
  EXPECT_EQ(obstacle.getUpdatesTotal(), 0u);
}

TEST(ObstacleTest, PrunesGivenObstaclesOnlyWhenDurationExpires)
{
  Obstacle obstacle;
  ASSERT_TRUE(obstacle.setPoly(squarePoly("given")));
  obstacle.setDuration(-1);
  obstacle.setTStamp(100);
  EXPECT_FALSE(obstacle.pruneByAge(5, 1000));

  obstacle.setDuration(30);
  EXPECT_FALSE(obstacle.pruneByAge(5, 129.9));
  EXPECT_TRUE(obstacle.pruneByAge(5, 131));
}
