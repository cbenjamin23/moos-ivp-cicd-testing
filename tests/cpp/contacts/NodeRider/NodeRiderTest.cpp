#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "NodeRider.h"
#include "NodeRiderSet.h"

TEST(NodeRiderTest, ValidatesReservedVarsPoliciesPrecisionAndSpecs)
{
  NodeRider rider;
  EXPECT_FALSE(rider.valid());
  EXPECT_EQ(rider.getSpec(), "");
  EXPECT_FALSE(rider.setVar("NAME"));
  EXPECT_TRUE(rider.setVar("DB_UPTIME"));
  EXPECT_FALSE(rider.setRiderFld("bad_field"));
  EXPECT_TRUE(rider.setRiderFld("uptime"));
  EXPECT_FALSE(rider.setPolicyConfig("updated+-1"));
  EXPECT_TRUE(rider.setPolicyConfig("updated+10"));
  EXPECT_TRUE(rider.setPrecision(12));
  EXPECT_EQ(rider.getPrecision(), 10u);
  EXPECT_TRUE(rider.valid());
  EXPECT_EQ(rider.getSpec(), "var=DB_UPTIME, policy=updated+10, rfld=uptime");

  NodeRider frequency;
  EXPECT_TRUE(frequency.setVar("NAV_DEPTH"));
  EXPECT_TRUE(frequency.setPolicyConfig("2.5"));
  EXPECT_EQ(frequency.getPolicy(), "freq");
  EXPECT_DOUBLE_EQ(frequency.getFrequency(), 2.5);
  EXPECT_EQ(frequency.getSpec(), "var=NAV_DEPTH, policy=2.5");
}

TEST(NodeRiderSetTest, ParsesNodeReporterRiderConfigAndReportsAlwaysValues)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=DB_UPTIME,policy=always,rfld=uptime,prec=3"));
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_SUMMARY,policy=always,rfld=summary"));
  EXPECT_FALSE(set.addNodeRider("var=DB_UPTIME,policy=always"));
  EXPECT_EQ(set.size(), 2u);

  std::vector<std::string> vars = set.getVars();
  ASSERT_EQ(vars.size(), 2u);
  EXPECT_EQ(vars[0], "DB_UPTIME");
  EXPECT_EQ(vars[1], "CONTACT_SUMMARY");

  std::vector<std::string> specs = set.getRiderSpecs();
  ASSERT_EQ(specs.size(), 2u);
  EXPECT_EQ(specs[0], "var=DB_UPTIME, policy=always, rfld=uptime");
  EXPECT_EQ(specs[1], "var=CONTACT_SUMMARY, policy=always, rfld=summary");

  EXPECT_TRUE(set.updateRider("DB_UPTIME", "", 12.34567, 100));
  EXPECT_TRUE(set.updateRider("CONTACT_SUMMARY", "abe,ben", 100));
  EXPECT_EQ(set.getRiderReports(100), "uptime=12.346,summary={abe,ben}");
  EXPECT_EQ(set.getRiderReports(101), "uptime=12.346,summary={abe,ben}");
}

TEST(NodeRiderSetTest, ReportsUpdatedPolicyOnlyWhenFresh)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACTS_LIST,policy=updated,rfld=contacts"));

  EXPECT_EQ(set.getRiderReports(100), "");
  EXPECT_TRUE(set.updateRider("CONTACTS_LIST", "abe,ben", 100));
  EXPECT_EQ(set.getRiderReports(100), "contacts={abe,ben}");
  EXPECT_EQ(set.getRiderReports(101), "");

  EXPECT_TRUE(set.updateRider("CONTACTS_LIST", "abe,ben,cal", 102));
  EXPECT_EQ(set.getRiderReports(102), "contacts={abe,ben,cal}");
  EXPECT_EQ(set.getRiderReports(103), "");
}

TEST(NodeRiderSetTest, ReportsUpdatedPlusFrequencyFallbackWithMoosVarField)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=updated+10,rfld=range"));

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.1234, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=42.1234");
  EXPECT_EQ(set.getRiderReports(109.9), "");
  EXPECT_EQ(set.getRiderReports(110), "CONTACT_RANGE=42.1234");
}

TEST(NodeRiderSetTest, ReportsFrequencyPolicyOnlyAfterElapsedInterval)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=NAV_DEPTH,policy=5,rfld=depth,prec=1"));

  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 12.34, 100));
  EXPECT_EQ(set.getRiderReports(104.9), "");
  EXPECT_EQ(set.getRiderReports(105), "depth=12.3");
  EXPECT_EQ(set.getRiderReports(109.9), "");
  EXPECT_EQ(set.getRiderReports(110), "depth=12.3");
}
