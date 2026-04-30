#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "VPlug_GeoShapes.h"

namespace {

const char* kBoxSpec =
    "pts={0,0:20,0:20,10:0,10},label=safety_box,type=opregion";

const char* kGridSpec =
    "pts={0,0:20,0:20,20:0,20},label=search_grid@10,10@7";

const char* kConvexGridSpec =
    "pts={0,0:20,0:20,20:0,20},cell_size=10,"
    "cell_vars=visited:0:score:5,label=convex_search";

void ExpectMapHasLabel(const std::map<std::string, XYPoint>& points,
                       const std::string& label)
{
  EXPECT_NE(points.find(label), points.end());
}

}  // namespace

// Covers VPlug geo shapes lifecycle behavior: defaults are empty with zero bounds.
TEST(VPlugGeoShapesLifecycleTest, DefaultsAreEmptyWithZeroBounds)
{
  VPlug_GeoShapes shapes;

  EXPECT_EQ(shapes.sizePolygons(), 0u);
  EXPECT_EQ(shapes.sizeSegLists(), 0u);
  EXPECT_EQ(shapes.sizeSeglrs(), 0u);
  EXPECT_EQ(shapes.sizeCircles(), 0u);
  EXPECT_EQ(shapes.sizePoints(), 0u);
  EXPECT_EQ(shapes.sizeVectors(), 0u);
  EXPECT_EQ(shapes.sizeGrids(), 0u);
  EXPECT_EQ(shapes.sizeConvexGrids(), 0u);
  EXPECT_EQ(shapes.sizeRangePulses(), 0u);
  EXPECT_EQ(shapes.sizeCommsPulses(), 0u);
  EXPECT_EQ(shapes.sizeMarkers(), 0u);
  EXPECT_EQ(shapes.sizeTextBoxes(), 0u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 0u);
  EXPECT_NEAR(shapes.getXMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getXMax(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMax(), 0.0, kGeomTol);
}

// Covers VPlug geo shapes lifecycle behavior: polygon and vector payloads replace by label.
TEST(VPlugGeoShapesLifecycleTest, PolygonAndVectorPayloadsReplaceByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPolygon(kBoxSpec, 10.0));
  ASSERT_EQ(shapes.sizePolygons(), 1u);
  EXPECT_EQ(shapes.getPolygon(0).get_label(), "safety_box");
  EXPECT_NEAR(shapes.getPolygon(0).get_time(), 10.0, kGeomTol);

  ASSERT_TRUE(shapes.addPolygon(
      "pts={100,0:120,0:120,10:100,10},label=safety_box,type=opregion",
      20.0));
  ASSERT_EQ(shapes.sizePolygons(), 1u);
  EXPECT_NEAR(shapes.getPolygon(0).get_min_x(), 100.0, kGeomTol);
  EXPECT_NEAR(shapes.getPolygon(0).get_time(), 20.0, kGeomTol);

  ASSERT_TRUE(shapes.addVector("x=1,y=2,mag=4,ang=90,label=drift"));
  ASSERT_TRUE(shapes.addVector("x=8,y=9,mag=2,ang=180,label=drift"));
  ASSERT_EQ(shapes.getVectors().size(), 1u);
  EXPECT_NEAR(shapes.getVectors()[0].xpos(), 8.0, kGeomTol);
  EXPECT_NEAR(shapes.getVectors()[0].ypos(), 9.0, kGeomTol);
  EXPECT_EQ(shapes.getVectors()[0].get_label(), "drift");
}

// Covers VPlug geo shapes lifecycle behavior: map backed shapes auto label and replace by label.
TEST(VPlugGeoShapesLifecycleTest, MapBackedShapesAutoLabelAndReplaceByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPoint("x=1,y=2,type=waypoint"));
  ASSERT_TRUE(shapes.addPoint("x=3,y=4,type=waypoint"));
  ASSERT_EQ(shapes.getPoints().size(), 2u);
  ExpectMapHasLabel(shapes.getPoints(), "pt_0");
  ExpectMapHasLabel(shapes.getPoints(), "pt_1");
  EXPECT_EQ(shapes.getPoints().find("pt_0")->second.get_label(), "");

  ASSERT_TRUE(shapes.addPoint("x=5,y=6,label=station,type=waypoint"));
  ASSERT_TRUE(shapes.addPoint("x=7,y=8,label=station,type=waypoint"));
  ASSERT_EQ(shapes.getPoints().size(), 3u);
  EXPECT_NEAR(shapes.getPoints().find("station")->second.x(), 7.0, kGeomTol);

  ASSERT_TRUE(shapes.addMarker("x=10,y=11,type=circle,width=2"));
  ASSERT_TRUE(shapes.addCircle("x=20,y=21,radius=5"));
  ASSERT_EQ(shapes.getMarkers().size(), 1u);
  ASSERT_EQ(shapes.getCircles().size(), 1u);
  EXPECT_NE(shapes.getMarkers().find("marker_0"), shapes.getMarkers().end());
  EXPECT_NE(shapes.getCircles().find("0"), shapes.getCircles().end());

  ASSERT_TRUE(shapes.addMarker("x=12,y=13,type=diamond,label=mark"));
  ASSERT_TRUE(shapes.addMarker("x=14,y=15,type=square,label=mark"));
  ASSERT_EQ(shapes.getMarkers().size(), 2u);
  EXPECT_NEAR(shapes.getMarkers().find("mark")->second.get_vx(), 14.0,
              kGeomTol);
}

// Covers VPlug geo shapes lifecycle behavior: seg lists use separate map storage excluded from size.
TEST(VPlugGeoShapesLifecycleTest, SegListsUseSeparateMapStorageExcludedFromSize)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addSegList(
      "pts={0,0:10,0:10,10},label=route,type=survey_lane"));
  ASSERT_EQ(shapes.getSegLists().size(), 1u);
  // addSegList now stores in m_segls, but sizeSegLists still reports the
  // unused vector-backed m_seglists collection.
  EXPECT_EQ(shapes.sizeSegLists(), 0u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 0u);

  ASSERT_TRUE(shapes.addSegList(
      "pts={5,5:15,5:15,15},label=route,type=survey_lane"));
  ASSERT_EQ(shapes.getSegLists().size(), 1u);
  EXPECT_NEAR(shapes.getSegLists().find("route")->second.get_vx(0), 5.0,
              kGeomTol);

  ASSERT_TRUE(shapes.addSegList("pts={1,1:2,2}"));
  ASSERT_EQ(shapes.getSegLists().size(), 2u);
  EXPECT_NE(shapes.getSegLists().find("seglx"), shapes.getSegLists().end());
}

// Covers VPlug geo shapes lifecycle behavior: seglrs use map storage included in size.
TEST(VPlugGeoShapesLifecycleTest, SeglrsUseMapStorageIncludedInSize)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addSeglr(
      "pts={0,0:10,0},ray=45,ray_len=20,label=turn_ray,type=path_ray"));
  ASSERT_EQ(shapes.getSeglrs().size(), 1u);
  EXPECT_EQ(shapes.sizeSeglrs(), 1u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 1u);

  ASSERT_TRUE(shapes.addSeglr(
      "pts={5,5:15,5},ray=90,ray_len=10,label=turn_ray,type=path_ray"));
  ASSERT_EQ(shapes.getSeglrs().size(), 1u);
  EXPECT_NEAR(shapes.getSeglrs().find("turn_ray")->second.getVX(0), 5.0,
              kGeomTol);

  ASSERT_TRUE(shapes.addSeglr("pts={0,0},ray=180,active=false,label=turn_ray"));
  EXPECT_TRUE(shapes.getSeglrs().empty());
  EXPECT_EQ(shapes.sizeTotalShapes(), 0u);
}

// Covers VPlug geo shapes lifecycle behavior: active false payloads delete existing shapes by label.
TEST(VPlugGeoShapesLifecycleTest, ActiveFalsePayloadsDeleteExistingShapesByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPolygon(kBoxSpec));
  ASSERT_TRUE(shapes.addPoint("x=1,y=2,label=station"));
  ASSERT_TRUE(shapes.addCircle("x=1,y=2,radius=3,label=ring"));
  ASSERT_TRUE(shapes.addVector("x=1,y=2,mag=3,ang=4,label=flow"));
  ASSERT_TRUE(shapes.addRangePulse("x=1,y=2,radius=3,label=ping"));
  ASSERT_TRUE(shapes.addCommsPulse("sx=0,sy=0,tx=10,ty=20,label=link"));
  ASSERT_TRUE(shapes.addMarker("x=1,y=2,type=circle,label=mark"));
  ASSERT_EQ(shapes.sizeTotalShapes(), 7u);

  EXPECT_TRUE(shapes.addPolygon("label=safety_box,active=false"));
  EXPECT_TRUE(shapes.addPoint("x=0,y=0,label=station,active=false"));
  EXPECT_TRUE(shapes.addCircle("x=0,y=0,radius=0,label=ring,active=false"));
  EXPECT_TRUE(shapes.addVector("x=0,y=0,mag=0,ang=0,label=flow,active=false"));
  EXPECT_TRUE(shapes.addRangePulse("x=0,y=0,label=ping,active=false"));
  EXPECT_TRUE(shapes.addCommsPulse(
      "sx=0,sy=0,tx=1,ty=1,label=link,active=false"));
  EXPECT_TRUE(shapes.addMarker("x=0,y=0,type=circle,label=mark,active=false"));

  EXPECT_EQ(shapes.sizePolygons(), 0u);
  EXPECT_TRUE(shapes.getPoints().empty());
  EXPECT_TRUE(shapes.getCircles().empty());
  EXPECT_TRUE(shapes.getVectors().empty());
  EXPECT_TRUE(shapes.getRangePulses().empty());
  EXPECT_TRUE(shapes.getCommsPulses().empty());
  EXPECT_TRUE(shapes.getMarkers().empty());
  EXPECT_EQ(shapes.sizeTotalShapes(), 0u);
}

// Covers VPlug geo shapes lifecycle behavior: duration expiry only visits some collections.
TEST(VPlugGeoShapesLifecycleTest, DurationExpiryOnlyVisitsSomeCollections)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPolygon(
      "pts={0,0:10,0:10,10:0,10},label=poly,duration=5", 100.0));
  ASSERT_TRUE(shapes.addPoint("x=1,y=2,label=point,duration=5", 100.0));
  ASSERT_TRUE(shapes.addMarker("x=1,y=2,type=circle,label=mark,duration=5",
                               100.0));
  ASSERT_TRUE(shapes.addCircle("x=1,y=2,radius=3,label=ring,duration=5",
                               18, 100.0));
  ASSERT_TRUE(shapes.addVector("x=1,y=2,mag=3,ang=4,label=flow,duration=5"));
  ASSERT_TRUE(shapes.addRangePulse("x=1,y=2,radius=3,label=ping,duration=5",
                                   100.0));
  ASSERT_TRUE(shapes.addCommsPulse(
      "sx=0,sy=0,tx=10,ty=20,label=link,duration=5", 100.0));

  shapes.manageMemory(104.9);
  EXPECT_EQ(shapes.sizeTotalShapes(), 7u);

  shapes.manageMemory(105.1);
  EXPECT_EQ(shapes.sizePolygons(), 0u);
  EXPECT_TRUE(shapes.getPoints().empty());
  EXPECT_TRUE(shapes.getMarkers().empty());
  EXPECT_TRUE(shapes.getCircles().empty());
  // Vectors and both pulse vectors are not scanned by manageMemory.
  EXPECT_EQ(shapes.getVectors().size(), 1u);
  EXPECT_EQ(shapes.getRangePulses().size(), 1u);
  EXPECT_EQ(shapes.getCommsPulses().size(), 1u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 3u);
}

// Covers VPlug geo shapes lifecycle behavior: total size excludes map backed seg lists.
TEST(VPlugGeoShapesLifecycleTest, TotalSizeExcludesMapBackedSegLists)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPolygon(kBoxSpec));
  ASSERT_TRUE(shapes.addSegList("pts={0,0:1,1},label=route"));
  ASSERT_TRUE(shapes.addSeglr("pts={0,0},ray=45,label=ray"));
  ASSERT_TRUE(shapes.addPoint("x=1,y=2,label=point"));
  ASSERT_TRUE(shapes.addMarker("x=1,y=2,type=circle,label=mark"));
  ASSERT_TRUE(shapes.addCircle("x=1,y=2,radius=3,label=ring"));
  ASSERT_TRUE(shapes.addVector("x=1,y=2,mag=3,ang=4,label=flow"));
  ASSERT_TRUE(shapes.addGrid(kGridSpec));
  ASSERT_TRUE(shapes.addConvexGrid(kConvexGridSpec));
  ASSERT_TRUE(shapes.addRangePulse("x=1,y=2,radius=3,label=ping"));
  ASSERT_TRUE(shapes.addCommsPulse("sx=0,sy=0,tx=10,ty=20,label=link"));

  ASSERT_EQ(shapes.getSegLists().size(), 1u);
  EXPECT_EQ(shapes.sizeSegLists(), 0u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 10u);
}

// Covers VPlug geo shapes lifecycle behavior: clear without args leaves several live collections.
TEST(VPlugGeoShapesLifecycleTest, ClearWithoutArgsLeavesSeveralLiveCollections)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addSegList("pts={0,0:1,1},label=route"));
  ASSERT_TRUE(shapes.addConvexGrid(kConvexGridSpec));
  ASSERT_TRUE(shapes.addCommsPulse("sx=0,sy=0,tx=10,ty=20,label=link"));
  ASSERT_TRUE(shapes.addPoint("x=1,y=2,label=point"));
  ASSERT_TRUE(shapes.addCircle("x=1,y=2,radius=3,label=ring"));

  shapes.clear();

  // clear() resets many legacy containers, but does not clear m_segls,
  // m_convex_grids, or m_comms_pulses.
  EXPECT_TRUE(shapes.getPoints().empty());
  EXPECT_TRUE(shapes.getCircles().empty());
  EXPECT_EQ(shapes.getSegLists().size(), 1u);
  EXPECT_EQ(shapes.getConvexGrids().size(), 1u);
  EXPECT_EQ(shapes.getCommsPulses().size(), 1u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 2u);
  EXPECT_NEAR(shapes.getXMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getXMax(), 0.0, kGeomTol);
}

// Covers VPlug geo shapes lifecycle behavior: public clear recognizes only subset of shape names.
TEST(VPlugGeoShapesLifecycleTest, PublicClearRecognizesOnlySubsetOfShapeNames)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPoint("x=1,y=2,label=point"));
  ASSERT_TRUE(shapes.addCircle("x=1,y=2,radius=3,label=ring"));
  ASSERT_TRUE(shapes.addMarker("x=1,y=2,type=circle,label=mark"));
  ASSERT_TRUE(shapes.addVector("x=1,y=2,mag=3,ang=4,label=flow"));

  shapes.clear("points");
  EXPECT_TRUE(shapes.getPoints().empty());

  shapes.clear("circles");
  shapes.clear("markers");
  shapes.clear("vectors");
  EXPECT_EQ(shapes.getCircles().size(), 1u);
  EXPECT_EQ(shapes.getMarkers().size(), 1u);
  EXPECT_EQ(shapes.getVectors().size(), 1u);

  EXPECT_TRUE(shapes.setParam("clear", "circles"));
  EXPECT_FALSE(shapes.setParam("clear", "markers"));
  EXPECT_TRUE(shapes.setParam("clear", "vectors"));
  EXPECT_TRUE(shapes.getCircles().empty());
  EXPECT_EQ(shapes.getMarkers().size(), 1u);
  EXPECT_TRUE(shapes.getVectors().empty());
}

// Covers VPlug geo shapes lifecycle behavior: set param clear swaps polygon and seglist targets.
TEST(VPlugGeoShapesLifecycleTest, SetParamClearSwapsPolygonAndSeglistTargets)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPolygon(kBoxSpec));
  ASSERT_TRUE(shapes.addSegList("pts={0,0:10,0},label=route"));

  EXPECT_TRUE(shapes.setParam("clear", "seglists"));
  EXPECT_EQ(shapes.sizePolygons(), 0u);
  EXPECT_EQ(shapes.getSegLists().size(), 1u);

  ASSERT_TRUE(shapes.addPolygon(kBoxSpec));
  EXPECT_TRUE(shapes.setParam("clear", "polygons"));
  EXPECT_EQ(shapes.sizePolygons(), 1u);
  EXPECT_EQ(shapes.getSegLists().size(), 1u);
}

// Covers VPlug geo shapes lifecycle behavior: type filtered clear retains matching types.
TEST(VPlugGeoShapesLifecycleTest, TypeFilteredClearRetainsMatchingTypes)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addPoint("x=1,y=2,label=survey,type=survey"));
  ASSERT_TRUE(shapes.addPoint("x=3,y=4,label=nav,type=nav"));

  shapes.clear("points", "survey");

  ASSERT_EQ(shapes.getPoints().size(), 1u);
  EXPECT_NE(shapes.getPoints().find("survey"), shapes.getPoints().end());
}

// Covers VPlug geo shapes lifecycle behavior: bounds only expand until full clear.
TEST(VPlugGeoShapesLifecycleTest, BoundsOnlyExpandUntilFullClear)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addCircle("x=100,y=50,radius=5,label=ring"));
  EXPECT_NEAR(shapes.getXMax(), 105.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMax(), 55.0, kGeomTol);

  ASSERT_TRUE(shapes.addCircle("x=0,y=0,radius=1,label=ring"));
  ASSERT_EQ(shapes.getCircles().size(), 1u);
  EXPECT_NEAR(shapes.getCircles().find("ring")->second.get_max_x(), 1.0,
              kGeomTol);
  EXPECT_NEAR(shapes.getXMax(), 105.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMax(), 55.0, kGeomTol);

  shapes.clear();
  EXPECT_NEAR(shapes.getXMax(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMax(), 0.0, kGeomTol);
}

// Covers VPlug geo shapes lifecycle behavior: invalid payload return values match current api.
TEST(VPlugGeoShapesLifecycleTest, InvalidPayloadReturnValuesMatchCurrentApi)
{
  VPlug_GeoShapes shapes;

  EXPECT_TRUE(shapes.addPoint("x=bad,y=2,label=bad_point"));
  ASSERT_EQ(shapes.getPoints().size(), 1u);
  EXPECT_NEAR(shapes.getPoints().find("bad_point")->second.x(), 0.0,
              kGeomTol);

  EXPECT_TRUE(shapes.addVector("x=bad,y=2,mag=3,ang=4,label=bad_vector"));
  EXPECT_EQ(shapes.getVectors().size(), 1u);
  EXPECT_FALSE(shapes.getVectors()[0].valid());

  EXPECT_FALSE(shapes.addCircle("x=1,y=2,radius=bad,label=bad_circle"));
  EXPECT_TRUE(shapes.getCircles().empty());
}

// Covers VPlug geo shapes lifecycle behavior: grid replacement and updates are label driven.
TEST(VPlugGeoShapesLifecycleTest, GridReplacementAndUpdatesAreLabelDriven)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addGrid(kGridSpec));
  ASSERT_TRUE(shapes.addGrid(
      "pts={10,10:30,10:30,30:10,30},label=search_grid@10,10@2"));
  ASSERT_EQ(shapes.getGrids().size(), 1u);
  EXPECT_EQ(shapes.getGrids()[0].getLabel(), "search_grid");
  EXPECT_NEAR(shapes.getGrids()[0].getVal(0), 2.0, kGeomTol);

  ASSERT_TRUE(shapes.updateGrid("search_grid@0,2,9"));
  EXPECT_NEAR(shapes.getGrids()[0].getVal(0), 9.0, kGeomTol);

  ASSERT_TRUE(shapes.addConvexGrid(kConvexGridSpec));
  ASSERT_TRUE(shapes.addConvexGrid(
      "pts={10,10:30,10:30,30:10,30},cell_size=10,"
      "cell_vars=visited:0:score:5,label=convex_search"));
  ASSERT_EQ(shapes.getConvexGrids().size(), 1u);
  EXPECT_EQ(shapes.getConvexGrids()[0].get_label(), "convex_search");

  ASSERT_TRUE(shapes.updateConvexGrid("convex_search@replace@0,score,12"));
  EXPECT_NEAR(shapes.getConvexGrids()[0].getVal(
                  0, shapes.getConvexGrids()[0].getCellVarIX("score")),
              12.0, kGeomTol);
}

// Covers VPlug geo shapes lifecycle behavior: set param routes marine viewer grid and marker aliases.
TEST(VPlugGeoShapesLifecycleTest, SetParamRoutesMarineViewerGridAndMarkerAliases)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.setParam(
      "convex_grid",
      "pts={-50,-40:-10,0:90,0:90,-90:50,-150:-50,-90},"
      "cell_size=5,cell_vars=x:0:y:0:z:0,cell_min=x:0,"
      "cell_max=x:50,cell=211:x:50,label=psg"));
  ASSERT_EQ(shapes.getConvexGrids().size(), 1u);
  EXPECT_EQ(shapes.getConvexGrids()[0].get_label(), "psg");
  EXPECT_NEAR(shapes.getConvexGrids()[0].getVal(
                  211, shapes.getConvexGrids()[0].getCellVarIX("x")),
              50.0, kGeomTol);

  ASSERT_TRUE(shapes.setParam(
      "view_marker", "x=400,y=-200,label=02,color=orange,type=circle,width=4"));
  ASSERT_EQ(shapes.getMarkers().size(), 1u);
  EXPECT_EQ(shapes.getMarkers().find("02")->second.get_color_str("primary_color"),
            "orange");

  ASSERT_TRUE(shapes.setParam(
      "xygrid", "pts={0,0:20,0:20,20:0,20},label=legacy_grid@10,10@7"));
  ASSERT_EQ(shapes.getGrids().size(), 1u);
  EXPECT_EQ(shapes.getGrids()[0].getLabel(), "legacy_grid");
  EXPECT_NEAR(shapes.getGrids()[0].getVal(0), 7.0, kGeomTol);
}

// Covers VPlug geo shapes lifecycle behavior: grid updates apply before aggregate mismatch failure.
TEST(VPlugGeoShapesLifecycleTest, GridUpdatesApplyBeforeAggregateMismatchFailure)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addGrid(kGridSpec));
  ASSERT_TRUE(shapes.addGrid(
      "pts={100,100:120,100:120,120:100,120},label=other_grid@10,10@3"));

  EXPECT_FALSE(shapes.updateGrid("search_grid@0,7,11"));

  ASSERT_EQ(shapes.getGrids().size(), 2u);
  EXPECT_NEAR(shapes.getGrids()[0].getVal(0), 11.0, kGeomTol);
  EXPECT_NEAR(shapes.getGrids()[1].getVal(0), 3.0, kGeomTol);
}
