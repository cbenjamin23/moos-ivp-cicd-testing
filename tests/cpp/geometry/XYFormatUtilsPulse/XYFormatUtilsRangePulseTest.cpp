#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsRangePulse.h"
#include "XYRangePulse.h"

TEST(XYFormatUtilsRangePulseTest, ParsesViewRangePulsePayloadFromRangeSensors)
{
  XYRangePulse pulse = stringStandard2RangePulse(
      "x=-40,y=-150,radius=40,duration=15,label=beacon_04,"
      "time=1252348077.5,fill=0.9,edge_color=yellow,fill_color=yellow,"
      "edge_size=2,vertex_size=0,label_color=gray50,active=false");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_x(), -40.0, kGeomTol);
  EXPECT_NEAR(pulse.get_y(), -150.0, kGeomTol);
  EXPECT_NEAR(pulse.get_radius(), 40.0, kGeomTol);
  EXPECT_NEAR(pulse.get_duration(), 15.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_EQ(pulse.get_label(), "beacon_04");
  EXPECT_TRUE(pulse.time_set());
  EXPECT_NEAR(pulse.get_time(), 1252348077.5, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(), 0.9, kGeomTol);
  EXPECT_EQ(pulse.get_color_str("edge"), "yellow");
  EXPECT_EQ(pulse.get_color_str("fill"), "yellow");
  EXPECT_EQ(pulse.get_color_str("label"), "gray50");
  EXPECT_TRUE(pulse.edge_size_set());
  EXPECT_NEAR(pulse.get_edge_size(), 2.0, kGeomTol);
  EXPECT_TRUE(pulse.vertex_size_set());
  EXPECT_NEAR(pulse.get_vertex_size(), 0.0, kGeomTol);
  EXPECT_FALSE(pulse.active());
}

TEST(XYFormatUtilsRangePulseTest, ParsesDocumentedBeaconRangeSensorViewRangePulsePayload)
{
  XYRangePulse pulse = string2RangePulse(
      "x=4,y=15,radius=40,duration=15,label=04,fill=0.25,"
      "fill_color=green,edge_size=1,edge_color=green,time=3892830128.5");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_x(), 4.0, kGeomTol);
  EXPECT_NEAR(pulse.get_y(), 15.0, kGeomTol);
  EXPECT_NEAR(pulse.get_radius(), 40.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), 15.0, kGeomTol);
  EXPECT_EQ(pulse.get_label(), "04");
  EXPECT_NEAR(pulse.get_fill(), 0.25, kGeomTol);
  EXPECT_EQ(pulse.get_color_str("fill"), "green");
  EXPECT_EQ(pulse.get_color_str("edge"), "green");
  EXPECT_TRUE(pulse.edge_size_set());
  EXPECT_NEAR(pulse.get_edge_size(), 1.0, kGeomTol);
  EXPECT_TRUE(pulse.time_set());
  EXPECT_NEAR(pulse.get_time(), 3892830128.5, 0.1);
}

TEST(XYFormatUtilsRangePulseTest, ParsesCollisionDetectorGeneratedAlertPulsePayload)
{
  XYRangePulse pulse = string2RangePulse(
      "x=78.78,y=-78.405,radius=40,duration=20,fill=0.25,"
      "edge_color=red,fill_color=red,time=1252348077.5");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_x(), 78.78, kGeomTol);
  EXPECT_NEAR(pulse.get_y(), -78.405, kGeomTol);
  EXPECT_NEAR(pulse.get_radius(), 40.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), 20.0, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(), 0.25, kGeomTol);
  EXPECT_EQ(pulse.get_color_str("edge"), "red");
  EXPECT_EQ(pulse.get_color_str("fill"), "red");
  EXPECT_TRUE(pulse.time_set());
}

TEST(XYFormatUtilsRangePulseTest, ParsesBhvRangePulseMinimalBehaviorPosting)
{
  // BHV_RangePulse posts this compact VIEW_RANGE_PULSE form from NAV_X/Y
  // without explicit fill, color, or timestamp fields.
  XYRangePulse pulse =
      string2RangePulse("x=125,y=-48,radius=25,duration=20,label=one");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_x(), 125.0, kGeomTol);
  EXPECT_NEAR(pulse.get_y(), -48.0, kGeomTol);
  EXPECT_NEAR(pulse.get_radius(), 25.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), 20.0, kGeomTol);
  EXPECT_EQ(pulse.get_label(), "one");
  EXPECT_FALSE(pulse.time_set());
  EXPECT_NEAR(pulse.get_fill(), 0.25, kGeomTol);
  EXPECT_EQ(pulse.get_color_str("edge"), "green");
  EXPECT_EQ(pulse.get_color_str("fill"), "green");
}

TEST(XYFormatUtilsRangePulseTest, RoundTripsGeneratedBeaconAndContactSensorPulse)
{
  // uFldBeaconRangeSensor and uFldContactRangeSensor generate XYRangePulse
  // objects directly, using fill=0.9 plus optional edge/fill colors.
  XYRangePulse generated(-110.81, -58.06);
  generated.set_label("AA");
  generated.set_rad(40);
  generated.set_fill(0.9);
  generated.set_duration(15);
  generated.set_time(123.45);
  generated.set_color("edge", "blue");
  generated.set_color("fill", "blue");

  XYRangePulse parsed = string2RangePulse(generated.get_spec());

  ASSERT_TRUE(parsed.valid());
  EXPECT_NEAR(parsed.get_x(), -110.81, kGeomTol);
  EXPECT_NEAR(parsed.get_y(), -58.06, kGeomTol);
  EXPECT_NEAR(parsed.get_radius(), 40.0, kGeomTol);
  EXPECT_NEAR(parsed.get_fill(), 0.9, kGeomTol);
  EXPECT_TRUE(parsed.duration_set());
  EXPECT_NEAR(parsed.get_duration(), 15.0, kGeomTol);
  EXPECT_TRUE(parsed.time_set());
  EXPECT_NEAR(parsed.get_time(), 123.45, kGeomTol);
  EXPECT_EQ(parsed.get_label(), "AA");
  EXPECT_EQ(parsed.get_color_str("edge"), "blue");
  EXPECT_EQ(parsed.get_color_str("fill"), "blue");
}

TEST(XYFormatUtilsRangePulseTest, RequiresOnlyPulseCenterCoordinates)
{
  EXPECT_FALSE(string2RangePulse("y=2,radius=30,duration=5").valid());
  EXPECT_FALSE(string2RangePulse("x=1,radius=30,duration=5").valid());
  EXPECT_FALSE(string2RangePulse("x=bad,y=2,radius=30,duration=5").valid());
  EXPECT_FALSE(string2RangePulse("x=1,y=bad,radius=30,duration=5").valid());

  XYRangePulse defaults = string2RangePulse("x=1,y=2,label=minimal");
  ASSERT_TRUE(defaults.valid());
  EXPECT_NEAR(defaults.get_radius(), 40.0, kGeomTol);
  EXPECT_NEAR(defaults.get_duration(), 15.0, kGeomTol);
  EXPECT_FALSE(defaults.duration_set());
  EXPECT_NEAR(defaults.get_fill(), 0.25, kGeomTol);
  EXPECT_EQ(defaults.get_color_str("edge"), "green");
  EXPECT_EQ(defaults.get_color_str("fill"), "green");
}

TEST(XYFormatUtilsRangePulseTest, PreservesLooseOptionalFieldBehavior)
{
  XYRangePulse pulse = string2RangePulse(
      "x=1,y=2,radius=-5,duration=-7,linger=-3,fill=2,"
      "fill_invariant=maybe,active=maybe,fill_transparency=2,"
      "edge_size=-1,vertex_size=-2,edge_color=notacolor,"
      "type=ping,source=uFldBeaconRangeSensor,vsource=alpha,msg=echo,id=pulse-1");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_radius(), 0.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), -1.0, kGeomTol);
  EXPECT_NEAR(pulse.get_linger(), 0.0, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(), 1.0, kGeomTol);
  EXPECT_TRUE(pulse.active());
  EXPECT_TRUE(pulse.transparency_set());
  EXPECT_NEAR(pulse.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(pulse.edge_size_set());
  EXPECT_NEAR(pulse.get_edge_size(), 1.0, kGeomTol);
  EXPECT_FALSE(pulse.vertex_size_set());
  EXPECT_EQ(pulse.get_color_str("edge"), "green");
  EXPECT_EQ(pulse.get_type(), "ping");
  EXPECT_EQ(pulse.get_source(), "uFldBeaconRangeSensor");
  EXPECT_EQ(pulse.get_vsource(), "alpha");
  EXPECT_EQ(pulse.get_msg(), "echo");
  EXPECT_EQ(pulse.get_id(), "pulse-1");
}

TEST(XYRangePulseTest, EvolvesCircleRadiusWithDurationAndLingerWindow)
{
  XYRangePulse pulse;
  EXPECT_FALSE(pulse.valid());
  pulse.set_x(10);
  pulse.set_y(-20);
  EXPECT_TRUE(pulse.valid());
  pulse.set_rad(100);
  pulse.set_duration(10);
  pulse.set_linger(3);
  pulse.set_time(50);

  EXPECT_TRUE(pulse.get_circle(49, 4).empty());

  std::vector<double> halfway = pulse.get_circle(55, 4);
  ASSERT_EQ(halfway.size(), 8u);
  for(unsigned int i = 0; i < halfway.size(); i += 2) {
    EXPECT_NEAR(std::hypot(halfway[i] - 10.0, halfway[i + 1] + 20.0), 50.0,
                kLooseGeomTol);
  }

  std::vector<double> lingering = pulse.get_circle(63, 4);
  ASSERT_EQ(lingering.size(), 8u);
  for(unsigned int i = 0; i < lingering.size(); i += 2) {
    EXPECT_NEAR(std::hypot(lingering[i] - 10.0, lingering[i + 1] + 20.0), 100.0,
                kLooseGeomTol);
  }

  EXPECT_TRUE(pulse.get_circle(63.01, 4).empty());
}

TEST(XYRangePulseTest, StartsAtCenterAndClampsLowCirclePointCount)
{
  XYRangePulse pulse(10, -20);
  pulse.set_rad(30);
  pulse.set_duration(6);
  pulse.set_time(100);

  std::vector<double> initial = pulse.get_circle(100, 1);
  ASSERT_EQ(initial.size(), 6u);
  for(unsigned int i = 0; i < initial.size(); i += 2) {
    EXPECT_NEAR(initial[i], 10.0, kGeomTol);
    EXPECT_NEAR(initial[i + 1], -20.0, kGeomTol);
  }

  std::vector<double> just_after_start = pulse.get_circle(101, 1);
  ASSERT_EQ(just_after_start.size(), 6u);
  for(unsigned int i = 0; i < just_after_start.size(); i += 2) {
    EXPECT_NEAR(std::hypot(just_after_start[i] - 10.0, just_after_start[i + 1] + 20.0),
                5.0, kLooseGeomTol);
  }
}

TEST(XYRangePulseTest, ClampsCirclePointCountAndRejectsDegenerateRanges)
{
  XYRangePulse pulse(0, 0);
  pulse.set_rad(30);
  pulse.set_duration(6);
  pulse.set_time(0);

  std::vector<double> triangle = pulse.get_circle(3, 2);
  EXPECT_EQ(triangle.size(), 6u);

  pulse.set_rad(-1);
  EXPECT_TRUE(pulse.get_circle(3, 4).empty());

  pulse.set_rad(30);
  pulse.set_duration(-1);
  EXPECT_TRUE(pulse.get_circle(3, 4).empty());
}

TEST(XYRangePulseTest, FadesFillUnlessMarkedInvariant)
{
  XYRangePulse pulse(0, 0);
  pulse.set_fill(0.8);
  pulse.set_duration(10);
  pulse.set_time(100);

  EXPECT_NEAR(pulse.get_fill(99), 0.8, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(100), 0.8, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(105), 0.4, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(110), 0.0, kGeomTol);

  pulse.set_fill_invariant(true);
  EXPECT_NEAR(pulse.get_fill(105), 0.8, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(110), 0.8, kGeomTol);
}

TEST(XYRangePulseTest, SerializesViewerCompatibleSpec)
{
  XYRangePulse pulse(1.234567, -2.345678);
  pulse.set_rad(55.55555);
  pulse.set_fill(0.9);
  pulse.set_linger(4);
  pulse.set_fill_invariant(true);
  pulse.set_label("range_alpha");
  pulse.set_time(12.345);
  pulse.set_duration(6.789);
  pulse.set_color("edge", "orange");
  pulse.set_color("fill", "orange");
  pulse.set_edge_size(3);

  std::string spec = pulse.get_spec("active=true");
  EXPECT_TRUE(stringContains(spec, "x=1.23457"));
  EXPECT_TRUE(stringContains(spec, "y=-2.34568"));
  EXPECT_TRUE(stringContains(spec, "radius=55.55555"));
  EXPECT_TRUE(stringContains(spec, "fill=0.9"));
  EXPECT_TRUE(stringContains(spec, "linger=4"));
  EXPECT_TRUE(stringContains(spec, "fill_invariant=true"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=range_alpha"));
  EXPECT_TRUE(stringContains(spec, "time=12.35"));
  EXPECT_TRUE(stringContains(spec, "duration=6.79"));
  EXPECT_TRUE(stringContains(spec, "edge_color=orange"));
  EXPECT_TRUE(stringContains(spec, "fill_color=orange"));
  EXPECT_TRUE(stringContains(spec, "edge_size=3"));
}
