#include <gtest/gtest.h>

#include <cmath>

#include "EdgeTagSet.h"
#include "RectHullGenerator.h"
#include "SurveyGenerator.h"
#include "XYPolygon.h"
#include "XYSegList.h"

namespace {

XYPolygon rectangle(double xmin, double ymin, double xmax, double ymax)
{
  XYPolygon poly;
  poly.add_vertex(xmin, ymin);
  poly.add_vertex(xmax, ymin);
  poly.add_vertex(xmax, ymax);
  poly.add_vertex(xmin, ymax);
  return poly;
}

XYPolygon triangleSurveyRegion()
{
  XYPolygon poly;
  poly.add_vertex(0, 0);
  poly.add_vertex(80, 0);
  poly.add_vertex(20, 50);
  return poly;
}

bool allLanePairsTagged(const XYSegList& segl)
{
  EdgeTagSet tags = segl.get_edge_tags();
  for(unsigned int i = 0; i + 1 < segl.size(); i += 2) {
    if(!tags.matches(i, i + 1, "lane"))
      return false;
  }
  return true;
}

}  // namespace

// Covers rect hull generator behavior: builds axis aligned bounds for mission regions.
TEST(RectHullGeneratorTest, BuildsAxisAlignedBoundsForMissionRegions)
{
  RectHullGenerator generator;
  generator.addPoint(-35, -15);
  generator.addPoint(35, -15);
  generator.addPoint(35, 15);
  generator.addPoint(-35, 15);

  XYPolygon hull = generator.generateRectHull();
  ASSERT_EQ(hull.size(), 4u);
  EXPECT_TRUE(hull.is_convex());
  EXPECT_DOUBLE_EQ(generator.getPolyWidth(), 70);
  EXPECT_DOUBLE_EQ(generator.getPolyHeight(), 30);
  EXPECT_DOUBLE_EQ(hull.get_min_x(), -35);
  EXPECT_DOUBLE_EQ(hull.get_max_x(), 35);
  EXPECT_DOUBLE_EQ(hull.get_min_y(), -15);
  EXPECT_DOUBLE_EQ(hull.get_max_y(), 15);
  EXPECT_TRUE(hull.contains(0, 0));
}

// Covers rect hull generator behavior: normalizes quarter turn hull dimensions.
TEST(RectHullGeneratorTest, NormalizesQuarterTurnHullDimensions)
{
  RectHullGenerator generator;
  generator.setRotationAngle(270);
  generator.addPoint(0, 0);
  generator.addPoint(70, 0);
  generator.addPoint(70, 30);
  generator.addPoint(0, 30);

  XYPolygon hull = generator.generateRectHull();
  ASSERT_EQ(hull.size(), 4u);
  EXPECT_TRUE(hull.is_convex());
  EXPECT_DOUBLE_EQ(generator.getPolyWidth(), 30);
  EXPECT_DOUBLE_EQ(generator.getPolyHeight(), 70);
  EXPECT_DOUBLE_EQ(hull.get_min_x(), 0);
  EXPECT_DOUBLE_EQ(hull.get_max_x(), 70);
  EXPECT_DOUBLE_EQ(hull.get_min_y(), 0);
  EXPECT_DOUBLE_EQ(hull.get_max_y(), 30);
}

// Covers rect hull generator behavior: treats opposite oblique angles as same hull.
TEST(RectHullGeneratorTest, TreatsOppositeObliqueAnglesAsSameHull)
{
  RectHullGenerator forty_five;
  RectHullGenerator two_twenty_five;
  for(const auto& pt : {XYPoint(0, 0), XYPoint(70, 0),
                        XYPoint(70, 30), XYPoint(0, 30)}) {
    forty_five.addPoint(pt);
    two_twenty_five.addPoint(pt);
  }
  forty_five.setRotationAngle(45);
  two_twenty_five.setRotationAngle(225);

  XYPolygon hull_a = forty_five.generateRectHull();
  XYPolygon hull_b = two_twenty_five.generateRectHull();
  ASSERT_EQ(hull_a.size(), 4u);
  ASSERT_EQ(hull_b.size(), 4u);
  EXPECT_NEAR(forty_five.getPolyWidth(), two_twenty_five.getPolyWidth(), 1e-9);
  EXPECT_NEAR(forty_five.getPolyHeight(), two_twenty_five.getPolyHeight(), 1e-9);
  EXPECT_NEAR(hull_a.area(), hull_b.area(), 1e-9);
}

// Covers rect hull generator behavior: normalizes negative and over full rotation angles.
TEST(RectHullGeneratorTest, NormalizesNegativeAndOverFullRotationAngles)
{
  RectHullGenerator negative_ninety;
  RectHullGenerator four_fifty;
  for(const auto& pt : {XYPoint(-10, -5), XYPoint(60, -5),
                        XYPoint(60, 25), XYPoint(-10, 25)}) {
    negative_ninety.addPoint(pt);
    four_fifty.addPoint(pt);
  }
  negative_ninety.setRotationAngle(-90);
  four_fifty.setRotationAngle(450);

  XYPolygon hull_a = negative_ninety.generateRectHull();
  XYPolygon hull_b = four_fifty.generateRectHull();
  ASSERT_EQ(hull_a.size(), 4u);
  ASSERT_EQ(hull_b.size(), 4u);
  EXPECT_TRUE(hull_a.is_convex());
  EXPECT_TRUE(hull_b.is_convex());
  EXPECT_DOUBLE_EQ(negative_ninety.getPolyWidth(), 30);
  EXPECT_DOUBLE_EQ(negative_ninety.getPolyHeight(), 70);
  EXPECT_DOUBLE_EQ(four_fifty.getPolyWidth(), 30);
  EXPECT_DOUBLE_EQ(four_fifty.getPolyHeight(), 70);
}

// Covers survey generator behavior: defaults setters and toggles back interactive controls.
TEST(SurveyGeneratorTest, DefaultsSettersAndTogglesBackInteractiveControls)
{
  SurveyGenerator generator;
  EXPECT_TRUE(generator.getPackLanes());
  EXPECT_TRUE(generator.getDropLane());
  EXPECT_FALSE(generator.getCoreWidth());
  EXPECT_FALSE(generator.getAutoShift());
  EXPECT_FALSE(generator.getAutoDense());
  EXPECT_DOUBLE_EQ(generator.getLaneAngle(), 0);
  EXPECT_DOUBLE_EQ(generator.getLaneWidth(), 20);

  generator.setLaneWidth(0.25);
  generator.setLaneAngle(-10);
  EXPECT_DOUBLE_EQ(generator.getLaneWidth(), 1);
  EXPECT_DOUBLE_EQ(generator.getLaneAngle(), 350);

  generator.togglePackLanes();
  generator.toggleDropLane();
  generator.toggleCoreWidth();
  generator.toggleAutoShift();
  generator.toggleAutoDense();
  EXPECT_FALSE(generator.getPackLanes());
  EXPECT_FALSE(generator.getDropLane());
  EXPECT_TRUE(generator.getCoreWidth());
  EXPECT_TRUE(generator.getAutoShift());
  EXPECT_TRUE(generator.getAutoDense());
}

// Covers survey generator behavior: rejects non convex regions and keeps existing region.
TEST(SurveyGeneratorTest, RejectsNonConvexRegionsAndKeepsExistingRegion)
{
  SurveyGenerator generator;
  XYPolygon convex = rectangle(0, 0, 30, 20);
  generator.setRegion(convex);
  ASSERT_EQ(generator.getRegion().size(), 4u);

  XYPolygon concave;
  concave.add_vertex(0, 0);
  concave.add_vertex(30, 0);
  concave.add_vertex(10, 10);
  concave.add_vertex(30, 20);
  concave.add_vertex(0, 20);
  ASSERT_FALSE(concave.is_convex());

  generator.setRegion(concave);
  EXPECT_EQ(generator.getRegion().size(), 4u);
  EXPECT_DOUBLE_EQ(generator.getRegion().area(), convex.area());
}

// Covers survey generator behavior: generates pMarineViewer lawnmower survey update pattern.
TEST(SurveyGeneratorTest, GeneratesPMarineViewerLawnmowerSurveyUpdatePattern)
{
  SurveyGenerator generator;
  generator.setRegion(rectangle(-35, -15, 35, 15));
  generator.setEntryPoint(0, 0);
  generator.setLaneWidth(8);
  generator.setLaneAngle(80);

  generator.generate();
  XYSegList survey = generator.getSurvey();
  XYPolygon hull = generator.getReHull();

  EXPECT_GT(survey.size(), 0u);
  EXPECT_EQ(survey.size() % 2, 0u);
  EXPECT_EQ(generator.getLaneCount(), survey.size() / 2);
  EXPECT_GT(generator.getSurveyLen(), 0);
  EXPECT_TRUE(hull.is_convex());
  EXPECT_GT(hull.area(), generator.getRegion().area());
  EXPECT_TRUE(allLanePairsTagged(survey));
}

// Covers survey generator behavior: auto dense tightens swath to divide rectangular hull.
TEST(SurveyGeneratorTest, AutoDenseTightensSwathToDivideRectangularHull)
{
  SurveyGenerator generator;
  generator.setRegion(rectangle(-35, -15, 35, 15));
  generator.setLaneWidth(20);
  generator.setLaneAngle(0);
  generator.setPackLanes(false);
  generator.setDropLane(false);
  generator.setAutoDense(true);

  generator.generate();
  EXPECT_NEAR(generator.getLaneWidthX(), 17.5, 1e-9);
  EXPECT_EQ(generator.getLaneCount(), 5u);
  EXPECT_EQ(generator.size(), 10u);
  EXPECT_TRUE(allLanePairsTagged(generator.getSurvey()));
}

// Covers survey generator behavior: auto dense keeps exact divisor swath width.
TEST(SurveyGeneratorTest, AutoDenseKeepsExactDivisorSwathWidth)
{
  SurveyGenerator generator;
  generator.setRegion(rectangle(-40, -10, 40, 10));
  generator.setLaneWidth(20);
  generator.setLaneAngle(0);
  generator.setPackLanes(false);
  generator.setDropLane(false);
  generator.setAutoDense(true);

  generator.generate();
  EXPECT_DOUBLE_EQ(generator.getLaneWidthX(), 20);
  EXPECT_GT(generator.size(), 0u);
  EXPECT_TRUE(allLanePairsTagged(generator.getSurvey()));
}

// Covers survey generator behavior: start point changes generated route ordering for viewer updates.
TEST(SurveyGeneratorTest, StartPointChangesGeneratedRouteOrderingForViewerUpdates)
{
  SurveyGenerator near_west;
  near_west.setRegion(rectangle(-40, -20, 40, 20));
  near_west.setEntryPoint(-100, 0);
  near_west.setLaneWidth(10);
  near_west.setLaneAngle(0);
  near_west.generate();

  SurveyGenerator near_east;
  near_east.setRegion(rectangle(-40, -20, 40, 20));
  near_east.setEntryPoint(100, 0);
  near_east.setLaneWidth(10);
  near_east.setLaneAngle(0);
  near_east.generate();

  ASSERT_GT(near_west.size(), 0u);
  ASSERT_GT(near_east.size(), 0u);
  EXPECT_NE(near_west.getSurveySpec(), near_east.getSurveySpec());
  EXPECT_TRUE(allLanePairsTagged(near_west.getSurvey()));
  EXPECT_TRUE(allLanePairsTagged(near_east.getSurvey()));
}

// Covers survey generator behavior: packing and dropping trim hull lanes to convex region.
TEST(SurveyGeneratorTest, PackingAndDroppingTrimHullLanesToConvexRegion)
{
  SurveyGenerator raw;
  raw.setRegion(triangleSurveyRegion());
  raw.setLaneWidth(10);
  raw.setLaneAngle(0);
  raw.setPackLanes(false);
  raw.setDropLane(false);
  raw.generate();

  SurveyGenerator trimmed;
  trimmed.setRegion(triangleSurveyRegion());
  trimmed.setLaneWidth(10);
  trimmed.setLaneAngle(0);
  trimmed.setPackLanes(true);
  trimmed.setDropLane(true);
  trimmed.generate();

  EXPECT_GT(raw.size(), 0u);
  EXPECT_GT(trimmed.size(), 0u);
  EXPECT_LE(trimmed.size(), raw.size());
  EXPECT_LT(trimmed.getSurveyLen(), raw.getSurveyLen());
  EXPECT_TRUE(allLanePairsTagged(trimmed.getSurvey()));
}
