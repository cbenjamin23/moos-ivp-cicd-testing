#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "LegRun.h"
#include "LegRunSet.h"
#include "NumericAssertions.h"
#include "TestFileUtils.h"
#include "XYPoint.h"
#include "XYSegList.h"

// Covers leg run behavior: parses point and turn parameters used by survey leg run patterns.
TEST(LegRunTest, ParsesPointAndTurnParametersUsedBySurveyLegRunPatterns)
{
  LegRun run;

  ASSERT_TRUE(run.setParams(
      "id=lane1,p1=0:0,p2=100:0,turn_rad=25,turn_ext=10,"
      "turn_bias=20,turn_dir=star"));
  ASSERT_TRUE(run.setLaneIX(3));

  EXPECT_TRUE(run.valid());
  EXPECT_EQ(run.getLegID(), "lane1");
  EXPECT_NEAR(run.getLegLen(), 100.0, kGeomTol);
  EXPECT_NEAR(run.getLegAng(), 90.0, kGeomTol);
  EXPECT_NEAR(run.getTurn1Rad(), 25.0, kGeomTol);
  EXPECT_NEAR(run.getTurn2Rad(), 25.0, kGeomTol);
  EXPECT_NEAR(run.getTurn1Ext(), 10.0, kGeomTol);
  EXPECT_NEAR(run.getTurn2Ext(), 10.0, kGeomTol);
  EXPECT_NEAR(run.getTurn1Bias(), 20.0, kGeomTol);
  EXPECT_NEAR(run.getTurn2Bias(), 20.0, kGeomTol);
  EXPECT_EQ(run.getTurn1Dir(), "star");
  EXPECT_EQ(run.getTurn2Dir(), "star");
  EXPECT_TRUE(run.isSetLaneIX());
  EXPECT_EQ(run.getLaneIX(), 3);
}

// Covers leg run behavior: builds leg from center angle and length and supports geometry mutation.
TEST(LegRunTest, BuildsLegFromCenterAngleAndLengthAndSupportsGeometryMutation)
{
  LegRun run;
  ASSERT_TRUE(run.setParams("leg=50:10:90:100,turn_rad=20"));

  EXPECT_TRUE(run.valid());
  EXPECT_NEAR(run.getCenterPt().x(), 50.0, kGeomTol);
  EXPECT_NEAR(run.getCenterPt().y(), 10.0, kGeomTol);
  EXPECT_NEAR(run.getPoint1().x(), 0.0, kGeomTol);
  EXPECT_NEAR(run.getPoint1().y(), 10.0, kGeomTol);
  EXPECT_NEAR(run.getPoint2().x(), 100.0, kGeomTol);
  EXPECT_NEAR(run.getPoint2().y(), 10.0, kGeomTol);

  ASSERT_TRUE(run.modCenterPt("x=60,y=20"));
  EXPECT_NEAR(run.getCenterPt().x(), 60.0, kGeomTol);
  EXPECT_NEAR(run.getCenterPt().y(), 20.0, kGeomTol);

  ASSERT_TRUE(run.setLegLen(50));
  EXPECT_NEAR(run.getLegLen(), 50.0, kGeomTol);
  ASSERT_TRUE(run.modLegAng(90));
  EXPECT_NEAR(run.getLegAng(), 180.0, kGeomTol);
}

// Covers leg run behavior: accepts keyword center format and deprecated vertex aliases.
TEST(LegRunTest, AcceptsKeywordCenterFormatAndDeprecatedVertexAliases)
{
  LegRun keyword_run;
  ASSERT_TRUE(keyword_run.setParam("leg", "x=50,y=10,ang=90,len=100"));
  ASSERT_TRUE(keyword_run.setTurnRad(20));
  EXPECT_TRUE(keyword_run.valid());
  EXPECT_NEAR(keyword_run.getPoint1().x(), 0.0, kGeomTol);
  EXPECT_NEAR(keyword_run.getPoint1().y(), 10.0, kGeomTol);
  EXPECT_NEAR(keyword_run.getPoint2().x(), 100.0, kGeomTol);
  EXPECT_NEAR(keyword_run.getPoint2().y(), 10.0, kGeomTol);

  EXPECT_FALSE(keyword_run.setParam("leg", "x=50,y=10,bad=90,len=100"));
  EXPECT_FALSE(keyword_run.setParam("leg", "x=50,y=10,ang=90"));

  LegRun alias_run;
  ASSERT_TRUE(alias_run.setParams("vx1=0:0,vx2=100:0,turn_rad=25"));
  EXPECT_TRUE(alias_run.valid());
  EXPECT_NEAR(alias_run.getLegLen(), 100.0, kGeomTol);
}

// Covers leg run behavior: clips turn radius and bias and rejects invalid inputs.
TEST(LegRunTest, ClipsTurnRadiusAndBiasAndRejectsInvalidInputs)
{
  LegRun run;
  ASSERT_TRUE(run.setParams("p1=0:0,p2=100:0,turn_rad=20"));

  EXPECT_FALSE(run.setTurnRad(0));
  EXPECT_TRUE(run.setTurnRadMin(10));
  EXPECT_TRUE(run.setTurnRad(4));
  EXPECT_DOUBLE_EQ(run.getTurn1Rad(), 10);
  EXPECT_DOUBLE_EQ(run.getTurn2Rad(), 10);

  EXPECT_TRUE(run.setTurnBias(125));
  EXPECT_DOUBLE_EQ(run.getTurn1Bias(), 100);
  EXPECT_DOUBLE_EQ(run.getTurn2Bias(), 100);
  EXPECT_TRUE(run.setTurnBias(-125));
  EXPECT_DOUBLE_EQ(run.getTurn1Bias(), -100);
  EXPECT_DOUBLE_EQ(run.getTurn2Bias(), -100);

  EXPECT_FALSE(run.setTurn1Ext(-1));
  EXPECT_FALSE(run.setTurn1Dir("up"));
  EXPECT_FALSE(run.setPoint1("not_a_point"));
}

// Covers leg run behavior: applies incremental modifiers and invalidates cached turn geometry.
TEST(LegRunTest, AppliesIncrementalModifiersAndInvalidatesCachedTurnGeometry)
{
  LegRun run;
  ASSERT_TRUE(run.setParams("id=lane1,p1=0:0,p2=100:0,turn_rad=20,turn_ext=5"));
  const double initial_turn_len = run.getTurn1Len();
  ASSERT_GT(initial_turn_len, 0.0);

  EXPECT_TRUE(run.modTurnRad(10));
  EXPECT_DOUBLE_EQ(run.getTurn1Rad(), 30);
  EXPECT_DOUBLE_EQ(run.getTurn2Rad(), 30);
  EXPECT_NE(run.getTurn1Len(), initial_turn_len);

  EXPECT_TRUE(run.modTurnExt(5));
  EXPECT_DOUBLE_EQ(run.getTurn1Ext(), 10);
  EXPECT_DOUBLE_EQ(run.getTurn2Ext(), 10);
  EXPECT_FALSE(run.modTurnExt(-20));
  EXPECT_DOUBLE_EQ(run.getTurn1Ext(), 10);

  EXPECT_TRUE(run.modTurnBias(15));
  EXPECT_DOUBLE_EQ(run.getTurn1Bias(), 15);
  EXPECT_DOUBLE_EQ(run.getTurn2Bias(), 15);
  EXPECT_TRUE(run.setTurnPtGap(0.1));
  EXPECT_TRUE(run.modTurnPtGap(-10));
  EXPECT_TRUE(run.turnModified());
}

// Covers leg run behavior: generates turn seglists and distance points around closed leg run.
TEST(LegRunTest, GeneratesTurnSeglistsAndDistancePointsAroundClosedLegRun)
{
  LegRun run;
  ASSERT_TRUE(run.setParams("p1=0:0,p2=100:0,turn_rad=20,turn_pt_gap=5"));

  XYSegList turn1 = run.initTurnPoints1();
  XYSegList turn2 = run.initTurnPoints2();
  EXPECT_GT(turn1.size(), 1u);
  EXPECT_GT(turn2.size(), 1u);
  EXPECT_GT(run.getTurn1Len(), 0.0);
  EXPECT_GT(run.getTurn2Len(), 0.0);
  EXPECT_GT(run.getTotalLen(), 200.0);

  XYPoint before_start = run.getDistPt(-10);
  EXPECT_NEAR(before_start.x(), 0.0, kGeomTol);
  EXPECT_NEAR(before_start.y(), 0.0, kGeomTol);

  XYPoint at_start = run.getDistPt(0);
  EXPECT_NEAR(at_start.x(), 0.0, kGeomTol);
  EXPECT_NEAR(at_start.y(), 0.0, kGeomTol);

  XYPoint halfway = run.getDistPt(50);
  EXPECT_NEAR(halfway.x(), 50.0, kLooseGeomTol);
  EXPECT_NEAR(halfway.y(), 0.0, kLooseGeomTol);

  XYPoint at_end_of_first_leg = run.getDistPt(100);
  EXPECT_NEAR(at_end_of_first_leg.x(), 100.0, kGeomTol);
  EXPECT_NEAR(at_end_of_first_leg.y(), 0.0, kGeomTol);

  XYPoint beyond = run.getDistPt(run.getTotalLen() + 10);
  EXPECT_TRUE(beyond.valid());
}

// Covers leg run behavior: serializes configured leg run for mission file reuse.
TEST(LegRunTest, SerializesConfiguredLegRunForMissionFileReuse)
{
  LegRun run;
  ASSERT_TRUE(run.setParams(
      "id=lane1,p1=0:0,p2=100:0,turn1_rad=20,turn2_rad=30,"
      "turn1_ext=5,turn2_ext=7,turn1_bias=10,turn2_bias=-10,"
      "turn1_dir=port,turn2_dir=star"));
  ASSERT_TRUE(run.setLaneIX(4));

  std::string spec = run.getSpec(',');
  EXPECT_NE(spec.find("p1=0:0"), std::string::npos);
  EXPECT_NE(spec.find("p2=100:0"), std::string::npos);
  EXPECT_NE(spec.find("turn1_rad=20"), std::string::npos);
  EXPECT_NE(spec.find("turn2_rad=30"), std::string::npos);
  EXPECT_NE(spec.find("turn1_dir=port"), std::string::npos);
  EXPECT_NE(spec.find("turn2_dir=star"), std::string::npos);
  EXPECT_EQ(spec.find("id=lane1"), std::string::npos);
  EXPECT_EQ(spec.find("lane_ix=4"), std::string::npos);
}

// Covers leg run behavior: string to leg run returns invalid object when any token fails.
TEST(LegRunTest, StringToLegRunReturnsInvalidObjectWhenAnyTokenFails)
{
  LegRun good = stringToLegRun("p1=0:0,p2=100:0,turn_rad=25,turn_dir=port");
  EXPECT_TRUE(good.valid());

  LegRun bad = stringToLegRun("p1=0:0,p2=100:0,turn_rad=abc");
  EXPECT_FALSE(bad.valid());
}

// Covers leg run set behavior: stores and updates leg runs by id.
TEST(LegRunSetTest, StoresAndUpdatesLegRunsById)
{
  LegRunSet set;
  ASSERT_TRUE(set.setLegParams("id=alpha,p1=0:0,p2=100:0,turn_rad=20"));
  ASSERT_TRUE(set.setLegParams("id=bravo,p1=0:50,p2=100:50,turn_rad=30"));
  ASSERT_TRUE(set.setLegParams("id=alpha,turn1_ext=12"));

  EXPECT_EQ(set.size(), 2u);
  std::vector<std::string> ids = set.getLegIDs();
  ASSERT_EQ(ids.size(), 2u);
  EXPECT_EQ(ids[0], "alpha");
  EXPECT_EQ(ids[1], "bravo");

  LegRun alpha = set.getLegRun("alpha");
  EXPECT_TRUE(alpha.valid());
  EXPECT_DOUBLE_EQ(alpha.getTurn1Ext(), 12);
  EXPECT_DOUBLE_EQ(alpha.getTurn2Ext(), 0);

  EXPECT_FALSE(set.getLegRun("missing").valid());
  EXPECT_FALSE(set.setLegParams("p1=0:0,p2=1:1,turn_rad=10"));
}

// Covers leg run set behavior: pins current v name fallback and inner parse failure behavior.
TEST(LegRunSetTest, PinsCurrentVNameFallbackAndInnerParseFailureBehavior)
{
  LegRunSet set;
  EXPECT_FALSE(set.setLegParams("vname=alpha,p1=0:0,p2=100:0,turn_rad=20"));
  EXPECT_EQ(set.size(), 0u);

  ASSERT_TRUE(set.setLegParams("id=alpha,p1=0:0,p2=100:0,turn_rad=20"));
  EXPECT_TRUE(set.setLegParams("id=alpha,turn_rad=not_a_number"));
  EXPECT_EQ(set.size(), 1u);
  EXPECT_DOUBLE_EQ(set.getLegRun("alpha").getTurn1Rad(), 20);
  EXPECT_TRUE(set.getLegRun("alpha").valid());
}

// Covers leg run set behavior: reads leg run files with comments and reports bad lines.
TEST(LegRunSetTest, ReadsLegRunFilesWithCommentsAndReportsBadLines)
{
  TempFile file("legruns",
                "# comment\n"
                "id=alpha,p1=0:0,p2=100:0,turn_rad=20\n"
                "// another comment\n"
                "id=bravo,leg=50:50:90:80,turn_rad=25\n");

  LegRunSet set;
  std::string warning;
  EXPECT_TRUE(set.handleLegRunFile(file.path(), 0, warning));
  EXPECT_EQ(warning, "");
  EXPECT_EQ(set.getLegRunFile(), file.path());
  EXPECT_EQ(set.size(), 2u);

  TempFile bad_file("bad_legruns",
                    "id=alpha,p1=0:0,p2=100:0,turn_rad=20\n"
                    "p1=0:0,p2=10:0,turn_rad=20\n");

  LegRunSet bad_set;
  EXPECT_FALSE(bad_set.handleLegRunFile(bad_file.path(), 0, warning));
  EXPECT_EQ(warning, "Bad LegRun Line:[p1=0:0,p2=10:0,turn_rad=20]");
}

// Covers leg run set behavior: loading additional files updates existing ids without clearing others.
TEST(LegRunSetTest, LoadingAdditionalFilesUpdatesExistingIdsWithoutClearingOthers)
{
  TempFile first_file("legruns_first",
                      "id=alpha,p1=0:0,p2=100:0,turn_rad=20\n"
                      "id=bravo,p1=0:50,p2=100:50,turn_rad=30\n");
  TempFile second_file("legruns_second", "id=alpha,turn1_ext=8\n");

  LegRunSet set;
  std::string warning;
  ASSERT_TRUE(set.handleLegRunFile(first_file.path(), 0, warning));
  ASSERT_TRUE(set.handleLegRunFile(second_file.path(), 100, warning));

  EXPECT_EQ(set.getLegRunFile(), second_file.path());
  EXPECT_EQ(set.size(), 2u);
  EXPECT_DOUBLE_EQ(set.getLegRun("alpha").getTurn1Ext(), 8);
  EXPECT_TRUE(set.getLegRun("bravo").valid());
}
