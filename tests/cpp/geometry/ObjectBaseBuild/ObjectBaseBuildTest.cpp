#include <gtest/gtest.h>

#include <initializer_list>
#include <string>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYBuildUtils.h"
#include "XYObject.h"

namespace {

void ExpectSpecContainsAll(const std::string& spec,
                           std::initializer_list<const char*> tokens)
{
  for(const char* token : tokens)
    EXPECT_TRUE(stringContains(spec, token)) << "missing token: " << token;
}

}  // namespace

// Covers XY object behavior: defaults are active valid and unset for viewer metadata.
TEST(XYObjectTest, DefaultsAreActiveValidAndUnsetForViewerMetadata)
{
  XYObject object;

  EXPECT_TRUE(object.valid());
  EXPECT_TRUE(object.active());
  EXPECT_FALSE(object.time_set());
  EXPECT_FALSE(object.duration_set());
  EXPECT_FALSE(object.transparency_set());
  EXPECT_FALSE(object.vertex_size_set());
  EXPECT_FALSE(object.edge_size_set());
  EXPECT_FALSE(object.color_set("label"));
  EXPECT_EQ(object.get_spec(), "");
}

// Covers XY object behavior: serializes marine viewer metadata and render hints in stable order.
TEST(XYObjectTest, SerializesMarineViewerMetadataAndRenderHintsInStableOrder)
{
  XYObject object;
  object.set_active(false);
  object.set_label("contact_a");
  object.set_msg("track");
  object.set_source("pMarineViewer");
  object.set_vsource("abe");
  object.set_type("waypoint");
  object.set_id("obj-7");
  object.set_color("label", "white");
  object.set_color("edge", "yellow");
  object.set_color("vertex", "green");
  object.set_color("fill", "invisible");
  object.set_time(12.25);
  object.set_duration(4);
  object.set_vertex_size(3);
  object.set_edge_size(2);
  object.set_transparency(0.25);

  EXPECT_EQ(object.get_type(), "waypoint");
  // XYObject stores type metadata, but the base-object spec omits it.
  EXPECT_EQ(object.get_spec(),
            "active=false,label=contact_a,msg=track,source=pMarineViewer,"
            "vsource=abe,id=obj-7,label_color=white,edge_color=yellow,"
            "vertex_color=green,fill_color=invisible,time=12.25,"
            "duration=4,vertex_size=3,edge_size=2,fill_transparency=0.25");
}

// Covers XY object behavior: active override parameter controls explicit lifecycle serialization.
TEST(XYObjectTest, ActiveOverrideParameterControlsExplicitLifecycleSerialization)
{
  XYObject object;

  EXPECT_EQ(object.get_spec(), "");
  EXPECT_EQ(object.get_spec("active=true"), "active=true");
  EXPECT_EQ(object.get_spec("ACTIVE=FALSE"), "active=false");

  object.set_active(false);
  EXPECT_EQ(object.get_spec(), "active=false");
  EXPECT_EQ(object.get_spec("active=true"), "active=true");
}

// Covers XY object behavior: set param accepts viewer metadata aliases and render hints.
TEST(XYObjectTest, SetParamAcceptsViewerMetadataAliasesAndRenderHints)
{
  XYObject object;

  EXPECT_TRUE(object.set_param("label", "op_region"));
  EXPECT_TRUE(object.set_param("source", "helm"));
  EXPECT_TRUE(object.set_param("vsource", "abe"));
  EXPECT_TRUE(object.set_param("type", "polygon"));
  EXPECT_TRUE(object.set_param("msg", "published"));
  EXPECT_TRUE(object.set_param("id", "poly-1"));
  EXPECT_TRUE(object.set_param("time", "99.5"));
  EXPECT_TRUE(object.set_param("dur", "7"));
  EXPECT_TRUE(object.set_param("active", "false"));
  EXPECT_TRUE(object.set_param("label_color", "invisible"));
  EXPECT_TRUE(object.set_param("edge_color", "white"));
  EXPECT_TRUE(object.set_param("vertex_color", "green"));
  EXPECT_TRUE(object.set_param("fill_color", "blue"));
  EXPECT_TRUE(object.set_param("fill_transparency", "0.4"));
  EXPECT_TRUE(object.set_param("edge_size", "2.5"));
  EXPECT_TRUE(object.set_param("vertex_size", "4"));

  EXPECT_EQ(object.get_label(), "op_region");
  EXPECT_EQ(object.get_source(), "helm");
  EXPECT_EQ(object.get_vsource(), "abe");
  EXPECT_EQ(object.get_type(), "polygon");
  EXPECT_EQ(object.get_msg(), "published");
  EXPECT_EQ(object.get_id(), "poly-1");
  EXPECT_FALSE(object.active());
  EXPECT_NEAR(object.get_time(), 99.5, kGeomTol);
  EXPECT_NEAR(object.get_duration(), 7.0, kGeomTol);
  EXPECT_EQ(object.get_color_str("label"), "invisible");
  EXPECT_EQ(object.get_color_str("edge"), "white");
  EXPECT_EQ(object.get_color_str("vertex"), "green");
  EXPECT_EQ(object.get_color_str("fill"), "blue");
  EXPECT_NEAR(object.get_transparency(), 0.4, kGeomTol);
  EXPECT_NEAR(object.get_edge_size(), 2.5, kGeomTol);
  EXPECT_NEAR(object.get_vertex_size(), 4.0, kGeomTol);
}

// Covers XY object behavior: set param rejects unknown active but reports invalid color as handled.
TEST(XYObjectTest, SetParamRejectsUnknownActiveButReportsInvalidColorAsHandled)
{
  XYObject object;
  object.set_active(true);
  object.set_color("edge", "red");

  EXPECT_FALSE(object.set_param("active", "maybe"));
  EXPECT_TRUE(object.active());
  EXPECT_FALSE(object.set_param("bogus", "value"));
  // Color parameters are treated as handled even when ColorPack rejects the
  // value and leaves the prior color intact.
  EXPECT_TRUE(object.set_param("edge_color", "not-a-color"));
  EXPECT_EQ(object.get_color_str("edge"), "red");
}

// Covers XY object behavior: numeric hint setters clamp or ignore current boundary cases.
TEST(XYObjectTest, NumericHintSettersClampOrIgnoreCurrentBoundaryCases)
{
  XYObject object;

  object.set_vertex_size(-1);
  object.set_edge_size(-1);
  EXPECT_FALSE(object.vertex_size_set());
  EXPECT_FALSE(object.edge_size_set());

  object.set_vertex_size(0);
  object.set_edge_size(0);
  EXPECT_TRUE(object.vertex_size_set());
  EXPECT_TRUE(object.edge_size_set());
  EXPECT_NEAR(object.get_vertex_size(), 0.0, kGeomTol);
  EXPECT_NEAR(object.get_edge_size(), 0.0, kGeomTol);

  object.set_transparency(-2);
  EXPECT_TRUE(object.transparency_set());
  EXPECT_NEAR(object.get_transparency(), 0.0, kGeomTol);
  object.set_transparency(2);
  EXPECT_NEAR(object.get_transparency(), 1.0, kGeomTol);

  object.set_duration(-5);
  EXPECT_TRUE(object.duration_set());
  EXPECT_NEAR(object.get_duration(), -1.0, kGeomTol);
}

// Covers XY object behavior: label visibility can be represented with invisible color tokens.
TEST(XYObjectTest, LabelVisibilityCanBeRepresentedWithInvisibleColorTokens)
{
  XYObject object;

  object.set_color("label", "invisible");
  object.set_color("edge", "off");
  object.set_color("vertex", "empty");

  EXPECT_TRUE(object.color_set("label"));
  EXPECT_TRUE(object.color_set("edge"));
  EXPECT_TRUE(object.color_set("vertex"));
  EXPECT_EQ(object.get_color_str("label"), "invisible");
  EXPECT_EQ(object.get_color_str("edge"), "invisible");
  EXPECT_EQ(object.get_color_str("vertex"), "invisible");
}

// Covers XY object behavior: legacy vertex color helpers and color pack getter remain usable.
TEST(XYObjectTest, LegacyVertexColorHelpersAndColorPackGetterRemainUsable)
{
  XYObject object;

  object.set_vertex_color("yellow");
  EXPECT_TRUE(object.color_set("vertex"));
  EXPECT_EQ(object.get_color("vertex").str(), "yellow");

  object.set_vertex_color_size("green", 5);
  EXPECT_EQ(object.get_color_str("vertex"), "green");
  EXPECT_NEAR(object.get_vertex_size(), 5.0, kGeomTol);

  object.set_edge_color("blue");
  object.set_label_color("white");
  EXPECT_EQ(object.get_color("edge").str(), "blue");
  EXPECT_EQ(object.get_color("label").str(), "white");
  EXPECT_EQ(object.get_color("missing").str(), "black");
}

// Covers XY object behavior: expiration requires time duration and positive start time.
TEST(XYObjectTest, ExpirationRequiresTimeDurationAndPositiveStartTime)
{
  XYObject object;

  object.set_duration(5);
  EXPECT_FALSE(object.expired(100));

  object.set_time(0);
  EXPECT_FALSE(object.expired(100));

  object.set_time(10);
  EXPECT_FALSE(object.expired(15));
  EXPECT_TRUE(object.expired(15.01));

  object.set_duration(-1);
  EXPECT_FALSE(object.expired(100));
}

// Covers XY object behavior: clear resets lifecycle and drawing state but leaves source type fields.
TEST(XYObjectTest, ClearResetsLifecycleAndDrawingStateButLeavesSourceTypeFields)
{
  XYObject object;
  object.set_label("label");
  object.set_msg("msg");
  object.set_id("id");
  object.set_source("source");
  object.set_vsource("vsource");
  object.set_type("type");
  object.set_active(false);
  object.set_time(10);
  object.set_duration(5);
  object.set_color("edge", "yellow");
  object.set_edge_size(2);
  object.set_transparency(0.5);

  object.clear();

  EXPECT_TRUE(object.active());
  EXPECT_FALSE(object.time_set());
  EXPECT_FALSE(object.duration_set());
  EXPECT_FALSE(object.edge_size_set());
  EXPECT_FALSE(object.transparency_set());
  EXPECT_FALSE(object.color_set("edge"));
  EXPECT_EQ(object.get_label(), "");
  EXPECT_EQ(object.get_msg(), "");
  EXPECT_EQ(object.get_id(), "");
  // Current XYObject::clear() intentionally only clears selected metadata.
  EXPECT_EQ(object.get_source(), "source");
  EXPECT_EQ(object.get_vsource(), "vsource");
  EXPECT_EQ(object.get_type(), "type");
}

// Covers XY build utils behavior: string to point wrapper preserves viewer metadata and hints.
TEST(XYBuildUtilsTest, StringToPointWrapperPreservesViewerMetadataAndHints)
{
  XYPoint point = stringToPoint(
      "x=12.5,y=-3,z=2,label=waypoint,type=survey_point,"
      "source=pMarineViewer,vsource=abe,msg=arrived,id=pt-1,"
      "vertex_size=4,edge_size=2,vertex_color=yellow,edge_color=gray50,"
      "label_color=invisible,fill_color=blue,time=10,duration=5,"
      "fill_transparency=0.4,active=false");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 12.5, kGeomTol);
  EXPECT_NEAR(point.y(), -3.0, kGeomTol);
  EXPECT_NEAR(point.z(), 2.0, kGeomTol);
  EXPECT_EQ(point.get_label(), "waypoint");
  EXPECT_EQ(point.get_type(), "survey_point");
  EXPECT_EQ(point.get_source(), "pMarineViewer");
  EXPECT_EQ(point.get_vsource(), "abe");
  EXPECT_EQ(point.get_msg(), "arrived");
  EXPECT_EQ(point.get_id(), "pt-1");
  EXPECT_FALSE(point.active());
  EXPECT_EQ(point.get_color_str("label"), "invisible");
  EXPECT_EQ(point.get_color_str("vertex"), "yellow");
  EXPECT_EQ(point.get_color_str("edge"), "gray50");
  EXPECT_EQ(point.get_color_str("fill"), "blue");
  EXPECT_NEAR(point.get_vertex_size(), 4.0, kGeomTol);
  EXPECT_NEAR(point.get_edge_size(), 2.0, kGeomTol);
  EXPECT_NEAR(point.get_time(), 10.0, kGeomTol);
  EXPECT_NEAR(point.get_duration(), 5.0, kGeomTol);
  EXPECT_NEAR(point.get_transparency(), 0.4, kGeomTol);
}

// Covers XY build utils behavior: string to point wrapper pins permissive numeric parsing and clamps.
TEST(XYBuildUtilsTest, StringToPointWrapperPinsPermissiveNumericParsingAndClamps)
{
  XYPoint point = stringToPoint(
      "x=not-a-number,y=4,z=also-bad,label=coerced,active=maybe,"
      "duration=-7,fill_transparency=2,vertex_size=-3,edge_size=-2,"
      "vertex_color=not-a-color");

  // Standard point parsing requires x/y fields but does not require numbers.
  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 0.0, kGeomTol);
  EXPECT_NEAR(point.y(), 4.0, kGeomTol);
  EXPECT_NEAR(point.z(), 0.0, kGeomTol);
  EXPECT_TRUE(point.active());
  EXPECT_TRUE(point.duration_set());
  EXPECT_NEAR(point.get_duration(), -1.0, kGeomTol);
  EXPECT_TRUE(point.transparency_set());
  EXPECT_NEAR(point.get_transparency(), 1.0, kGeomTol);
  EXPECT_FALSE(point.vertex_size_set());
  EXPECT_FALSE(point.edge_size_set());
  EXPECT_FALSE(point.color_set("vertex"));
}

// Covers XY build utils behavior: string to point wrapper supports abbreviated metadata form.
TEST(XYBuildUtilsTest, StringToPointWrapperSupportsAbbreviatedMetadataForm)
{
  XYPoint point = stringToPoint(
      "5,6,7:label,abbr_point:source,helm:vsource,viewer:msg,hello:"
      "id,p1:active,false:label_color,invisible:vertex_size,3");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 5.0, kGeomTol);
  EXPECT_NEAR(point.y(), 6.0, kGeomTol);
  EXPECT_NEAR(point.z(), 7.0, kGeomTol);
  EXPECT_EQ(point.get_label(), "abbr_point");
  EXPECT_EQ(point.get_source(), "helm");
  EXPECT_EQ(point.get_vsource(), "viewer");
  EXPECT_EQ(point.get_msg(), "hello");
  EXPECT_EQ(point.get_id(), "p1");
  EXPECT_FALSE(point.active());
  EXPECT_EQ(point.get_color_str("label"), "invisible");
  EXPECT_NEAR(point.get_vertex_size(), 3.0, kGeomTol);
}

// Covers XY build utils behavior: string to seg list wrapper parses route metadata and render hints.
TEST(XYBuildUtilsTest, StringToSegListWrapperParsesRouteMetadataAndRenderHints)
{
  XYSegList route = stringToSegList(
      "pts={0,0:50,0:100,25},label=survey_line,type=route,"
      "source=pHelmIvP,vsource=abe,msg=preview,id=route-1,"
      "edge_color=yellow,vertex_color=green,label_color=invisible,"
      "fill_color=blue,edge_size=2,vertex_size=3,fill_transparency=0.6,"
      "time=20,duration=4,active=false");

  ASSERT_TRUE(route.valid());
  ASSERT_EQ(route.size(), 3u);
  EXPECT_EQ(route.get_label(), "survey_line");
  EXPECT_EQ(route.get_type(), "route");
  EXPECT_EQ(route.get_source(), "pHelmIvP");
  EXPECT_EQ(route.get_vsource(), "abe");
  EXPECT_EQ(route.get_msg(), "preview");
  EXPECT_EQ(route.get_id(), "route-1");
  EXPECT_FALSE(route.active());
  EXPECT_EQ(route.get_color_str("edge"), "yellow");
  EXPECT_EQ(route.get_color_str("vertex"), "green");
  EXPECT_EQ(route.get_color_str("label"), "invisible");
  EXPECT_EQ(route.get_color_str("fill"), "blue");
  EXPECT_NEAR(route.get_edge_size(), 2.0, kGeomTol);
  EXPECT_NEAR(route.get_vertex_size(), 3.0, kGeomTol);
  EXPECT_NEAR(route.get_transparency(), 0.6, kGeomTol);
  EXPECT_NEAR(route.get_time(), 20.0, kGeomTol);
  EXPECT_NEAR(route.get_duration(), 4.0, kGeomTol);
}

// Covers XY build utils behavior: string to seg list wrapper accepts inactive label only deletion.
TEST(XYBuildUtilsTest, StringToSegListWrapperAcceptsInactiveLabelOnlyDeletion)
{
  XYSegList route = stringToSegList(
      "label=stale_route,active=false,duration=0,edge_color=not-a-color");

  ASSERT_TRUE(route.valid());
  EXPECT_FALSE(route.active());
  EXPECT_EQ(route.size(), 0u);
  EXPECT_EQ(route.get_label(), "stale_route");
  EXPECT_TRUE(route.duration_set());
  EXPECT_NEAR(route.get_duration(), 0.0, kGeomTol);
  EXPECT_FALSE(route.color_set("edge"));
  EXPECT_EQ(route.get_spec(), "active=false,label=stale_route,duration=0");
  EXPECT_EQ(route.get_spec_inactive(),
            "pts={0,0:9,0:0,9},active=false,label=stale_route,duration=0");
}

// Covers XY build utils behavior: string to poly wrapper parses area metadata and fill hints.
TEST(XYBuildUtilsTest, StringToPolyWrapperParsesAreaMetadataAndFillHints)
{
  XYPolygon poly = stringToPoly(
      "pts={0,0:20,0:20,10:0,10},label=safety_box,type=op_region,"
      "source=pObstacleMgr,vsource=abe,msg=monitor,id=poly-1,"
      "edge_color=white,vertex_color=green,label_color=invisible,"
      "fill_color=yellow,edge_size=2,vertex_size=3,"
      "fill_transparency=0.35,time=30,duration=8,active=true");

  ASSERT_TRUE(poly.valid());
  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 4u);
  EXPECT_NEAR(poly.area(), 200.0, kGeomTol);
  EXPECT_EQ(poly.get_label(), "safety_box");
  EXPECT_EQ(poly.get_type(), "op_region");
  EXPECT_EQ(poly.get_source(), "pObstacleMgr");
  EXPECT_EQ(poly.get_vsource(), "abe");
  EXPECT_EQ(poly.get_msg(), "monitor");
  EXPECT_EQ(poly.get_id(), "poly-1");
  EXPECT_TRUE(poly.active());
  EXPECT_EQ(poly.get_color_str("edge"), "white");
  EXPECT_EQ(poly.get_color_str("vertex"), "green");
  EXPECT_EQ(poly.get_color_str("label"), "invisible");
  EXPECT_EQ(poly.get_color_str("fill"), "yellow");
  EXPECT_NEAR(poly.get_edge_size(), 2.0, kGeomTol);
  EXPECT_NEAR(poly.get_vertex_size(), 3.0, kGeomTol);
  EXPECT_NEAR(poly.get_transparency(), 0.35, kGeomTol);
  EXPECT_NEAR(poly.get_time(), 30.0, kGeomTol);
  EXPECT_NEAR(poly.get_duration(), 8.0, kGeomTol);
}

// Covers XY build utils behavior: string to poly wrapper pins inactive label only deletion.
TEST(XYBuildUtilsTest, StringToPolyWrapperPinsInactiveLabelOnlyDeletion)
{
  XYPolygon poly = stringToPoly("label=stale_poly,active=false");

  EXPECT_TRUE(poly.valid());
  EXPECT_FALSE(poly.active());
  EXPECT_EQ(poly.size(), 0u);
  EXPECT_EQ(poly.get_label(), "stale_poly");
  EXPECT_EQ(poly.get_spec(), "active=false,label=stale_poly");
}

// Covers XY build utils behavior: string to poly wrapper rejects malformed pts before metadata can rescue.
TEST(XYBuildUtilsTest, StringToPolyWrapperRejectsMalformedPtsBeforeMetadataCanRescue)
{
  XYPolygon poly = stringToPoly(
      "pts={0,0:bad,5},label=bad_poly,active=false,edge_color=white");

  EXPECT_FALSE(poly.valid());
  EXPECT_TRUE(poly.active());
  EXPECT_EQ(poly.size(), 0u);
  EXPECT_EQ(poly.get_label(), "");
  EXPECT_FALSE(poly.color_set("edge"));
}

// Covers XY build utils behavior: wrapper round trips viewer style specs with base object fields.
TEST(XYBuildUtilsTest, WrapperRoundTripsViewerStyleSpecsWithBaseObjectFields)
{
  XYPoint point = stringToPoint("x=1,y=2,label=pt,source=unit,label_color=invisible");
  XYSegList route = stringToSegList(
      "pts={0,0:1,1},label=route,source=unit,edge_color=white");
  XYPolygon poly = stringToPoly(
      "pts={0,0:2,0:2,2:0,2},label=poly,source=unit,fill_color=yellow,"
      "fill_transparency=0.5");

  ASSERT_TRUE(point.valid());
  ASSERT_TRUE(route.valid());
  ASSERT_TRUE(poly.valid());

  ExpectSpecContainsAll(point.get_spec(),
                        {"x=1", "y=2", "label=pt", "source=unit",
                         "label_color=invisible"});
  ExpectSpecContainsAll(route.get_spec(),
                        {"pts={0,0:1,1}", "label=route", "source=unit",
                         "edge_color=white"});
  ExpectSpecContainsAll(poly.get_spec(),
                        {"pts={0,0:2,0:2,2:0,2}", "label=poly",
                         "source=unit", "fill_color=yellow",
                         "fill_transparency=0.5"});
}
