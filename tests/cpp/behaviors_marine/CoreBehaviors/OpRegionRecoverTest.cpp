#include <gtest/gtest.h>

#include <memory>

#include "BHV_OpRegionRecover.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV op region recover behavior: posts status when ownship is inside recovery polygon.
TEST(BHVOpRegionRecoverTest, PostsStatusWhenOwnshipIsInsideRecoveryPolygon)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_OpRegionRecover behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50}"));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "10"));
  ASSERT_TRUE(behavior.setParam("trigger_exit_time", "0"));
  ASSERT_TRUE(behavior.setParam("opregion_poly_var", "OPR_POLY"));
  EXPECT_FALSE(behavior.setParam("buffer_dist", "-1"));
  EXPECT_FALSE(behavior.setParam("polygon", "bad"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair debug;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPR_DEBUG", debug));
  EXPECT_NE(debug.get_sdata().find("in poly"), std::string::npos);
}

// Covers BHV op region recover behavior: outside recovery polygon builds objective and posts turn telemetry.
TEST(BHVOpRegionRecoverTest, OutsideRecoveryPolygonBuildsObjectiveAndPostsTurnTelemetry)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 100);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 2);

  BHV_OpRegionRecover behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50},label=recovery_region"));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "0"));
  ASSERT_TRUE(behavior.setParam("trigger_exit_time", "0"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 100);

  VarDataPair debug;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPR_DEBUG", debug));
  EXPECT_EQ(debug.get_sdata(), "not in poly");

  VarDataPair turn;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPR_TURN", turn));
  EXPECT_TRUE(turn.get_sdata() == "left" || turn.get_sdata() == "right" ||
              turn.get_sdata() == "none");

  VarDataPair heading_dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPR_HDG_DIST_TO_POLY", heading_dist));
  EXPECT_DOUBLE_EQ(heading_dist.get_ddata(), -1);

  VarDataPair absolute_dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPR_ABSOLUTE_PERIM_DIST", absolute_dist));
  EXPECT_DOUBLE_EQ(absolute_dist.get_ddata(), 50);
}

// Covers BHV op region recover behavior: dynamic region var replaces mission polygon at run time.
TEST(BHVOpRegionRecoverTest, DynamicRegionVarReplacesMissionPolygonAtRunTime)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_OpRegionRecover behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={100,100:150,100:150,150:100,150}"));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "0"));
  ASSERT_TRUE(behavior.setParam("trigger_exit_time", "0"));
  ASSERT_TRUE(behavior.setParam("dynamic_region_var", "RECOVERY_REGION"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  info.setValue("RECOVERY_REGION", "pts={-20,-20:20,-20:20,20:-20,20},label=dynamic_recovery");

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair debug;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPR_DEBUG", debug));
  EXPECT_NE(debug.get_sdata().find("in poly"), std::string::npos);

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("dynamic_recovery"), std::string::npos);
}

// Covers BHV op region recover behavior: idle erases buffered recovery polygon.
TEST(BHVOpRegionRecoverTest, IdleErasesBufferedRecoveryPolygon)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_OpRegionRecover behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50},label=recovery_region"));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "10"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  behavior.onIdleState();

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("active=false"), std::string::npos);
}
