#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYCommsPulse.h"
#include "XYFormatUtilsCommsPulse.h"

// Covers XY format utils comms pulse behavior: parses view comms pulse payload from node comms.
TEST(XYFormatUtilsCommsPulseTest, ParsesViewCommsPulsePayloadFromNodeComms)
{
  XYCommsPulse pulse = stringStandard2CommsPulse(
      "label=alpha2bravo,sx=4,sy=2,tx=44,ty=55,beam_width=10,"
      "duration=5,fill=0.3,fill_color=yellow,edge_color=green,"
      "time=1252348077.5,ptype=msg,type=comms,source=uFldNodeComms,"
      "vsource=alpha,msg=NODE_MESSAGE,id=cpulse-1,active=false,"
      "fill_transparency=0.4");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_sx(), 4.0, kGeomTol);
  EXPECT_NEAR(pulse.get_sy(), 2.0, kGeomTol);
  EXPECT_NEAR(pulse.get_beam_width(), 10.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), 5.0, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(), 0.3, kGeomTol);
  EXPECT_EQ(pulse.get_pulse_type(), "msg");
  EXPECT_EQ(pulse.get_label(), "alpha2bravo");
  EXPECT_EQ(pulse.get_type(), "comms");
  EXPECT_EQ(pulse.get_source(), "uFldNodeComms");
  EXPECT_EQ(pulse.get_vsource(), "alpha");
  EXPECT_EQ(pulse.get_msg(), "NODE_MESSAGE");
  EXPECT_EQ(pulse.get_id(), "cpulse-1");
  EXPECT_TRUE(pulse.time_set());
  EXPECT_NEAR(pulse.get_time(), 1252348077.5, kGeomTol);
  EXPECT_EQ(pulse.get_color_str("fill"), "yellow");
  EXPECT_EQ(pulse.get_color_str("edge"), "green");
  EXPECT_TRUE(pulse.transparency_set());
  EXPECT_NEAR(pulse.get_transparency(), 0.4, kGeomTol);
  EXPECT_FALSE(pulse.active());
}

// Covers XY format utils comms pulse behavior: parses documented node comms pulse info payload.
TEST(XYFormatUtilsCommsPulseTest, ParsesDocumentedNodeCommsPulseInfoPayload)
{
  XYCommsPulse pulse = string2CommsPulse(
      "label=one,sx=4,sy=2,tx=44,ty=55,beam_width=10,"
      "duration=5,fill=0.3,fill_color=yellow,edge_color=green");

  ASSERT_TRUE(pulse.valid());
  EXPECT_EQ(pulse.get_label(), "one");
  EXPECT_NEAR(pulse.get_sx(), 4.0, kGeomTol);
  EXPECT_NEAR(pulse.get_sy(), 2.0, kGeomTol);
  EXPECT_NEAR(pulse.get_beam_width(), 10.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), 5.0, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(), 0.3, kGeomTol);
  EXPECT_EQ(pulse.get_color_str("fill"), "yellow");
  EXPECT_EQ(pulse.get_color_str("edge"), "green");
}

// Covers XY format utils comms pulse behavior: round trips node comms generated auto pulse.
TEST(XYFormatUtilsCommsPulseTest, RoundTripsNodeCommsGeneratedAutoPulse)
{
  // uFldNodeComms::postViewCommsPulse uses label=vname1+"2"+vname2
  // for auto-colored pulses, clamps beam width to at least 5, and maps
  // auto color to orange fill.
  XYCommsPulse generated(10, 20, 40, 60);
  generated.set_duration(3);
  generated.set_label("alpha2bravo");
  generated.set_time(200.25);
  generated.set_beam_width(5);
  generated.set_fill(0.6);
  generated.set_pulse_type("msg");
  generated.set_color("fill", "orange");

  XYCommsPulse parsed = string2CommsPulse(generated.get_spec());

  ASSERT_TRUE(parsed.valid());
  EXPECT_NEAR(parsed.get_sx(), 10.0, kGeomTol);
  EXPECT_NEAR(parsed.get_sy(), 20.0, kGeomTol);
  EXPECT_TRUE(stringContains(parsed.get_spec(), "tx=40"));
  EXPECT_TRUE(stringContains(parsed.get_spec(), "ty=60"));
  EXPECT_NEAR(parsed.get_beam_width(), 5.0, kGeomTol);
  EXPECT_TRUE(parsed.duration_set());
  EXPECT_NEAR(parsed.get_duration(), 3.0, kGeomTol);
  EXPECT_NEAR(parsed.get_fill(), 0.6, kGeomTol);
  EXPECT_EQ(parsed.get_pulse_type(), "msg");
  EXPECT_EQ(parsed.get_label(), "alpha2bravo");
  EXPECT_TRUE(parsed.time_set());
  EXPECT_NEAR(parsed.get_time(), 200.25, kGeomTol);
  EXPECT_EQ(parsed.get_color_str("fill"), "orange");
}

// Covers XY format utils comms pulse behavior: round trips node comms explicit color label suffix.
TEST(XYFormatUtilsCommsPulseTest, RoundTripsNodeCommsExplicitColorLabelSuffix)
{
  // Explicit pcolor values are appended to the uFldNodeComms label and used
  // directly as the fill color.
  XYCommsPulse generated(0, 0, 100, 0);
  generated.set_duration(4);
  generated.set_label("alpha2bravoyellow");
  generated.set_time(42);
  generated.set_beam_width(10);
  generated.set_fill(0.35);
  generated.set_pulse_type("nrep");
  generated.set_color("fill", "yellow");

  XYCommsPulse parsed = string2CommsPulse(generated.get_spec());

  ASSERT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.get_label(), "alpha2bravoyellow");
  EXPECT_EQ(parsed.get_pulse_type(), "nrep");
  EXPECT_EQ(parsed.get_color_str("fill"), "yellow");
  EXPECT_NEAR(parsed.get_beam_width(), 10.0, kGeomTol);
  EXPECT_TRUE(parsed.time_set());
  EXPECT_NEAR(parsed.get_time(), 42.0, kGeomTol);
}

// Covers XY format utils comms pulse behavior: requires all source and target coordinates.
TEST(XYFormatUtilsCommsPulseTest, RequiresAllSourceAndTargetCoordinates)
{
  EXPECT_FALSE(string2CommsPulse("sy=0,tx=10,ty=20").valid());
  EXPECT_FALSE(string2CommsPulse("sx=0,tx=10,ty=20").valid());
  EXPECT_FALSE(string2CommsPulse("sx=0,sy=0,ty=20").valid());
  EXPECT_FALSE(string2CommsPulse("sx=0,sy=0,tx=10").valid());
  EXPECT_FALSE(string2CommsPulse("sx=bad,sy=0,tx=10,ty=20").valid());
  EXPECT_FALSE(string2CommsPulse("sx=0,sy=0,tx=bad,ty=20").valid());

  XYCommsPulse defaults = string2CommsPulse("sx=0,sy=0,tx=10,ty=20");
  ASSERT_TRUE(defaults.valid());
  EXPECT_NEAR(defaults.get_beam_width(), 10.0, kGeomTol);
  EXPECT_NEAR(defaults.get_duration(), 3.0, kGeomTol);
  EXPECT_FALSE(defaults.duration_set());
  EXPECT_NEAR(defaults.get_fill(), 0.35, kGeomTol);
  EXPECT_EQ(defaults.get_color_str("fill"), "green");
  EXPECT_TRUE(defaults.edge_size_set());
  EXPECT_NEAR(defaults.get_edge_size(), 0.0, kGeomTol);
}

// Covers XY format utils comms pulse behavior: preserves loose optional field behavior.
TEST(XYFormatUtilsCommsPulseTest, PreservesLooseOptionalFieldBehavior)
{
  XYCommsPulse pulse = string2CommsPulse(
      "sx=1,sy=2,tx=3,ty=4,beam_width=-5,duration=-7,fill=-2,"
      "ptype=nrep,active=maybe,fill_transparency=2,edge_size=-1,"
      "vertex_size=-2,fill_color=notacolor");

  ASSERT_TRUE(pulse.valid());
  EXPECT_NEAR(pulse.get_beam_width(), 0.0, kGeomTol);
  EXPECT_TRUE(pulse.duration_set());
  EXPECT_NEAR(pulse.get_duration(), -1.0, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(), 0.0, kGeomTol);
  EXPECT_EQ(pulse.get_pulse_type(), "nrep");
  EXPECT_TRUE(pulse.active());
  EXPECT_TRUE(pulse.transparency_set());
  EXPECT_NEAR(pulse.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(pulse.edge_size_set());
  EXPECT_NEAR(pulse.get_edge_size(), 0.0, kGeomTol);
  EXPECT_FALSE(pulse.vertex_size_set());
  EXPECT_FALSE(pulse.get_color_str("fill").empty());
}

// Covers XY comms pulse behavior: builds triangle at target with beam width perpendicular to path.
TEST(XYCommsPulseTest, BuildsTriangleAtTargetWithBeamWidthPerpendicularToPath)
{
  XYCommsPulse pulse;
  EXPECT_FALSE(pulse.valid());
  pulse.set_sx(0);
  pulse.set_sy(0);
  pulse.set_tx(0);
  pulse.set_ty(100);
  EXPECT_TRUE(pulse.valid());
  pulse.set_beam_width(20);
  pulse.set_duration(4);
  pulse.set_time(10);

  EXPECT_TRUE(pulse.get_triangle(9.99).empty());

  std::vector<double> triangle = pulse.get_triangle(12);
  ASSERT_EQ(triangle.size(), 6u);
  EXPECT_NEAR(triangle[0], 0.0, kGeomTol);
  EXPECT_NEAR(triangle[1], 0.0, kGeomTol);
  EXPECT_NEAR(triangle[2], 10.0, kLooseGeomTol);
  EXPECT_NEAR(triangle[3], 100.0, kLooseGeomTol);
  EXPECT_NEAR(triangle[4], -10.0, kLooseGeomTol);
  EXPECT_NEAR(triangle[5], 100.0, kLooseGeomTol);

  EXPECT_EQ(pulse.get_triangle(14).size(), 6u);
  EXPECT_TRUE(pulse.get_triangle(14.01).empty());
}

// Covers XY comms pulse behavior: builds eastbound triangle and handles zero beam width.
TEST(XYCommsPulseTest, BuildsEastboundTriangleAndHandlesZeroBeamWidth)
{
  XYCommsPulse pulse(0, 0, 100, 0);
  pulse.set_beam_width(30);
  pulse.set_duration(5);

  std::vector<double> eastbound = pulse.get_triangle(2.5);
  ASSERT_EQ(eastbound.size(), 6u);
  EXPECT_NEAR(eastbound[0], 0.0, kGeomTol);
  EXPECT_NEAR(eastbound[1], 0.0, kGeomTol);
  EXPECT_NEAR(eastbound[2], 100.0, kLooseGeomTol);
  EXPECT_NEAR(eastbound[3], -15.0, kLooseGeomTol);
  EXPECT_NEAR(eastbound[4], 100.0, kLooseGeomTol);
  EXPECT_NEAR(eastbound[5], 15.0, kLooseGeomTol);

  pulse.set_beam_width(-1);
  std::vector<double> zero_width = pulse.get_triangle(2.5);
  ASSERT_EQ(zero_width.size(), 6u);
  EXPECT_NEAR(zero_width[2], 100.0, kGeomTol);
  EXPECT_NEAR(zero_width[3], 0.0, kGeomTol);
  EXPECT_NEAR(zero_width[4], 100.0, kGeomTol);
  EXPECT_NEAR(zero_width[5], 0.0, kGeomTol);
}

// Covers XY comms pulse behavior: fades fill over duration.
TEST(XYCommsPulseTest, FadesFillOverDuration)
{
  XYCommsPulse pulse(0, 0, 10, 0);
  pulse.set_fill(0.6);
  pulse.set_duration(3);
  pulse.set_time(20);

  EXPECT_NEAR(pulse.get_fill(19), 0.6, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(20), 0.6, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(21.5), 0.3, kGeomTol);
  EXPECT_NEAR(pulse.get_fill(23), 0.0, kGeomTol);
}

// Covers XY comms pulse behavior: zero duration pulse exists only at post time.
TEST(XYCommsPulseTest, ZeroDurationPulseExistsOnlyAtPostTime)
{
  XYCommsPulse pulse(0, 0, 10, 0);
  pulse.set_duration(0);
  pulse.set_time(20);

  EXPECT_EQ(pulse.get_triangle(19.99).size(), 0u);
  EXPECT_EQ(pulse.get_triangle(20).size(), 6u);
  EXPECT_EQ(pulse.get_triangle(20.01).size(), 0u);
  EXPECT_NEAR(pulse.get_fill(20), pulse.get_fill(), kGeomTol);
  EXPECT_NEAR(pulse.get_fill(20.01), 0.0, kGeomTol);
}

// Covers XY comms pulse behavior: documents current target getter quirk.
TEST(XYCommsPulseTest, DocumentsCurrentTargetGetterQuirk)
{
  XYCommsPulse pulse(1, 2, 30, 40);

  EXPECT_NEAR(pulse.get_sx(), 1.0, kGeomTol);
  EXPECT_NEAR(pulse.get_sy(), 2.0, kGeomTol);
  EXPECT_NEAR(pulse.get_tx(), 1.0, kGeomTol);
  EXPECT_NEAR(pulse.get_ty(), 2.0, kGeomTol);

  std::string spec = pulse.get_spec();
  EXPECT_TRUE(stringContains(spec, "tx=30"));
  EXPECT_TRUE(stringContains(spec, "ty=40"));
}

// Covers XY comms pulse behavior: serializes viewer compatible spec.
TEST(XYCommsPulseTest, SerializesViewerCompatibleSpec)
{
  XYCommsPulse pulse(1.234567, -2.345678, 9.876543, 8.765432);
  pulse.set_beam_width(12.3456);
  pulse.set_fill(0.45);
  pulse.set_pulse_type("nrep");
  pulse.set_label("alpha2bravo");
  pulse.set_time(12.345);
  pulse.set_duration(6.789);
  pulse.set_color("fill", "orange");

  std::string spec = pulse.get_spec("active=true");
  EXPECT_TRUE(stringContains(spec, "sx=1.23457"));
  EXPECT_TRUE(stringContains(spec, "sy=-2.34568"));
  EXPECT_TRUE(stringContains(spec, "tx=9.87654"));
  EXPECT_TRUE(stringContains(spec, "ty=8.76543"));
  EXPECT_TRUE(stringContains(spec, "beam_width=12.3456"));
  EXPECT_TRUE(stringContains(spec, "fill=0.45"));
  EXPECT_TRUE(stringContains(spec, "ptype=nrep"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=alpha2bravo"));
  EXPECT_TRUE(stringContains(spec, "time=12.35"));
  EXPECT_TRUE(stringContains(spec, "duration=6.79"));
  EXPECT_TRUE(stringContains(spec, "fill_color=orange"));
}
