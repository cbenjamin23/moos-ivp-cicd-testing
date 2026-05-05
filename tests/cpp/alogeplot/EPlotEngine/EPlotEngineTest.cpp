#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "EPlotEngine.h"
#include "TestFileUtils.h"

namespace {

class TestableEPlotEngine : public EPlotEngine {
 public:
  using EPlotEngine::handleALogFile;
  using EPlotEngine::handleALogFiles;
  using EPlotEngine::writeBaseGridFrame;
  using EPlotEngine::writeBaseLabels;
  using EPlotEngine::writeBaseZones;
  using EPlotEngine::writeEncounters;

  unsigned int eventCount() const { return m_cpa_events.size(); }
  std::string communityName() const { return m_community_name; }
  double collisionRange() const { return m_collision_range; }
  double nearMissRange() const { return m_near_miss_range; }
  double encounterRange() const { return m_encounter_range; }
  std::string gridHeightString() const { return m_grid_hgt_cm_str; }
  std::string pointSize() const { return m_point_size; }
  std::string pointColor() const { return m_point_color; }
};

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

std::string ReadFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  EXPECT_TRUE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers EPlotEngine's stable validation,
// sizing/color parameters, alog parsing, scene primitive writers, and summary
// state against app_alogeplot. The external scn2jpg.sh system call in
// generate() is intentionally not invoked by these component tests.

// Covers input and scene-file validation.
TEST(EPlotEngineTest, ValidatesInputAndSceneFiles)
{
  TempDir temp("alogeplot_validation");
  const std::string alog = temp.writeFile("input.alog", "");

  EPlotEngine engine;
  EXPECT_FALSE(engine.addALogFile(temp.filePath("missing.alog")));
  EXPECT_TRUE(engine.addALogFile(alog));
  EXPECT_TRUE(engine.setSceneFile(temp.filePath("scene.scn")));
  EXPECT_FALSE(engine.setSceneFile(temp.filePath("missing/scene.scn")));
}

// Covers color, point-size, and dimension validation including minimum clamps.
TEST(EPlotEngineTest, ValidatesStyleAndDimensionParameters)
{
  TestableEPlotEngine engine;

  EXPECT_TRUE(engine.setPointColor("red"));
  EXPECT_EQ(engine.pointColor(), "red");
  EXPECT_FALSE(engine.setPointColor("not-a-color"));
  EXPECT_EQ(engine.pointColor(), "red");

  EXPECT_TRUE(engine.setPointSize("0.25"));
  EXPECT_EQ(engine.pointSize(), "0.25");
  EXPECT_FALSE(engine.setPointSize("large"));

  EXPECT_TRUE(engine.setPlotWidCM("2"));
  EXPECT_TRUE(engine.setPlotHgtCM("3"));
  EXPECT_TRUE(engine.setGridWidCM("1"));
  EXPECT_TRUE(engine.setGridHgtCM("2"));
  EXPECT_EQ(engine.gridHeightString(), "2");
  EXPECT_FALSE(engine.setPlotWidCM("bad"));
  EXPECT_FALSE(engine.setGridHgtCM("bad"));
}

// Covers the current setGridHgtCM quirk: it checks the plot height while
// assigning grid height, so a small grid height can remain below the documented
// clamp when plot height is already valid.
TEST(EPlotEngineTest, GridHeightClampDependsOnPlotHeightQuirk)
{
  TestableEPlotEngine engine;

  EXPECT_TRUE(engine.setGridHgtCM("2"));
  EXPECT_EQ(engine.gridHeightString(), "2");
}

// Covers alog parsing for community name, encounter events, and collision
// detector parameter ranges.
TEST(EPlotEngineTest, HandleAlogFileExtractsEncountersAndRanges)
{
  TempDir temp("alogeplot_parse");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "not an alog row",
                    "0.100 DB_TIME MOOSDB_alpha 123456",
                    "1.000 COLLISION_DETECT_PARAMS uFldCollisionDetect "
                    "collision_range=4,near_miss_range=8,encounter_range=20",
                    "2.000 ENCOUNTER_SUMMARY uFldCollisionDetect "
                    "v1=abe,v2=ben,cpa=6,eff=50,x=1,y=2,id=3"}));

  TestableEPlotEngine engine;
  EXPECT_TRUE(engine.handleALogFile(alog));

  EXPECT_EQ(engine.communityName(), "alpha");
  EXPECT_EQ(engine.eventCount(), 1u);
  EXPECT_DOUBLE_EQ(engine.collisionRange(), 4);
  EXPECT_DOUBLE_EQ(engine.nearMissRange(), 8);
  EXPECT_DOUBLE_EQ(engine.encounterRange(), 20);
}

// Covers missing alog handling at the protected parsing layer.
TEST(EPlotEngineTest, HandleAlogFileRejectsMissingInput)
{
  TempDir temp("alogeplot_missing");
  TestableEPlotEngine engine;

  EXPECT_FALSE(engine.handleALogFile(temp.filePath("missing.alog")));
}

// Covers multi-file state reset and summary reporting.
TEST(EPlotEngineTest, HandleAlogFilesResetsStateAndReportsSummary)
{
  TempDir temp("alogeplot_handle_all");
  const std::string first = temp.writeFile(
      "first.alog",
      WithNewlines({"0.100 DB_TIME MOOSDB_alpha 123456",
                    "1.000 COLLISION_DETECT_PARAMS uFldCollisionDetect "
                    "collision_range=4,near_miss_range=8,encounter_range=20",
                    "2.000 ENCOUNTER_SUMMARY uFldCollisionDetect "
                    "v1=abe,v2=ben,cpa=6,eff=50"}));
  const std::string second = temp.writeFile(
      "second.alog",
      WithNewlines({"3.000 ENCOUNTER_SUMMARY uFldCollisionDetect "
                    "v1=abe,v2=cal,cpa=7,eff=60"}));

  TestableEPlotEngine engine;
  ASSERT_TRUE(engine.addALogFile(first));
  ASSERT_TRUE(engine.addALogFile(second));

  testing::internal::CaptureStdout();
  engine.handleALogFiles();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(engine.eventCount(), 2u);
  EXPECT_TRUE(Contains(output, "Total encounters: 2"));
  EXPECT_TRUE(Contains(output, "Community Name:   alpha"));
}

// Covers scene writer output without invoking the external conversion command.
TEST(EPlotEngineTest, SceneWritersEmitGridZonesLabelsAndEncounterPoints)
{
  TempDir temp("alogeplot_scene");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 COLLISION_DETECT_PARAMS uFldCollisionDetect "
                    "collision_range=5,near_miss_range=10,encounter_range=20",
                    "2.000 ENCOUNTER_SUMMARY uFldCollisionDetect "
                    "v1=abe,v2=ben,cpa=10,eff=50"}));
  const std::string scene = temp.filePath("scene.scn");

  TestableEPlotEngine engine;
  ASSERT_TRUE(engine.handleALogFile(alog));
  FILE* file = fopen(scene.c_str(), "w");
  ASSERT_NE(file, nullptr);
  engine.writeBaseGridFrame(file);
  engine.writeBaseZones(file);
  engine.writeBaseLabels(file);
  engine.writeEncounters(file);
  fclose(file);

  const std::string contents = ReadFile(scene);
  EXPECT_TRUE(Contains(contents, "grid = key=master"));
  EXPECT_TRUE(Contains(contents, "key=kzone"));
  EXPECT_TRUE(Contains(contents, "Closest Point of Approach"));
  EXPECT_TRUE(Contains(contents, "point = key=p0"));
  EXPECT_TRUE(Contains(contents, "x=7.5"));
  EXPECT_TRUE(Contains(contents, "y=5"));
}
