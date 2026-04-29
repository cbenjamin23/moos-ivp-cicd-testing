#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "HintHolder.h"
#include "NumericAssertions.h"
#include "VPlug_DropPoints.h"
#include "VPlug_GeoSettings.h"
#include "VPlug_GeoShapesMap.h"
#include "VPlug_VehiSettings.h"
#include "XYCircle.h"
#include "XYConvexGrid.h"
#include "XYPoint.h"
#include "XYPolygon.h"
#include "XYSegList.h"
#include "XYVector.h"
#include "XYWedge.h"

namespace {

void ExpectColorPackNear(const ColorPack& color,
                         double expected_r,
                         double expected_g,
                         double expected_b)
{
  ASSERT_TRUE(color.set());
  EXPECT_NEAR(color.red(), expected_r, kLooseGeomTol);
  EXPECT_NEAR(color.grn(), expected_g, kLooseGeomTol);
  EXPECT_NEAR(color.blu(), expected_b, kLooseGeomTol);
}

}  // namespace

TEST(HintHolderTest, DefaultsReturnEmptyColorAndZeroMeasure)
{
  HintHolder hints;

  EXPECT_FALSE(hints.hasColor("edge_color"));
  EXPECT_FALSE(hints.hasMeasure("edge_size"));
  EXPECT_EQ(hints.getColor("edge_color"), "");
  EXPECT_DOUBLE_EQ(hints.getMeasure("edge_size"), 0.0);
  EXPECT_EQ(hints.getSpec(), "");
}

TEST(HintHolderTest, ParsesBehaviorStyleVisualHintsAndSerializesDeterministically)
{
  HintHolder hints;

  ASSERT_TRUE(hints.setHints(
      "edge_color=blue,vertex_color=white,lcolor=gray50,"
      "edge_size=2.5,vertex_size=4"));
  ASSERT_TRUE(hints.hasColor("label_color"));
  EXPECT_EQ(hints.getColor("label_color"), "gray50");
  EXPECT_EQ(hints.getColor("edge_color"), "blue");
  EXPECT_EQ(hints.getColor("vertex_color"), "white");
  EXPECT_DOUBLE_EQ(hints.getMeasure("edge_size"), 2.5);
  EXPECT_DOUBLE_EQ(hints.getMeasure("vertex_size"), 4.0);

  EXPECT_EQ(hints.getSpec(),
            "edge_color=blue,label_color=gray50,vertex_color=white,"
            "edge_size=2.5,vertex_size=4");
}

TEST(HintHolderTest, RejectsInvalidColorsAndDoesNotStoreInvalidMeasures)
{
  HintHolder hints;

  EXPECT_TRUE(hints.setColor("edge_color", "white"));
  EXPECT_TRUE(hints.setMeasure("edge_size", 2.5));
  EXPECT_EQ(hints.getColor("edge_color"), "white");
  EXPECT_DOUBLE_EQ(hints.getMeasure("edge_size"), 2.5);

  EXPECT_FALSE(hints.setHint("edge_color", "not-a-color"));
  EXPECT_EQ(hints.getColor("edge_color"), "white");
  // The string overload reports success for a numeric negative even though
  // the double overload rejects it and leaves the measure unset.
  EXPECT_TRUE(hints.setHint("edge_size", "-1"));
  EXPECT_FALSE(hints.setHint("vertex_size", "wide"));
  EXPECT_TRUE(hints.hasMeasure("edge_size"));
  EXPECT_DOUBLE_EQ(hints.getMeasure("edge_size"), 2.5);
  EXPECT_FALSE(hints.hasMeasure("vertex_size"));
}

TEST(HintHolderTest, SetHintsLeavesPriorEntriesWhenLaterEntryFails)
{
  HintHolder hints;

  // setHints is not transactional: earlier accepted pairs remain after a
  // later invalid pair makes the overall call return false.
  EXPECT_FALSE(hints.setHints("edge_color=green,vertex_size=bad"));
  EXPECT_TRUE(hints.hasColor("edge_color"));
  EXPECT_EQ(hints.getColor("edge_color"), "green");
  EXPECT_FALSE(hints.hasMeasure("vertex_size"));
}

TEST(HintHolderTest, AppliesGeneralAndPrefixSpecificHintsToGeometryObjects)
{
  HintHolder hints;
  ASSERT_TRUE(hints.setHints(
      "label_color=white,edge_color=blue,vertex_color=red,"
      "vertex_size=3,edge_size=2,route_edge_color=green,"
      "route_vertex_size=6,poly_fill_color=yellow"));

  XYPoint point(1, 2, "waypoint");
  applyHints(point, hints);
  EXPECT_EQ(point.get_color_str("label"), "white");
  EXPECT_EQ(point.get_color_str("vertex"), "red");
  EXPECT_DOUBLE_EQ(point.get_vertex_size(), 3.0);

  XYSegList route = makeTransitLane();
  applyHints(route, hints, "route");
  EXPECT_EQ(route.get_color_str("label"), "white");
  EXPECT_EQ(route.get_color_str("edge"), "green");
  EXPECT_EQ(route.get_color_str("vertex"), "red");
  EXPECT_DOUBLE_EQ(route.get_edge_size(), 2.0);
  EXPECT_DOUBLE_EQ(route.get_vertex_size(), 6.0);

  XYPolygon poly = makeSquarePoly(0, 0, 10, 10);
  applyHints(poly, hints, "poly");
  EXPECT_EQ(poly.get_color_str("fill"), "yellow");
  EXPECT_EQ(poly.get_color_str("edge"), "blue");
}

TEST(HintHolderTest, AcceptsUnknownNumericMeasuresAndColorNamedParameters)
{
  HintHolder hints;

  ASSERT_TRUE(hints.setHint("range_ring_radius", "25"));
  ASSERT_TRUE(hints.setHint("bogus_color_channel", "purple"));
  EXPECT_TRUE(hints.hasMeasure("range_ring_radius"));
  EXPECT_DOUBLE_EQ(hints.getMeasure("range_ring_radius"), 25.0);
  EXPECT_TRUE(hints.hasColor("bogus_color_channel"));
  EXPECT_EQ(hints.getColor("bogus_color_channel"), "purple");
}

TEST(VPlugGeoSettingsTest, ConstructorDefaultsMatchViewerGeoAttributeDefaults)
{
  VPlug_GeoSettings settings;

  EXPECT_TRUE(settings.viewable("polygon_viewable_all"));
  EXPECT_TRUE(settings.viewable("seglr_viewable_labels"));
  EXPECT_TRUE(settings.viewable("drop_point_viewable_all"));
  EXPECT_TRUE(settings.viewable("range_pulse_viewable_all"));
  EXPECT_TRUE(settings.viewable("tiff_viewable"));
  EXPECT_FALSE(settings.viewable("hash_viewable", true));
  EXPECT_FALSE(settings.viewable("datum_viewable", true));

  EXPECT_DOUBLE_EQ(settings.geosize("hash_delta"), 100.0);
  EXPECT_DOUBLE_EQ(settings.geosize("drop_point_vertex_size"), 2.0);
  EXPECT_DOUBLE_EQ(settings.geosize("vector_length_zoom"), 2.0);
  EXPECT_DOUBLE_EQ(settings.opaqueness("grid_opaqueness"), 0.3);
  EXPECT_EQ(settings.attribute("drop_point_coords"), "as-dropped");
  EXPECT_EQ(settings.attribute("polygon_label_pos"), "top");
  EXPECT_EQ(settings.colorname("drop_point_vertex_color"), "yellow");
  EXPECT_EQ(settings.colorname("datum_color"), "red");
}

TEST(VPlugGeoSettingsTest, AppliesMarineViewerGeoAttrPayloadsAndAliases)
{
  VPlug_GeoSettings settings;

  EXPECT_TRUE(settings.setParam("tiff_view", "off"));
  EXPECT_FALSE(settings.viewable("tiff_viewable", true));
  EXPECT_EQ(settings.strvalue("tiff_view"), "false");

  EXPECT_TRUE(settings.setParam("drop_point_coords", "lat-lon"));
  EXPECT_EQ(settings.attribute("drop_point_coords"), "lat-lon");
  EXPECT_EQ(settings.strvalue("drop_point_coords"), "lat-lon");

  EXPECT_TRUE(settings.setParam("polygon_label_pos", "MID"));
  // The validity check is case-insensitive, but the stored value is verbatim.
  EXPECT_EQ(settings.attribute("polygon_label_pos"), "MID");
  EXPECT_FALSE(settings.setParam("polygon_label_pos", "bottom"));
  EXPECT_EQ(settings.attribute("polygon_label_pos"), "MID");
}

TEST(VPlugGeoSettingsTest, TogglesViewabilityAndRejectsUnknownBooleanValues)
{
  VPlug_GeoSettings settings;

  EXPECT_TRUE(settings.setParam("polygon_viewable_all", "toggle"));
  EXPECT_FALSE(settings.viewable("POLYGON_VIEWABLE_ALL", true));
  EXPECT_TRUE(settings.setParam("polygon_viewable_all", "on"));
  EXPECT_TRUE(settings.viewable("polygon_viewable_all", false));
  EXPECT_FALSE(settings.setParam("polygon_viewable_all", "maybe"));
  EXPECT_TRUE(settings.viewable("polygon_viewable_all", false));
  EXPECT_FALSE(settings.setParam("bogus_viewable", "on"));
}

TEST(VPlugGeoSettingsTest, SizeMappingsAcceptDeltaScaleAndClipToViewerRanges)
{
  VPlug_GeoSettings settings;

  ASSERT_TRUE(settings.setParam("vector_length_zoom", "scale:3"));
  EXPECT_DOUBLE_EQ(settings.geosize("vector_length_zoom"), 4.0);
  ASSERT_TRUE(settings.setParam("vector_length_zoom", "delta:-10"));
  EXPECT_DOUBLE_EQ(settings.geosize("vector_length_zoom"), 0.0);

  ASSERT_TRUE(settings.setParam("hash_delta", "1"));
  EXPECT_DOUBLE_EQ(settings.geosize("hash_delta"), 10.0);
  ASSERT_TRUE(settings.setParam("hash_delta", "2000000"));
  EXPECT_DOUBLE_EQ(settings.geosize("hash_delta"), 1000000.0);

  ASSERT_TRUE(settings.setParam("marker_scale", "scale:0.05"));
  EXPECT_DOUBLE_EQ(settings.geosize("marker_scale"), 0.1);
  EXPECT_FALSE(settings.setParam("marker_scale", "scale:wide"));
  EXPECT_DOUBLE_EQ(settings.geosize("marker_scale"), 0.1);
}

TEST(VPlugGeoSettingsTest, OpaquenessSupportsMoreLessNumericAndClamps)
{
  VPlug_GeoSettings settings;

  EXPECT_TRUE(settings.setParam("grid_opaqueness", "more"));
  EXPECT_DOUBLE_EQ(settings.opaqueness("grid_opaqueness"), 0.4);
  EXPECT_TRUE(settings.setParam("grid_opaqueness", "2"));
  EXPECT_DOUBLE_EQ(settings.opaqueness("grid_opaqueness"), 1.0);
  EXPECT_TRUE(settings.setParam("grid_opaqueness", "less"));
  EXPECT_DOUBLE_EQ(settings.opaqueness("grid_opaqueness"), 0.9);
  EXPECT_FALSE(settings.setParam("grid_opaqueness", "clear"));
  EXPECT_DOUBLE_EQ(settings.opaqueness("grid_opaqueness"), 0.9);
}

TEST(VPlugGeoSettingsTest, ColorMappingsValidateAndExposeColorPacks)
{
  VPlug_GeoSettings settings;

  EXPECT_TRUE(settings.setParam("vector_label_color", "aqua"));
  EXPECT_EQ(settings.colorname("VECTOR_LABEL_COLOR"), "aqua");
  ExpectColorPackNear(settings.geocolor("vector_label_color"), 0.0, 1.0, 1.0);

  EXPECT_FALSE(settings.setParam("datum_color", "not-a-color"));
  EXPECT_EQ(settings.colorname("datum_color"), "red");
  ExpectColorPackNear(settings.geocolor("not_configured", "green"),
                      0.0, 128.0 / 255.0, 0.0);
}

TEST(VPlugGeoSettingsTest, StrvalueOnlyReportsSupportedSubset)
{
  VPlug_GeoSettings settings;

  EXPECT_EQ(settings.strvalue("datum_size"), "2");
  EXPECT_EQ(settings.strvalue("drop_point_vertex_size"), "2");
  EXPECT_EQ(settings.strvalue("grid_opaqueness"), "0.3");
  EXPECT_EQ(settings.strvalue("marker_edge_width"), "");
  EXPECT_EQ(settings.strvalue("vector_label_color"), "");
}

TEST(VPlugVehiSettingsTest, ConstructorDefaultsMatchMarineViewerVehicleDefaults)
{
  VPlug_VehiSettings settings;

  EXPECT_TRUE(settings.isViewableVehicles());
  EXPECT_TRUE(settings.isViewableTrails());
  EXPECT_TRUE(settings.isViewableBearingLines());
  EXPECT_FALSE(settings.isViewableTrailsConnect());
  EXPECT_FALSE(settings.isViewableTrailsFuture());
  EXPECT_EQ(settings.getTrailsLength(), 100u);
  EXPECT_DOUBLE_EQ(settings.getTrailsPointSize(), 1.0);
  EXPECT_DOUBLE_EQ(settings.getVehiclesShapeScale(), 1.0);
  EXPECT_DOUBLE_EQ(settings.getStaleReportThresh(), 60.0);
  EXPECT_DOUBLE_EQ(settings.getStaleRemoveThresh(), 180.0);
  EXPECT_EQ(settings.getVehiclesNameMode(), "names+shortmode");
  EXPECT_EQ(settings.strvalue("trails_color"), "white");
  EXPECT_EQ(settings.strvalue("vehicles_active_color"), "red");
  EXPECT_EQ(settings.strvalue("vehicles_inactive_color"), "yellow");
  EXPECT_EQ(settings.strvalue("vehicles_name_color"), "white");
}

TEST(VPlugVehiSettingsTest, ColorAliasesAcceptValidColorsAndRejectBadOnes)
{
  VPlug_VehiSettings settings;

  ExpectColorPackNear(settings.getColorTrails(), 1.0, 1.0, 1.0);
  ExpectColorPackNear(settings.getColorInactiveVehicle(), 1.0, 1.0, 0.0);
  ExpectColorPackNear(settings.getColorVehicleName(), 1.0, 1.0, 1.0);

  EXPECT_TRUE(settings.setParam("active_vehicle_color", "green"));
  EXPECT_EQ(settings.strvalue("vehicles_active_color"), "green");
  ExpectColorPackNear(settings.getColorActiveVehicle(), 0.0, 128.0 / 255.0, 0.0);

  EXPECT_TRUE(settings.setParam("vehicle_names_color", "gray50"));
  EXPECT_EQ(settings.strvalue("vehicles_name_color"), "gray50");

  EXPECT_FALSE(settings.setParam("trails_color", "not-a-color"));
  EXPECT_EQ(settings.strvalue("trails_color"), "white");
}

TEST(VPlugVehiSettingsTest, BooleanVehicleAndTrailSettingsUseOnOffYesNoToggle)
{
  VPlug_VehiSettings settings;

  EXPECT_TRUE(settings.setParam("vehicles_viewable", "off"));
  EXPECT_FALSE(settings.isViewableVehicles());
  EXPECT_TRUE(settings.setParam("vehicles_viewable", "toggle"));
  EXPECT_TRUE(settings.isViewableVehicles());
  EXPECT_TRUE(settings.setParam("trails_future_viewable", "yes"));
  EXPECT_TRUE(settings.isViewableTrailsFuture());
  EXPECT_FALSE(settings.setParam("bearing_lines_viewable", "maybe"));
  EXPECT_TRUE(settings.isViewableBearingLines());
}

TEST(VPlugVehiSettingsTest, VehicleNameModeSupportsDeprecatedNamesAndToggleCycle)
{
  VPlug_VehiSettings settings;

  EXPECT_TRUE(settings.setParam("vehicle_name_viewable", "false"));
  EXPECT_EQ(settings.getVehiclesNameMode(), "off");
  EXPECT_TRUE(settings.setParam("vehicles_name_viewable", "true"));
  EXPECT_EQ(settings.getVehiclesNameMode(), "names");
  EXPECT_TRUE(settings.setParam("vehicles_name_mode", "names+depth"));
  EXPECT_EQ(settings.getVehiclesNameMode(), "names+depth");

  EXPECT_TRUE(settings.setParam("vehicles_name_mode", "toggle"));
  EXPECT_EQ(settings.getVehiclesNameMode(), "names+auxmode");
  EXPECT_TRUE(settings.setParam("vehicles_name_mode", "toggle"));
  EXPECT_EQ(settings.getVehiclesNameMode(), "off");
  // The toggle cycle can produce names+auxmode, but direct assignment cannot.
  EXPECT_FALSE(settings.setParam("vehicles_name_mode", "names+auxmode"));
  EXPECT_EQ(settings.getVehiclesNameMode(), "off");
}

TEST(VPlugVehiSettingsTest, NumericSettingsClipAndCommandsScaleCurrentValues)
{
  VPlug_VehiSettings settings;

  EXPECT_TRUE(settings.setParam("trails_point_size", 20.0));
  EXPECT_DOUBLE_EQ(settings.getTrailsPointSize(), 10.0);
  EXPECT_TRUE(settings.setParam("trails_point_size", "smaller"));
  EXPECT_DOUBLE_EQ(settings.getTrailsPointSize(), 8.0);

  EXPECT_TRUE(settings.setParam("trails_length", 100000.0));
  EXPECT_EQ(settings.getTrailsLength(), 65000u);
  EXPECT_TRUE(settings.setParam("trails_length", "shorter"));
  EXPECT_EQ(settings.getTrailsLength(), 52000u);

  EXPECT_TRUE(settings.setParam("vehicle_shape_scale", "0.01"));
  EXPECT_DOUBLE_EQ(settings.getVehiclesShapeScale(), 0.1);
  EXPECT_TRUE(settings.setParam("vehicles_shape_scale", "bigger"));
  EXPECT_DOUBLE_EQ(settings.getVehiclesShapeScale(), 0.125);
  EXPECT_TRUE(settings.setParam("vehicles_shape_scale", "reset"));
  EXPECT_DOUBLE_EQ(settings.getVehiclesShapeScale(), 1.0);

  EXPECT_TRUE(settings.setParam("stale_report_thresh", "1"));
  EXPECT_DOUBLE_EQ(settings.getStaleReportThresh(), 2.0);
  EXPECT_TRUE(settings.setParam("stale_remove_thresh", "0"));
  EXPECT_DOUBLE_EQ(settings.getStaleRemoveThresh(), 0.0);
}

TEST(VPlugVehiSettingsTest, RejectsNegativeNumbersUnknownAttributesAndBadCommands)
{
  VPlug_VehiSettings settings;

  EXPECT_FALSE(settings.setParam("trails_point_size", -1.0));
  EXPECT_DOUBLE_EQ(settings.getTrailsPointSize(), 1.0);
  EXPECT_FALSE(settings.setParam("stale_report_thresh", "-5"));
  EXPECT_DOUBLE_EQ(settings.getStaleReportThresh(), 60.0);
  EXPECT_FALSE(settings.setParam("vehicles_shape_scale", "huge"));
  EXPECT_DOUBLE_EQ(settings.getVehiclesShapeScale(), 1.0);
  EXPECT_FALSE(settings.setParam("unknown_vehicle_setting", "true"));
  EXPECT_EQ(settings.strvalue("unknown_vehicle_setting"), "");
}

TEST(VPlugDropPointsTest, DefaultsAndSimpleAddPointUseAsDroppedCoordinates)
{
  VPlug_DropPoints drops;

  EXPECT_TRUE(drops.viewable());
  EXPECT_EQ(drops.size(), 0u);
  EXPECT_EQ(drops.getCoordinates(0), "index-out-of-range");

  XYPoint point(1, 2, "manual");
  drops.addPoint(point);
  ASSERT_EQ(drops.size(), 1u);
  EXPECT_EQ(drops.getPoint(0).get_label(), "manual");
  EXPECT_EQ(drops.getCoordinates(0), "");
}

TEST(VPlugDropPointsTest, MetadataAddColorsLabelsByNativeCoordinateKind)
{
  VPlug_DropPoints drops;

  XYPoint local_point(10, -3, "initial-local");
  XYPoint latlon_point(20, -4, "initial-latlon");
  drops.addPoint(local_point, "42.1,-70.2", "10,-3", "10,-3");
  drops.addPoint(latlon_point, "42.2,-70.3", "42.2,-70.3", "42.2,-70.3");

  EXPECT_EQ(drops.getPoint(0).get_color_str("label"), "yellow");
  EXPECT_EQ(drops.getPoint(1).get_color_str("label"), "lightgreen");
}

TEST(VPlugDropPointsTest, CoordinateModeRelabelsPointsButGetCoordinatesStaysNative)
{
  VPlug_DropPoints drops;
  drops.addPoint(XYPoint(10, -3, "initial"), "42.1,-70.2", "10,-3", "native");

  ASSERT_TRUE(drops.setParam("drop_point_coords", "lat-lon"));
  EXPECT_EQ(drops.getPoint(0).get_label(), "42.1,-70.2");
  // Current implementation never updates m_coord_mode, so this getter stays
  // on the as-dropped/native string even after relabeling the point.
  EXPECT_EQ(drops.getCoordinates(0), "native");

  ASSERT_TRUE(drops.setParam("drop_point_coords", "local-grid"));
  EXPECT_EQ(drops.getPoint(0).get_label(), "10,-3");
  EXPECT_EQ(drops.getCoordinates(0), "native");

  ASSERT_TRUE(drops.setParam("drop_point_coords", "as-dropped"));
  EXPECT_EQ(drops.getPoint(0).get_label(), "native");
  EXPECT_FALSE(drops.setParam("drop_point_coords", "utm"));
}

TEST(VPlugDropPointsTest, EditCommandsClearLastAndClearAll)
{
  VPlug_DropPoints drops;

  EXPECT_FALSE(drops.setParam("drop_point_edit", "clear_last"));
  drops.addPoint(XYPoint(1, 1, "one"));
  drops.addPoint(XYPoint(2, 2, "two"));
  drops.deletePoint(XYPoint(1, 1, "one"));
  EXPECT_EQ(drops.size(), 2u);

  EXPECT_TRUE(drops.setParam("drop_point_edit", "clear_last"));
  ASSERT_EQ(drops.size(), 1u);
  EXPECT_EQ(drops.getPoint(0).get_label(), "one");

  EXPECT_TRUE(drops.setParam("drop_point_edit", "clear"));
  EXPECT_EQ(drops.size(), 0u);
  EXPECT_FALSE(drops.setParam("drop_point_edit", "undo"));
}

TEST(VPlugDropPointsTest, VertexSizeClipsAndColorSettingsCurrentlyUseParamName)
{
  VPlug_DropPoints drops;
  drops.addPoint(XYPoint(1, 1, "one"));

  EXPECT_TRUE(drops.setParam("drop_point_vertex_size", "30"));
  EXPECT_DOUBLE_EQ(drops.getPoint(0).get_vertex_size(), 20.0);
  EXPECT_TRUE(drops.setParam("drop_point_vertex_size", "0"));
  EXPECT_DOUBLE_EQ(drops.getPoint(0).get_vertex_size(), 1.0);
  EXPECT_FALSE(drops.setParam("drop_point_vertex_size", "large"));

  // These branches pass the parameter name, not the supplied value, into
  // set_color. That invalid color string is retained as black by XYObject.
  EXPECT_TRUE(drops.setParam("drop_point_vertex_color", "green"));
  EXPECT_EQ(drops.getPoint(0).get_color_str("vertex"), "black");
  EXPECT_TRUE(drops.setParam("drop_point_label_color", "blue"));
  EXPECT_EQ(drops.getPoint(0).get_color_str("label"), "black");
}

TEST(VPlugDropPointsTest, ViewableMenuSettingIsCurrentlyUnsupported)
{
  VPlug_DropPoints drops;

  EXPECT_FALSE(drops.setParam("drop_point_viewable_all", "false"));
  EXPECT_TRUE(drops.viewable());
}

TEST(VPlugGeoShapesMapTest, AddsViewerPointPayloadsPerCommunityButPointSizeCountsPolygons)
{
  VPlug_GeoShapesMap shapes;

  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_POINT",
      "x=12,y=-3,label=station,vertex_color=green,vertex_size=4,"
      "source=pHelmIvP,type=viewer",
      "abe", 100.0));

  EXPECT_EQ(shapes.sizeVehicles(), 1u);
  ASSERT_EQ(shapes.getVehiNames().size(), 1u);
  EXPECT_EQ(shapes.getVehiNames()[0], "abe");
  EXPECT_EQ(shapes.getPoints("abe").size(), 1u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 1u);
  // Current VPlug_GeoShapesMap::size("points") delegates to sizePolygons().
  EXPECT_EQ(shapes.sizePoints(), 0u);
  EXPECT_EQ(shapes.size("points", "abe"), 0u);
}

TEST(VPlugGeoShapesMapTest, AggregatesPolygonWedgeAndHexagonAccessorsByVehicle)
{
  VPlug_GeoShapesMap shapes;

  ASSERT_TRUE(shapes.addGeoShape("VIEW_POLYGON",
                                 "pts={0,0:10,0:10,10:0,10},label=box",
                                 "abe", 1.0));
  ASSERT_TRUE(shapes.addGeoShape("VIEW_WEDGE",
                                 "x=0,y=0,rad_low=5,rad_hgh=20,ang_low=0,"
                                 "ang_hgh=90,label=sector",
                                 "abe", 1.0));

  EXPECT_EQ(shapes.sizePolygons(), 1u);
  EXPECT_EQ(shapes.sizeWedges(), 1u);
  EXPECT_EQ(shapes.sizeHexagons(), 0u);
  EXPECT_EQ(shapes.getPolygons("abe").size(), 1u);
  EXPECT_EQ(shapes.getWedges("abe").size(), 1u);
  EXPECT_EQ(shapes.getHexagons("abe").size(), 0u);
}

TEST(VPlugGeoShapesMapTest, UnsupportedOrInvalidViewerPayloadsMayStillCreateVehicleSlot)
{
  VPlug_GeoShapesMap shapes;

  EXPECT_FALSE(shapes.addGeoShape("VIEW_CIRCLE", "x=0,y=0,rad=-5,label=bad",
                                  "bad_src", 10.0));
  EXPECT_EQ(shapes.sizeVehicles(), 1u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 0u);
  EXPECT_EQ(shapes.getVehiNames().size(), 1u);
  EXPECT_EQ(shapes.getVehiNames()[0], "bad_src");

  EXPECT_FALSE(shapes.addGeoShape("VIEW_NOT_A_SHAPE", "x=1,y=2", "abe", 10.0));
  // Unrecognized params do not touch the map at all.
  EXPECT_EQ(shapes.sizeVehicles(), 1u);
  EXPECT_EQ(shapes.getVehiNames().size(), 1u);
}

TEST(VPlugGeoShapesMapTest, ReplacesAndDeletesMapBackedShapesByLabelAndActiveFlag)
{
  VPlug_GeoShapesMap shapes;

  EXPECT_TRUE(shapes.addGeoShape("VIEW_MARKER",
                                 "x=5,y=6,label=mark,type=circle,width=4",
                                 "abe", 20.0));
  ASSERT_EQ(shapes.getMarkers("abe").size(), 1u);
  EXPECT_DOUBLE_EQ(shapes.getMarkers("abe").find("mark")->second.get_vx(), 5.0);

  EXPECT_TRUE(shapes.addGeoShape("marker",
                                 "x=8,y=9,label=mark,type=diamond,width=2",
                                 "abe", 21.0));
  ASSERT_EQ(shapes.getMarkers("abe").size(), 1u);
  EXPECT_DOUBLE_EQ(shapes.getMarkers("abe").find("mark")->second.get_vx(), 8.0);

  EXPECT_TRUE(shapes.addGeoShape("VIEW_MARKER",
                                 "x=8,y=9,label=mark,type=diamond,active=false",
                                 "abe", 22.0));
  EXPECT_TRUE(shapes.getMarkers("abe").empty());
}

TEST(VPlugGeoShapesMapTest, ManageMemoryExpiresDurationLimitedViewerObjects)
{
  VPlug_GeoShapesMap shapes;

  EXPECT_TRUE(shapes.addGeoShape("VIEW_TEXTBOX",
                                 "x=1,y=2,label=note,msg=hello,duration=5",
                                 "shoreside", 100.0));
  ASSERT_EQ(shapes.getTextBoxes("shoreside").size(), 1u);

  shapes.manageMemory(104.9);
  EXPECT_EQ(shapes.getTextBoxes("shoreside").size(), 1u);
  shapes.manageMemory(105.1);
  EXPECT_TRUE(shapes.getTextBoxes("shoreside").empty());
}

TEST(VPlugGeoShapesMapTest, RoutesMarineViewerRegisteredVariablesFromMoosPostings)
{
  VPlug_GeoShapesMap shapes;

  testing::internal::CaptureStdout();
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_POLYGON",
      "pts={32,-100:38,-98:40,-100:32,-104},label=obstacle,type=obstacle",
      "shoreside", 10.0));
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(stringContains(out, "adding polygon\n"));
  EXPECT_TRUE(stringContains(out, "handled:true\n"));

  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_SEGLIST",
      "pts={0,0:10,0:10,10},label=bhv_path,edge_color=yellow",
      "abe", 11.0));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_POINT",
      "x=80,y=-35,label=abe_station,source=abe_rubber_band,active=true",
      "abe", 11.5));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_CIRCLE",
      "x=-150.3,y=-117.5,radius=80,label=alert_abe,edge_color=white,"
      "vertex_size=0,edge_size=1,duration=3",
      "shoreside", 11.75));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_WEDGE",
      "x=0,y=0,rad_low=40,rad_hgh=80,ang_low=350,ang_hgh=20,"
      "label=contact_bearing_sector,edge_color=yellow",
      "shoreside", 11.9));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_RANGE_PULSE",
      "x=4,y=15,radius=40,duration=15,label=04,fill=0.25",
      "shoreside", 12.0));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_COMMS_PULSE",
      "label=alpha2bravo,sx=4,sy=2,tx=44,ty=55,beam_width=10,"
      "duration=5,fill=0.3,fill_color=yellow,edge_color=green",
      "shoreside", 12.5));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_VECTOR",
      "x=45,y=-95,mag=2.4,ang=130,label=bravo_current,edge_color=white",
      "shoreside", 12.75));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_MARKER",
      "x=400,y=-200,label=02,color=orange,type=circle,width=4",
      "shoreside", 13.0));
  EXPECT_TRUE(shapes.addGeoShape(
      "GRID_CONFIG",
      "pts={0,0:20,0:20,20:0,20},label=search_grid@10,10@7",
      "shoreside", 14.0));
  EXPECT_TRUE(shapes.addGeoShape("GRID_DELTA", "search_grid@0,2,11",
                                 "shoreside", 15.0));
  EXPECT_TRUE(shapes.addGeoShape(
      "VIEW_GRID",
      "pts={-50,-40:-10,0:90,0:90,-90:50,-150:-50,-90},"
      "cell_size=5,cell_vars=x:0:y:0:z:0,cell_min=x:0,"
      "cell_max=x:50,label=psg",
      "shoreside", 15.5));
  EXPECT_TRUE(shapes.addGeoShape("VIEW_GRID_DELTA", "psg@211,x,3:211,y,7",
                                 "shoreside", 15.75));

  EXPECT_EQ(shapes.sizeVehicles(), 2u);
  EXPECT_EQ(shapes.getVehiNames(), (std::vector<std::string>{"abe", "shoreside"}));
  ASSERT_EQ(shapes.getPolygons("shoreside").size(), 1u);
  EXPECT_EQ(shapes.getPolygons("shoreside")[0].get_label(), "obstacle");
  ASSERT_EQ(shapes.getSegLists("abe").size(), 1u);
  EXPECT_EQ(shapes.getSegLists("abe").find("bhv_path")->second.get_label(),
            "bhv_path");
  ASSERT_EQ(shapes.getPoints("abe").size(), 1u);
  EXPECT_EQ(shapes.getPoints("abe").find("abe_station")->second.get_source(),
            "abe_rubber_band");
  ASSERT_EQ(shapes.getCircles("shoreside").size(), 1u);
  ASSERT_NE(shapes.getCircles("shoreside").find("alert_abe"),
            shapes.getCircles("shoreside").end());
  EXPECT_NEAR(shapes.getCircles("shoreside").find("alert_abe")
                  ->second.getRad(),
              80.0, kGeomTol);
  ASSERT_EQ(shapes.getWedges("shoreside").size(), 1u);
  EXPECT_EQ(shapes.getWedges("shoreside")[0].get_label(),
            "contact_bearing_sector");
  EXPECT_NEAR(shapes.getWedges("shoreside")[0].getRadHigh(), 80.0,
              kGeomTol);

  ASSERT_EQ(shapes.getRangePulses("shoreside").size(), 1u);
  EXPECT_NEAR(shapes.getRangePulses("shoreside")[0].get_radius(), 40.0,
              kGeomTol);
  EXPECT_NEAR(shapes.getRangePulses("shoreside")[0].get_time(), 12.0,
              kGeomTol);
  ASSERT_EQ(shapes.getCommsPulses("shoreside").size(), 1u);
  EXPECT_EQ(shapes.getCommsPulses("shoreside")[0].get_label(), "alpha2bravo");
  EXPECT_NEAR(shapes.getCommsPulses("shoreside")[0].get_beam_width(), 10.0,
              kGeomTol);
  EXPECT_NEAR(shapes.getCommsPulses("shoreside")[0].get_time(), 12.5,
              kGeomTol);
  ASSERT_EQ(shapes.getVectors("shoreside").size(), 1u);
  EXPECT_EQ(shapes.getVectors("shoreside")[0].get_label(), "bravo_current");
  EXPECT_NEAR(shapes.getVectors("shoreside")[0].mag(), 2.4, kGeomTol);
  EXPECT_NEAR(shapes.getVectors("shoreside")[0].ang(), 130.0, kGeomTol);
  EXPECT_EQ(shapes.getMarkers("shoreside").find("02")
                ->second.get_color_str("primary_color"),
            "orange");
  ASSERT_EQ(shapes.getGrids("shoreside").size(), 1u);
  EXPECT_NEAR(shapes.getGrids("shoreside")[0].getVal(0), 11.0, kGeomTol);
  std::vector<XYConvexGrid> cgrids = shapes.getConvexGrids("shoreside");
  ASSERT_EQ(cgrids.size(), 1u);
  EXPECT_EQ(cgrids[0].get_label(), "psg");
  EXPECT_NEAR(cgrids[0].getVal(211, cgrids[0].getCellVarIX("x")), 3.0,
              kGeomTol);
  EXPECT_NEAR(cgrids[0].getVal(211, cgrids[0].getCellVarIX("y")), 7.0,
              kGeomTol);
}

TEST(VPlugGeoShapesMapTest, ClearSpecificVehicleLeavesVehicleSlotButRemovesShapes)
{
  VPlug_GeoShapesMap shapes;

  EXPECT_TRUE(shapes.addGeoShape("VIEW_MARKER", "x=1,y=2,label=a,type=circle",
                                 "abe", 1.0));
  EXPECT_TRUE(shapes.addGeoShape("VIEW_MARKER", "x=3,y=4,label=b,type=circle",
                                 "ben", 1.0));
  ASSERT_EQ(shapes.sizeVehicles(), 2u);

  shapes.clear("abe");
  EXPECT_EQ(shapes.sizeVehicles(), 2u);
  EXPECT_TRUE(shapes.getMarkers("abe").empty());
  EXPECT_EQ(shapes.getMarkers("ben").size(), 1u);

  shapes.clear("all");
  EXPECT_EQ(shapes.sizeVehicles(), 0u);
  EXPECT_EQ(shapes.sizeTotalShapes(), 0u);
  EXPECT_TRUE(shapes.getVehiNames().empty());
}
