#include <gtest/gtest.h>

#include <memory>

#include "BHV_Trail.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV trail behavior: computes trail point and pursuit state for ledger contact.
TEST(BHVTrailTest, ComputesTrailPointAndPursuitStateForLedgerContact)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 40, 0, 2, 0, 100, 0, 2);

  BHV_Trail behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("trail_range", "50"));
  ASSERT_TRUE(behavior.setParam("trail_angle", "180"));
  ASSERT_TRUE(behavior.setParam("trail_angle_type", "relative"));
  ASSERT_TRUE(behavior.setParam("radius", "5"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "20"));
  EXPECT_FALSE(behavior.setParam("trail_angle_type", "sideways"));
  EXPECT_FALSE(behavior.setParam("mod_trail_range_pct", "0"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair pursuit;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PURSUIT", pursuit));
  EXPECT_DOUBLE_EQ(pursuit.get_ddata(), 1);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("trailpoint"), std::string::npos);
}

// Covers BHV trail behavior: outer distance suppresses pursuit and idle can post trail distance.
TEST(BHVTrailTest, OuterDistanceSuppressesPursuitAndIdleCanPostTrailDistance)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 0, 2);

  BHV_Trail behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("trail_range", "50"));
  ASSERT_TRUE(behavior.setParam("trail_angle", "180"));
  ASSERT_TRUE(behavior.setParam("trail_angle_type", "absolute"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "50"));
  ASSERT_TRUE(behavior.setParam("post_trail_dist_on_idle", "true"));
  ASSERT_TRUE(behavior.setParam("mod_trail_range", "-100"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair relevance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TRAIL_RELEVANCE", relevance));
  EXPECT_DOUBLE_EQ(relevance.get_ddata(), 0);

  VarDataPair pursuit;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PURSUIT", pursuit));
  EXPECT_DOUBLE_EQ(pursuit.get_ddata(), 0);

  behavior.clearMessages();
  behavior.onIdleState();

  VarDataPair distance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TRAIL_DISTANCE", distance));
  EXPECT_FALSE(distance.is_string());

  behavior.clearMessages();
  behavior.onRunToIdleState();
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PURSUIT", pursuit));
  EXPECT_DOUBLE_EQ(pursuit.get_ddata(), 0);
}

// Covers BHV trail behavior: ownship at trail point uses inside radius region.
TEST(BHVTrailTest, OwnshipAtTrailPointUsesInsideRadiusRegion)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 50, 0, 2, 0, 100, 0, 2);

  BHV_Trail behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("trail_range", "50"));
  ASSERT_TRUE(behavior.setParam("trail_angle", "180"));
  ASSERT_TRUE(behavior.setParam("trail_angle_type", "relative"));
  ASSERT_TRUE(behavior.setParam("radius", "5"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "20"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair region;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "REGION", region));
  EXPECT_EQ(region.get_sdata(), "Inside radius");

  VarDataPair distance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TRAIL_DISTANCE", distance));
  EXPECT_DOUBLE_EQ(distance.get_ddata(), 0);
}

// Covers BHV trail behavior: spawnable template can request contact manager alerts.
TEST(BHVTrailTest, SpawnableTemplateCanRequestContactManagerAlerts)
{
  BHV_Trail behavior(makeCourseSpeedDomain());

  behavior.setDynamicallySpawnable(true);
  ASSERT_TRUE(behavior.setParam("updates", "TRAIL_UPDATES"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "120"));
  ASSERT_TRUE(behavior.setParam("no_alert_request", "false"));

  behavior.onHelmStart();

  VarDataPair request;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BCM_ALERT_REQUEST", request));
  EXPECT_NE(request.get_sdata().find("id="), std::string::npos);
  EXPECT_NE(request.get_sdata().find("on_flag=TRAIL_UPDATES=name=$[VNAME] # contact=$[VNAME]"),
            std::string::npos);
  EXPECT_NE(request.get_sdata().find("alert_range=120"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("cpa_range=125"), std::string::npos);
}
