#include <gtest/gtest.h>

#include "BHV_TestFailure.h"
#include "BehaviorMarineTestUtils.h"

namespace {

class InspectableTestFailure : public BHV_TestFailure {
 public:
  using BHV_TestFailure::BHV_TestFailure;
  bool crashMode() const { return m_failure_crash; }
  bool burnMode() const { return m_failure_burn; }
  double burnTime() const { return m_failure_burn_time; }
};

}  // namespace

// Covers BHV test failure behavior: parses failure mode without triggering crash path.
TEST(BHVTestFailureTest, ParsesFailureModeWithoutTriggeringCrashPath)
{
  InspectableTestFailure behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("failure_type", "burn,0"));
  EXPECT_FALSE(behavior.crashMode());
  EXPECT_TRUE(behavior.burnMode());
  EXPECT_DOUBLE_EQ(behavior.burnTime(), 0);

  behavior.onCompleteState();

  ASSERT_TRUE(behavior.setParam("failure_type", "crash"));
  EXPECT_TRUE(behavior.crashMode());
  EXPECT_FALSE(behavior.burnMode());
  EXPECT_FALSE(behavior.setParam("unknown", "crash"));
}

// Covers BHV test failure behavior: burn aliases and malformed values keep current burn duration.
TEST(BHVTestFailureTest, BurnAliasesAndMalformedValuesKeepCurrentBurnDuration)
{
  InspectableTestFailure behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("failure_type", "hang,2.5"));
  EXPECT_FALSE(behavior.crashMode());
  EXPECT_TRUE(behavior.burnMode());
  EXPECT_DOUBLE_EQ(behavior.burnTime(), 2.5);

  ASSERT_TRUE(behavior.setParam("failure_type", "burn,bad"));
  EXPECT_FALSE(behavior.crashMode());
  EXPECT_TRUE(behavior.burnMode());
  EXPECT_DOUBLE_EQ(behavior.burnTime(), 2.5);

  ASSERT_TRUE(behavior.setParam("failure_type", "unknown_mode"));
  EXPECT_FALSE(behavior.crashMode());
  EXPECT_TRUE(behavior.burnMode());
  EXPECT_DOUBLE_EQ(behavior.burnTime(), 2.5);
}
