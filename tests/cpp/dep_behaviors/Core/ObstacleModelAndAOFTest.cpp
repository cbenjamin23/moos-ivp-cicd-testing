#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "AOF_AvoidObstacle.h"
#include "AOF_AvoidObstacleX.h"
#include "DepBehaviorTestUtils.h"
#include "ObShipModel.h"
#include "RefineryObAvoid.h"
#include "XYFormatUtilsPoly.h"
#include "XYPolygon.h"

namespace {

ObShipModel makeConfiguredObstacleModel(double heading = 45)
{
  ObShipModel model;
  EXPECT_EQ(model.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(model.setMinUtilCPA(8), "");
  EXPECT_EQ(model.setMaxUtilCPA(20), "");
  EXPECT_EQ(model.setAllowableTTC(30), "");
  EXPECT_EQ(model.setPwtInnerDist(20), "");
  EXPECT_EQ(model.setPwtOuterDist(80), "");
  EXPECT_EQ(model.setCompletedDist(120), "");
  EXPECT_TRUE(model.setPose(0, 0, heading, 2));
  return model;
}

class ExposedRefineryObAvoid : public RefineryObAvoid {
public:
  explicit ExposedRefineryObAvoid(IvPDomain domain) : RefineryObAvoid(domain) {}
  using RefineryObAvoid::getHdgSnappedHigh;
  using RefineryObAvoid::getHdgSnappedLow;
  using RefineryObAvoid::getHdgSnappedProx;
  using RefineryObAvoid::getSpdSnappedProx;
};

}  // namespace

// Covers obstacle model and AOF behavior: ObShipModel tracks range buffers and priority relevance.
TEST(ObstacleModelAndAOFTest, ObShipModelTracksRangeBuffersAndPriorityRelevance)
{
  ObShipModel model = makeConfiguredObstacleModel();

  EXPECT_TRUE(model.isValid());
  EXPECT_TRUE(model.paramIsSet("osx"));
  EXPECT_TRUE(model.paramIsSet("osy"));
  EXPECT_TRUE(model.paramIsSet("osh"));
  EXPECT_TRUE(model.paramIsSet("min_util_cpa"));
  EXPECT_TRUE(model.paramIsSet("max_util_cpa"));
  EXPECT_TRUE(model.paramIsSet("allowable_ttc"));
  EXPECT_FALSE(model.ownshipInObstacle());
  EXPECT_FALSE(model.ownshipInObstacleBuffMin());
  EXPECT_FALSE(model.ownshipInObstacleBuffMax());

  EXPECT_DOUBLE_EQ(model.getRange(), 40);
  EXPECT_GT(model.getRangeRelevance(), 0);
  EXPECT_LT(model.getRangeRelevance(), 1);
  EXPECT_DOUBLE_EQ(model.getRangeRelevance(), (80.0 - 40.0) / (80.0 - 20.0));

  EXPECT_TRUE(model.getObstacle().is_convex());
  EXPECT_TRUE(model.getObstacleBuffMin().is_convex());
  EXPECT_TRUE(model.getObstacleBuffMax().is_convex());
  EXPECT_EQ(model.getObstacleLabel(), "obstacle_alpha");
  EXPECT_EQ(model.getPassingSide(), "star");
  EXPECT_GE(model.getObcentBng(), 0);
  EXPECT_LT(model.getObcentBng(), 360);
  EXPECT_GE(model.getObcentRelBng(), 0);
  EXPECT_LT(model.getObcentRelBng(), 360);
  EXPECT_TRUE(model.osHdgInPlatMajor(270));
  EXPECT_TRUE(model.osSpdInPlatMinor(0));
}

// Covers obstacle model and AOF behavior: ObShipModel range relevance clamps at inner and outer distances.
TEST(ObstacleModelAndAOFTest, ObShipModelRangeRelevanceClampsAtInnerAndOuterDistances)
{
  ObShipModel inside_inner = makeConfiguredObstacleModel();
  EXPECT_TRUE(inside_inner.setPose(30, 0, 90, 2));
  EXPECT_DOUBLE_EQ(inside_inner.getRange(), 10);
  EXPECT_DOUBLE_EQ(inside_inner.getRangeRelevance(), 1);

  ObShipModel at_outer = makeConfiguredObstacleModel();
  EXPECT_TRUE(at_outer.setPose(-40, 0, 90, 2));
  EXPECT_DOUBLE_EQ(at_outer.getRange(), 80);
  EXPECT_DOUBLE_EQ(at_outer.getRangeRelevance(), 0);

  ObShipModel outside_outer = makeConfiguredObstacleModel();
  EXPECT_TRUE(outside_outer.setPose(-80, 0, 90, 2));
  EXPECT_GT(outside_outer.getRange(), outside_outer.getPwtOuterDist());
  EXPECT_DOUBLE_EQ(outside_outer.getRangeRelevance(), 0);
}

// Covers obstacle model and AOF behavior: ObShipModel defaults and zero width priority band are explicit.
TEST(ObstacleModelAndAOFTest, ObShipModelDefaultsAndZeroWidthPriorityBandAreExplicit)
{
  ObShipModel unset;
  EXPECT_FALSE(unset.isValid());
  EXPECT_FALSE(unset.paramIsSet("osx"));
  EXPECT_FALSE(unset.paramIsSet("unknown"));
  EXPECT_DOUBLE_EQ(unset.getRange(), -1);
  EXPECT_EQ(unset.getFailedExpandPolyStr(), "");

  EXPECT_FALSE(unset.setPose(0, 0, 450, 2));
  EXPECT_TRUE(unset.paramIsSet("osx"));
  EXPECT_DOUBLE_EQ(unset.getOSH(), 90);

  ObShipModel zero_width = makeConfiguredObstacleModel(90);
  EXPECT_EQ(zero_width.setPwtInnerDist(40), "");
  EXPECT_EQ(zero_width.setPwtOuterDist(40), "");
  EXPECT_DOUBLE_EQ(zero_width.getRange(), 40);
  EXPECT_DOUBLE_EQ(zero_width.getRangeRelevance(), 0);

  EXPECT_TRUE(zero_width.setPose(30, 0, 90, 2));
  EXPECT_DOUBLE_EQ(zero_width.getRange(), 10);
  EXPECT_DOUBLE_EQ(zero_width.getRangeRelevance(), 1);
}

// Covers obstacle model and AOF behavior: ObShipModel validity requires each pose and buffer input.
TEST(ObstacleModelAndAOFTest, ObShipModelValidityRequiresEachPoseAndBufferInput)
{
  ObShipModel missing_osx;
  EXPECT_EQ(missing_osx.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_osx.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_osx.setMaxUtilCPA(20), "");
  EXPECT_EQ(missing_osx.setAllowableTTC(30), "");
  EXPECT_TRUE(missing_osx.setPoseOSY(0));
  EXPECT_TRUE(missing_osx.setPoseOSH(90));
  EXPECT_FALSE(missing_osx.isValid());

  ObShipModel missing_osy;
  EXPECT_EQ(missing_osy.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_osy.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_osy.setMaxUtilCPA(20), "");
  EXPECT_EQ(missing_osy.setAllowableTTC(30), "");
  EXPECT_TRUE(missing_osy.setPoseOSX(0));
  EXPECT_TRUE(missing_osy.setPoseOSH(90));
  EXPECT_FALSE(missing_osy.isValid());

  ObShipModel missing_osh;
  EXPECT_EQ(missing_osh.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_osh.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_osh.setMaxUtilCPA(20), "");
  EXPECT_EQ(missing_osh.setAllowableTTC(30), "");
  EXPECT_TRUE(missing_osh.setPoseOSX(0));
  EXPECT_TRUE(missing_osh.setPoseOSY(0));
  EXPECT_FALSE(missing_osh.isValid());

  ObShipModel missing_obstacle;
  EXPECT_EQ(missing_obstacle.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_obstacle.setMaxUtilCPA(20), "");
  EXPECT_EQ(missing_obstacle.setAllowableTTC(30), "");
  EXPECT_FALSE(missing_obstacle.setPose(0, 0, 90, 2));
  EXPECT_FALSE(missing_obstacle.isValid());
}

// Covers obstacle model and AOF behavior: ObShipModel classifies passing side and aft obstacles from ownship heading.
TEST(ObstacleModelAndAOFTest, ObShipModelClassifiesPassingSideAndAftObstaclesFromOwnshipHeading)
{
  ObShipModel starboard_pass = makeConfiguredObstacleModel(45);
  EXPECT_EQ(starboard_pass.getPassingSide(), "star");
  EXPECT_FALSE(starboard_pass.isObstacleAft(20));

  ObShipModel port_pass = makeConfiguredObstacleModel(135);
  EXPECT_EQ(port_pass.getPassingSide(), "port");
  EXPECT_FALSE(port_pass.isObstacleAft(20));

  ObShipModel directly_ahead = makeConfiguredObstacleModel(90);
  EXPECT_EQ(directly_ahead.getPassingSide(), "n/a");
  EXPECT_FALSE(directly_ahead.isObstacleAft(20));
  EXPECT_DOUBLE_EQ(directly_ahead.getRangeInOSH(), 40);
  EXPECT_DOUBLE_EQ(directly_ahead.getCPAInOSH(), 0);

  ObShipModel aft_obstacle = makeConfiguredObstacleModel(270);
  EXPECT_EQ(aft_obstacle.getPassingSide(), "n/a");
  EXPECT_TRUE(aft_obstacle.isObstacleAft(20));
  EXPECT_DOUBLE_EQ(aft_obstacle.getRangeInOSH(), -1);
}

// Covers obstacle model and AOF behavior: ObShipModel classifies major and minor basin boundaries.
TEST(ObstacleModelAndAOFTest, ObShipModelClassifiesMajorAndMinorBasinBoundaries)
{
  ObShipModel starboard_pass = makeConfiguredObstacleModel(45);
  EXPECT_TRUE(starboard_pass.osHdgInBasinMajor(135));
  EXPECT_TRUE(starboard_pass.osHdgInBasinMajor(135, true));
  EXPECT_FALSE(starboard_pass.osHdgInBasinMajor(45));
  EXPECT_FALSE(starboard_pass.osHdgInBasinMajor(270));
  EXPECT_TRUE(starboard_pass.osHdgInPlatMajor(270));

  ObShipModel port_pass = makeConfiguredObstacleModel(135);
  EXPECT_TRUE(port_pass.osHdgInBasinMajor(45));
  EXPECT_TRUE(port_pass.osHdgInBasinMajor(45, true));
  EXPECT_FALSE(port_pass.osHdgInBasinMajor(135));
  EXPECT_FALSE(port_pass.osHdgInBasinMajor(270));

  ObShipModel head_on = makeConfiguredObstacleModel(90);
  EXPECT_FALSE(head_on.osHdgInBasinMajor(90, true));
  EXPECT_TRUE(head_on.osHdgSpdInBasinMinor(90, 2));
  EXPECT_FALSE(head_on.osHdgSpdInBasinMinor(90, 1));
  EXPECT_FALSE(head_on.osHdgSpdInBasinMinor(270, 2));
  EXPECT_EQ(head_on.setAllowableTTC(0), "");
  EXPECT_FALSE(head_on.osHdgSpdInBasinMinor(90, 2));
}

// Covers obstacle model and AOF behavior: ObShipModel rejects invalid ordering and ownship inside obstacle.
TEST(ObstacleModelAndAOFTest, ObShipModelRejectsInvalidOrderingAndOwnshipInsideObstacle)
{
  ObShipModel model;
  EXPECT_NE(model.setObstacle("pts={0,0:10,0:0,10:10,10}"), "");
  EXPECT_NE(model.setMinUtilCPA(-1), "");
  EXPECT_NE(model.setMaxUtilCPA(-1), "");
  EXPECT_NE(model.setAllowableTTC(-1), "");
  EXPECT_NE(model.setPwtInnerDist(-1), "");
  EXPECT_NE(model.setPwtOuterDist(-1), "");
  EXPECT_NE(model.setCompletedDist(-1), "");
  EXPECT_EQ(model.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(model.setMaxUtilCPA(5), "");
  EXPECT_NE(model.setMinUtilCPA(10), "");

  ObShipModel cpa_min_first;
  EXPECT_EQ(cpa_min_first.setMinUtilCPA(10), "");
  EXPECT_NE(cpa_min_first.setMaxUtilCPA(5), "");

  ObShipModel inside = makeConfiguredObstacleModel();
  EXPECT_TRUE(inside.setPose(50, 0, 90, 2));
  EXPECT_TRUE(inside.ownshipInObstacle());
}

// Covers obstacle model and AOF behavior: ObShipModel setter ordering auto adjusts until conflicting explicit values.
TEST(ObstacleModelAndAOFTest, ObShipModelSetterOrderingAutoAdjustsUntilConflictingExplicitValues)
{
  ObShipModel model;
  EXPECT_EQ(model.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(model.setPwtOuterDist(8), "");
  EXPECT_DOUBLE_EQ(model.getPwtInnerDist(), 8);
  EXPECT_DOUBLE_EQ(model.getPwtOuterDist(), 8);
  EXPECT_NE(model.setCompletedDist(6), "");

  ObShipModel completed_first;
  EXPECT_EQ(completed_first.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(completed_first.setCompletedDist(6), "");
  EXPECT_DOUBLE_EQ(completed_first.getPwtInnerDist(), 6);
  EXPECT_DOUBLE_EQ(completed_first.getPwtOuterDist(), 6);
  EXPECT_DOUBLE_EQ(completed_first.getCompletedDist(), 6);
  EXPECT_NE(completed_first.setPwtOuterDist(8), "");

  ObShipModel cpa_model;
  EXPECT_EQ(cpa_model.setMaxUtilCPA(5), "");
  EXPECT_DOUBLE_EQ(cpa_model.getMinUtilCPA(), 5);
  EXPECT_NE(cpa_model.setMinUtilCPA(6), "");
}

// Covers obstacle model and AOF behavior: ObShipModel setter auto adjusts implicit distance and CPA companions.
TEST(ObstacleModelAndAOFTest, ObShipModelSetterAutoAdjustsImplicitDistanceAndCPACompanions)
{
  ObShipModel inner_first;
  EXPECT_EQ(inner_first.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(inner_first.setPwtInnerDist(60), "");
  EXPECT_DOUBLE_EQ(inner_first.getPwtInnerDist(), 60);
  EXPECT_DOUBLE_EQ(inner_first.getPwtOuterDist(), 60);
  EXPECT_DOUBLE_EQ(inner_first.getCompletedDist(), 60);

  ObShipModel completed_first;
  EXPECT_EQ(completed_first.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(completed_first.setCompletedDist(6), "");
  EXPECT_DOUBLE_EQ(completed_first.getPwtInnerDist(), 6);
  EXPECT_DOUBLE_EQ(completed_first.getPwtOuterDist(), 6);

  ObShipModel min_cpa_first;
  EXPECT_EQ(min_cpa_first.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(min_cpa_first.setMinUtilCPA(30), "");
  EXPECT_DOUBLE_EQ(min_cpa_first.getMinUtilCPA(), 30);
  EXPECT_DOUBLE_EQ(min_cpa_first.getMaxUtilCPA(), 30);
}

// Covers obstacle model and AOF behavior: ObShipModel reports every explicit distance ordering conflict.
TEST(ObstacleModelAndAOFTest, ObShipModelReportsEveryExplicitDistanceOrderingConflict)
{
  ObShipModel outer_then_inner;
  EXPECT_EQ(outer_then_inner.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(outer_then_inner.setPwtOuterDist(10), "");
  EXPECT_NE(outer_then_inner.setPwtInnerDist(12), "");

  ObShipModel completed_then_inner;
  EXPECT_EQ(completed_then_inner.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(completed_then_inner.setCompletedDist(10), "");
  EXPECT_NE(completed_then_inner.setPwtInnerDist(12), "");

  ObShipModel inner_then_outer;
  EXPECT_EQ(inner_then_outer.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(inner_then_outer.setPwtInnerDist(10), "");
  EXPECT_NE(inner_then_outer.setPwtOuterDist(8), "");

  ObShipModel inner_then_completed;
  EXPECT_EQ(inner_then_completed.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(inner_then_completed.setPwtInnerDist(10), "");
  EXPECT_NE(inner_then_completed.setCompletedDist(8), "");

  ObShipModel repaired_after_error = makeConfiguredObstacleModel();
  EXPECT_NE(repaired_after_error.setCompletedDist(30), "");
  EXPECT_NE(repaired_after_error.setPwtOuterDist(40), "");
  EXPECT_TRUE(repaired_after_error.isValid());
  EXPECT_DOUBLE_EQ(repaired_after_error.getPwtOuterDist(),
                   repaired_after_error.getCompletedDist());
}

// Covers obstacle model and AOF behavior: ObShipModel pose setters update derived range and heading state.
TEST(ObstacleModelAndAOFTest, ObShipModelPoseSettersUpdateDerivedRangeAndHeadingState)
{
  ObShipModel model = makeConfiguredObstacleModel(90);
  EXPECT_DOUBLE_EQ(model.getOSX(), 0);
  EXPECT_DOUBLE_EQ(model.getOSY(), 0);
  EXPECT_DOUBLE_EQ(model.getOSH(), 90);
  EXPECT_DOUBLE_EQ(model.getOSV(), 2);
  EXPECT_DOUBLE_EQ(model.getRange(), 40);

  EXPECT_TRUE(model.setPoseOSX(10));
  EXPECT_DOUBLE_EQ(model.getOSX(), 10);
  EXPECT_DOUBLE_EQ(model.getRange(), 30);

  EXPECT_TRUE(model.setPoseOSY(5));
  EXPECT_DOUBLE_EQ(model.getOSY(), 5);
  EXPECT_GT(model.getRange(), 29);
  EXPECT_LT(model.getRange(), 31);

  EXPECT_TRUE(model.setPoseOSH(450));
  EXPECT_DOUBLE_EQ(model.getOSH(), 450);
  EXPECT_TRUE(model.setPoseOSV(3));
  EXPECT_DOUBLE_EQ(model.getOSV(), 3);
}

// Covers obstacle model and AOF behavior: ObShipModel pose speed semantics distinguish optional and direct updates.
TEST(ObstacleModelAndAOFTest, ObShipModelPoseSpeedSemanticsDistinguishOptionalAndDirectUpdates)
{
  ObShipModel model = makeConfiguredObstacleModel(90);
  EXPECT_DOUBLE_EQ(model.getOSV(), 2);

  EXPECT_TRUE(model.setPose(0, 0, 90, -1));
  EXPECT_DOUBLE_EQ(model.getOSV(), 2);

  EXPECT_TRUE(model.setPoseOSV(-1));
  EXPECT_DOUBLE_EQ(model.getOSV(), -1);
  EXPECT_TRUE(model.paramIsSet("osv"));
}

// Covers obstacle model and AOF behavior: ObShipModel flexible CPA buffers shrink near obstacle.
TEST(ObstacleModelAndAOFTest, ObShipModelFlexibleCPABuffersShrinkNearObstacle)
{
  ObShipModel model = makeConfiguredObstacleModel(90);
  EXPECT_TRUE(model.setPose(39, 0, 90, 2));

  EXPECT_DOUBLE_EQ(model.getRange(), 1);
  EXPECT_DOUBLE_EQ(model.getMinUtilCPAFlex(), 0);
  EXPECT_DOUBLE_EQ(model.getMaxUtilCPAFlex(), 0);
  EXPECT_FALSE(model.ownshipInObstacle());
  EXPECT_TRUE(model.getObstacleBuffMin().is_convex());
  EXPECT_TRUE(model.getObstacleBuffMax().is_convex());
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle penalizes unsafe time to collision and side crossing.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstaclePenalizesUnsafeTimeToCollisionAndSideCrossing)
{
  IvPDomain domain = makeDepBehaviorDomain();
  XYPolygon obstacle = string2Poly(depObstaclePolySpec());

  AOF_AvoidObstacle aof(domain);
  aof.setObstacleOrig(obstacle);
  aof.setObstacleBuff(obstacle);
  ASSERT_TRUE(aof.setParam("os_x", 0));
  ASSERT_TRUE(aof.setParam("os_y", 0));
  ASSERT_TRUE(aof.setParam("os_h", 0));
  ASSERT_TRUE(aof.setParam("allowable_ttc", 20));
  ASSERT_FALSE(aof.setParam("allowable_ttc", 0));
  ASSERT_TRUE(aof.initialize());

  const IvPBox toward_fast = makeCourseSpeedEvalBox(90, 3);
  const IvPBox away_fast = makeCourseSpeedEvalBox(270, 3);
  const IvPBox crossing_side = makeCourseSpeedEvalBox(181, 1);
  const IvPBox stopped = makeCourseSpeedEvalBox(90, 0);

  EXPECT_DOUBLE_EQ(aof.evalBox(&toward_fast), 0);
  EXPECT_DOUBLE_EQ(aof.evalBox(&away_fast), 100);
  EXPECT_DOUBLE_EQ(aof.evalBox(&crossing_side), 0);
  EXPECT_DOUBLE_EQ(aof.evalBox(&stopped), 100);
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle allows slow collision beyond ttc window.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleAllowsSlowCollisionBeyondTTCWindow)
{
  IvPDomain domain = makeDepBehaviorDomain();
  XYPolygon obstacle = string2Poly(depObstaclePolySpec());

  AOF_AvoidObstacle aof(domain);
  aof.setObstacleOrig(obstacle);
  aof.setObstacleBuff(obstacle);
  ASSERT_TRUE(aof.setParam("os_x", 0));
  ASSERT_TRUE(aof.setParam("os_y", 0));
  ASSERT_TRUE(aof.setParam("os_h", 0));
  ASSERT_TRUE(aof.setParam("allowable_ttc", 20));
  ASSERT_TRUE(aof.initialize());

  const IvPBox toward_slow = makeCourseSpeedEvalBox(90, 1);
  const IvPBox toward_fast = makeCourseSpeedEvalBox(90, 5);

  EXPECT_DOUBLE_EQ(aof.evalBox(&toward_slow), 100);
  EXPECT_DOUBLE_EQ(aof.evalBox(&toward_fast), 0);
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle treats exact ttc boundary as unsafe.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleTreatsExactTTCBoundaryAsUnsafe)
{
  IvPDomain domain = makeDepBehaviorDomain();
  XYPolygon obstacle = string2Poly(depObstaclePolySpec());

  AOF_AvoidObstacle aof(domain);
  aof.setObstacleOrig(obstacle);
  aof.setObstacleBuff(obstacle);
  ASSERT_TRUE(aof.setParam("os_x", 0));
  ASSERT_TRUE(aof.setParam("os_y", 0));
  ASSERT_TRUE(aof.setParam("os_h", 0));
  ASSERT_TRUE(aof.setParam("allowable_ttc", 20));
  ASSERT_TRUE(aof.initialize());

  const IvPBox exact_ttc = makeCourseSpeedEvalBox(90, 2);
  const IvPBox beyond_ttc = makeCourseSpeedEvalBox(90, 1);

  EXPECT_DOUBLE_EQ(aof.evalBox(&exact_ttc), 0);
  EXPECT_DOUBLE_EQ(aof.evalBox(&beyond_ttc), 100);
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle initialization requires course and speed domain.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleInitializationRequiresCourseAndSpeedDomain)
{
  IvPDomain no_speed;
  no_speed.addDomain("course", 0, 359, 360);
  AOF_AvoidObstacle missing_speed(no_speed);
  EXPECT_FALSE(missing_speed.initialize());

  IvPDomain no_course;
  no_course.addDomain("speed", 0, 5, 6);
  AOF_AvoidObstacle missing_course(no_course);
  EXPECT_FALSE(missing_course.initialize());
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle initialization guards required inputs.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleInitializationGuardsRequiredInputs)
{
  IvPDomain domain = makeDepBehaviorDomain();
  XYPolygon obstacle = string2Poly(depObstaclePolySpec());

  AOF_AvoidObstacle missing_pose(domain);
  missing_pose.setObstacleOrig(obstacle);
  missing_pose.setObstacleBuff(obstacle);
  EXPECT_FALSE(missing_pose.initialize());

  AOF_AvoidObstacle ownship_inside(domain);
  ownship_inside.setObstacleOrig(obstacle);
  ownship_inside.setObstacleBuff(obstacle);
  ASSERT_TRUE(ownship_inside.setParam("os_x", 50));
  ASSERT_TRUE(ownship_inside.setParam("os_y", 0));
  ASSERT_TRUE(ownship_inside.setParam("os_h", 90));
  ASSERT_TRUE(ownship_inside.setParam("allowable_ttc", 20));
  EXPECT_FALSE(ownship_inside.initialize());

  AOF_AvoidObstacle missing_heading(domain);
  missing_heading.setObstacleOrig(obstacle);
  missing_heading.setObstacleBuff(obstacle);
  ASSERT_TRUE(missing_heading.setParam("os_x", 0));
  ASSERT_TRUE(missing_heading.setParam("os_y", 0));
  ASSERT_TRUE(missing_heading.setParam("allowable_ttc", 20));
  EXPECT_FALSE(missing_heading.initialize());
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle initialization rejects malformed obstacles and missing ttc.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleInitializationRejectsMalformedObstaclesAndMissingTTC)
{
  IvPDomain domain = makeDepBehaviorDomain();
  XYPolygon obstacle = string2Poly(depObstaclePolySpec());
  XYPolygon nonconvex = string2Poly("pts={0,0:10,0:0,10:10,10}");

  AOF_AvoidObstacle missing_ttc(domain);
  missing_ttc.setObstacleOrig(obstacle);
  missing_ttc.setObstacleBuff(obstacle);
  ASSERT_TRUE(missing_ttc.setParam("os_x", 0));
  ASSERT_TRUE(missing_ttc.setParam("os_y", 0));
  ASSERT_TRUE(missing_ttc.setParam("os_h", 90));
  EXPECT_FALSE(missing_ttc.initialize());
  EXPECT_FALSE(missing_ttc.setParam("unknown", 1));
  EXPECT_FALSE(missing_ttc.setParam("unknown", "1"));

  AOF_AvoidObstacle bad_original(domain);
  bad_original.setObstacleOrig(nonconvex);
  bad_original.setObstacleBuff(obstacle);
  ASSERT_TRUE(bad_original.setParam("os_x", 0));
  ASSERT_TRUE(bad_original.setParam("os_y", 0));
  ASSERT_TRUE(bad_original.setParam("os_h", 90));
  ASSERT_TRUE(bad_original.setParam("allowable_ttc", 20));
  EXPECT_FALSE(bad_original.initialize());

  AOF_AvoidObstacle bad_buffer(domain);
  bad_buffer.setObstacleOrig(obstacle);
  bad_buffer.setObstacleBuff(nonconvex);
  ASSERT_TRUE(bad_buffer.setParam("os_x", 0));
  ASSERT_TRUE(bad_buffer.setParam("os_y", 0));
  ASSERT_TRUE(bad_buffer.setParam("os_h", 90));
  ASSERT_TRUE(bad_buffer.setParam("allowable_ttc", 20));
  EXPECT_FALSE(bad_buffer.initialize());
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle x initialization requires course and speed domain.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleXInitializationRequiresCourseAndSpeedDomain)
{
  ObShipModel model = makeConfiguredObstacleModel();

  IvPDomain no_speed;
  no_speed.addDomain("course", 0, 359, 360);
  AOF_AvoidObstacleX missing_speed(no_speed);
  missing_speed.setObShipModel(model);
  EXPECT_FALSE(missing_speed.initialize());

  IvPDomain no_course;
  no_course.addDomain("speed", 0, 5, 6);
  AOF_AvoidObstacleX missing_course(no_course);
  missing_course.setObShipModel(model);
  EXPECT_FALSE(missing_course.initialize());
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle x uses ObShipModel basin and plateau semantics.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleXUsesObShipModelBasinAndPlateauSemantics)
{
  IvPDomain domain = makeDepBehaviorDomain();
  ObShipModel model = makeConfiguredObstacleModel();

  AOF_AvoidObstacleX aof(domain);
  aof.setObShipModel(model);
  EXPECT_FALSE(aof.setParam("os_x", 0));
  ASSERT_TRUE(aof.initialize());

  const IvPBox toward_fast = makeCourseSpeedEvalBox(90, 5);
  const IvPBox away_fast = makeCourseSpeedEvalBox(270, 5);
  const IvPBox stopped = makeCourseSpeedEvalBox(90, 0);

  const double toward_utility = aof.evalBox(&toward_fast);
  const double away_utility = aof.evalBox(&away_fast);
  const double stopped_utility = aof.evalBox(&stopped);

  EXPECT_GE(toward_utility, 0);
  EXPECT_LE(toward_utility, 100);
  EXPECT_GE(away_utility, 0);
  EXPECT_LE(away_utility, 100);
  EXPECT_GE(stopped_utility, 0);
  EXPECT_LE(stopped_utility, 100);
  EXPECT_GT(away_utility, toward_utility);
  EXPECT_GE(stopped_utility, toward_utility);

  double min_seen = 100;
  double max_seen = 0;
  for(unsigned int course = 0; course < 360; ++course) {
    for(unsigned int speed = 0; speed <= 5; ++speed) {
      const IvPBox sample = makeCourseSpeedEvalBox(course, speed);
      const double utility = aof.evalBox(&sample);
      EXPECT_GE(utility, 0);
      EXPECT_LE(utility, 100);
      min_seen = std::min(min_seen, utility);
      max_seen = std::max(max_seen, utility);
    }
  }
  EXPECT_LT(min_seen, max_seen);
}

// Covers obstacle model and AOF behavior: AOF avoid obstacle x initialization requires configured model.
TEST(ObstacleModelAndAOFTest, AOFAvoidObstacleXInitializationRequiresConfiguredModel)
{
  IvPDomain domain = makeDepBehaviorDomain();

  AOF_AvoidObstacleX missing_model(domain);
  EXPECT_FALSE(missing_model.setParam("unknown", 1));
  EXPECT_FALSE(missing_model.setParam("unknown", "1"));
  EXPECT_FALSE(missing_model.initialize());

  ObShipModel missing_safety_params;
  EXPECT_EQ(missing_safety_params.setObstacle(depObstaclePolySpec()), "");
  EXPECT_TRUE(missing_safety_params.setPose(0, 0, 90, 2));
  AOF_AvoidObstacleX missing_safety_aof(domain);
  missing_safety_aof.setObShipModel(missing_safety_params);
  EXPECT_FALSE(missing_safety_aof.initialize());

  ObShipModel inside = makeConfiguredObstacleModel();
  EXPECT_TRUE(inside.setPose(50, 0, 90, 2));
  AOF_AvoidObstacleX ownship_inside(domain);
  ownship_inside.setObShipModel(inside);
  EXPECT_FALSE(ownship_inside.initialize());

  ObShipModel missing_y;
  EXPECT_EQ(missing_y.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_y.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_y.setMaxUtilCPA(20), "");
  EXPECT_EQ(missing_y.setAllowableTTC(30), "");
  EXPECT_TRUE(missing_y.setPoseOSX(0));
  EXPECT_TRUE(missing_y.setPoseOSH(90));
  AOF_AvoidObstacleX missing_y_aof(domain);
  missing_y_aof.setObShipModel(missing_y);
  EXPECT_FALSE(missing_y_aof.initialize());

  ObShipModel missing_heading;
  EXPECT_EQ(missing_heading.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_heading.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_heading.setMaxUtilCPA(20), "");
  EXPECT_EQ(missing_heading.setAllowableTTC(30), "");
  EXPECT_TRUE(missing_heading.setPoseOSX(0));
  EXPECT_TRUE(missing_heading.setPoseOSY(0));
  AOF_AvoidObstacleX missing_heading_aof(domain);
  missing_heading_aof.setObShipModel(missing_heading);
  EXPECT_FALSE(missing_heading_aof.initialize());

  ObShipModel missing_max_cpa;
  EXPECT_EQ(missing_max_cpa.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_max_cpa.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_max_cpa.setAllowableTTC(30), "");
  EXPECT_TRUE(missing_max_cpa.setPose(0, 0, 90, 2));
  AOF_AvoidObstacleX missing_max_cpa_aof(domain);
  missing_max_cpa_aof.setObShipModel(missing_max_cpa);
  EXPECT_FALSE(missing_max_cpa_aof.initialize());

  ObShipModel missing_ttc;
  EXPECT_EQ(missing_ttc.setObstacle(depObstaclePolySpec()), "");
  EXPECT_EQ(missing_ttc.setMinUtilCPA(8), "");
  EXPECT_EQ(missing_ttc.setMaxUtilCPA(20), "");
  EXPECT_TRUE(missing_ttc.setPose(0, 0, 90, 2));
  AOF_AvoidObstacleX missing_ttc_aof(domain);
  missing_ttc_aof.setObShipModel(missing_ttc);
  EXPECT_FALSE(missing_ttc_aof.initialize());

  ObShipModel invalid_obstacle;
  EXPECT_NE(invalid_obstacle.setObstacle("pts={0,0:10,0:0,10:10,10}"), "");
  EXPECT_EQ(invalid_obstacle.setMinUtilCPA(8), "");
  EXPECT_EQ(invalid_obstacle.setMaxUtilCPA(20), "");
  EXPECT_EQ(invalid_obstacle.setAllowableTTC(30), "");
  EXPECT_FALSE(invalid_obstacle.setPose(0, 0, 90, 2));
  AOF_AvoidObstacleX invalid_obstacle_aof(domain);
  invalid_obstacle_aof.setObShipModel(invalid_obstacle);
  EXPECT_FALSE(invalid_obstacle_aof.initialize());
}

// Covers obstacle model and AOF behavior: refinery ob avoid builds non overlapping plateau and basin regions.
TEST(ObstacleModelAndAOFTest, RefineryObAvoidBuildsNonOverlappingPlateauAndBasinRegions)
{
  IvPDomain domain = makeDepBehaviorDomain();
  ObShipModel model = makeConfiguredObstacleModel();

  RefineryObAvoid refinery(domain);
  refinery.setRefineRegions(model);

  const std::vector<IvPBox> plateaus = refinery.getPlateaus();
  const std::vector<IvPBox> basins = refinery.getBasins();

  EXPECT_FALSE(plateaus.empty());
  EXPECT_FALSE(basins.empty());
  for(const IvPBox& plateau : plateaus) {
    EXPECT_EQ(plateau.getDim(), 2);
    EXPECT_GE(plateau.getPlat(), 0);
  }
  for(const IvPBox& basin : basins) {
    EXPECT_EQ(basin.getDim(), 2);
    EXPECT_LT(basin.getPlat(), 0);
  }
}

// Covers obstacle model and AOF behavior: refinery ob avoid covers port verbose and snapping edge cases.
TEST(ObstacleModelAndAOFTest, RefineryObAvoidCoversPortVerboseAndSnappingEdgeCases)
{
  IvPDomain domain = makeDepBehaviorDomain();
  ObShipModel port_model = makeConfiguredObstacleModel(135);
  ASSERT_EQ(port_model.getPassingSide(), "port");

  ExposedRefineryObAvoid refinery(domain);
  refinery.setVerbose(true);
  refinery.setRefineRegions(port_model);
  EXPECT_FALSE(refinery.getPlateaus().empty());
  EXPECT_FALSE(refinery.getBasins().empty());

  EXPECT_DOUBLE_EQ(refinery.getHdgSnappedLow(-1), 359);
  EXPECT_DOUBLE_EQ(refinery.getHdgSnappedHigh(359.8), 0);
  EXPECT_DOUBLE_EQ(refinery.getHdgSnappedProx(359.2), 359);
  EXPECT_DOUBLE_EQ(refinery.getHdgSnappedProx(359.8), 0);
  EXPECT_DOUBLE_EQ(refinery.getSpdSnappedProx(2.4), 2);
}

// Covers obstacle model and AOF behavior: refinery ob avoid skips minor regions when ttc is zero.
TEST(ObstacleModelAndAOFTest, RefineryObAvoidSkipsMinorRegionsWhenTTCIsZero)
{
  IvPDomain domain = makeDepBehaviorDomain();
  ObShipModel zero_ttc = makeConfiguredObstacleModel(90);
  EXPECT_EQ(zero_ttc.setAllowableTTC(0), "");

  RefineryObAvoid refinery(domain);
  refinery.setRefineRegions(zero_ttc);

  EXPECT_FALSE(refinery.getPlateaus().empty());
  EXPECT_TRUE(refinery.getBasins().empty());
}

// Covers obstacle model and AOF behavior: refinery ob avoid ignores domains missing course or speed.
TEST(ObstacleModelAndAOFTest, RefineryObAvoidIgnoresDomainsMissingCourseOrSpeed)
{
  IvPDomain depth_only;
  depth_only.addDomain("depth", 0, 100, 101);
  ObShipModel model = makeConfiguredObstacleModel();

  RefineryObAvoid refinery(depth_only);
  refinery.setRefineRegions(model);
  EXPECT_TRUE(refinery.getPlateaus().empty());
  EXPECT_TRUE(refinery.getBasins().empty());

  IvPDomain course_only;
  course_only.addDomain("course", 0, 359, 360);
  RefineryObAvoid course_refinery(course_only);
  course_refinery.setRefineRegions(model);
  EXPECT_TRUE(course_refinery.getPlateaus().empty());
  EXPECT_TRUE(course_refinery.getBasins().empty());

  IvPDomain speed_only;
  speed_only.addDomain("speed", 0, 5, 6);
  RefineryObAvoid speed_refinery(speed_only);
  speed_refinery.setRefineRegions(model);
  EXPECT_TRUE(speed_refinery.getPlateaus().empty());
  EXPECT_TRUE(speed_refinery.getBasins().empty());
}
