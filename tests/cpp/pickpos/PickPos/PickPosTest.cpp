#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "PickPos.h"
#include "TestFileUtils.h"

namespace {

class TestablePickPos : public PickPos {
 public:
  using PickPos::pickColors;
  using PickPos::pickFileLines;
  using PickPos::pickGroupNames;
  using PickPos::pickHeadingVals;
  using PickPos::pickIndices;
  using PickPos::pickPosByFile;
  using PickPos::pickPosByCircle;
  using PickPos::pickSpeedVals;
  using PickPos::pickVehicleNames;

  unsigned int pickAmt() const { return m_pick_amt; }
  unsigned int maxTries() const { return m_max_tries; }
  double bufferDist() const { return m_buffer_dist; }
  double headingSnap() const { return m_hdg_snap; }
  double speedSnap() const { return m_spd_snap; }
  double pointSnap() const { return m_pt_snap; }
  const std::string& outputType() const { return m_output_type; }
  const std::string& headingType() const { return m_hdg_type; }
  const std::string& speedType() const { return m_spd_type; }
  const std::string& groupType() const { return m_grp_type; }
  double headingVal1() const { return m_hdg_val1; }
  double headingVal2() const { return m_hdg_val2; }
  double headingVal3() const { return m_hdg_val3; }
  double speedVal1() const { return m_spd_val1; }
  double speedVal2() const { return m_spd_val2; }
  double circleX() const { return m_circ_x; }
  double circleY() const { return m_circ_y; }
  double circleRad() const { return m_circ_rad; }
  bool circleSet() const { return m_circ_set; }
  const std::vector<std::string>& filePositions() const
  {
    return m_file_positions;
  }
  const std::vector<std::string>& fileLines() const { return m_file_lines; }
  const std::vector<std::string>& pickPositions() const
  {
    return m_pick_positions;
  }
  const std::vector<std::string>& pickLines() const { return m_pick_lines; }
  const std::vector<std::string>& pickGroups() const { return m_pick_groups; }
  const std::vector<int>& pickIndicesOut() const { return m_pick_indices; }
  const std::vector<std::string>& pickVNames() const { return m_pick_vnames; }
  const std::vector<std::string>& pickColorsOut() const { return m_pick_colors; }
  const std::vector<double>& pickHeadings() const { return m_pick_headings; }
  const std::vector<double>& pickSpeeds() const { return m_pick_speeds; }
};

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

double FieldValue(const std::string& line, const std::string& field)
{
  const std::string marker = field + "=";
  const std::string::size_type start = line.find(marker);
  if(start == std::string::npos)
    return 0;
  const std::string::size_type value_start = start + marker.size();
  const std::string::size_type value_end = line.find(',', value_start);
  return std::stod(line.substr(value_start, value_end - value_start));
}

}  // namespace

// Source audit note: this suite covers PickPos's deterministic validation,
// file loaders, name/color/group configuration, heading/speed modes, circle
// parsing, and non-fatal pick paths against app_pickpos. Branches that call
// exit() for CLI misuse are intentionally validated through their preconditions
// instead of being invoked directly from component tests.

// Covers scalar validation and accepted boundary values.
TEST(PickPosTest, ValidatesScalarConfiguration)
{
  TestablePickPos picker;

  EXPECT_TRUE(picker.setPickAmt("1"));
  EXPECT_EQ(picker.pickAmt(), 1u);
  EXPECT_FALSE(picker.setPickAmt("0"));
  EXPECT_FALSE(picker.setPickAmt("-1"));
  EXPECT_FALSE(picker.setPickAmt("bad"));

  EXPECT_TRUE(picker.setMaxTries("1"));
  EXPECT_EQ(picker.maxTries(), 1u);
  EXPECT_FALSE(picker.setMaxTries("0"));
  EXPECT_FALSE(picker.setMaxTries("bad"));
}

// Covers nonnegative double setters and output type validation.
TEST(PickPosTest, ValidatesSnapsBufferAndOutputType)
{
  TestablePickPos picker;

  EXPECT_TRUE(picker.setBufferDist("0"));
  EXPECT_DOUBLE_EQ(picker.bufferDist(), 0);
  EXPECT_TRUE(picker.setHeadingSnap("2.5"));
  EXPECT_DOUBLE_EQ(picker.headingSnap(), 2.5);
  EXPECT_TRUE(picker.setSpeedSnap("0"));
  EXPECT_DOUBLE_EQ(picker.speedSnap(), 0);
  EXPECT_TRUE(picker.setPointSnap("0.25"));
  EXPECT_DOUBLE_EQ(picker.pointSnap(), 0.25);

  EXPECT_TRUE(picker.setOutputType("terse"));
  EXPECT_EQ(picker.outputType(), "terse");
  EXPECT_TRUE(picker.setOutputType("full"));
  EXPECT_FALSE(picker.setOutputType("compact"));
}

// Covers heading parser forms and rejection of invalid ranges.
TEST(PickPosTest, ParsesHeadingConfiguration)
{
  TestablePickPos picker;

  EXPECT_TRUE(picker.setHdgConfig("90"));
  EXPECT_EQ(picker.headingType(), "rand");
  EXPECT_DOUBLE_EQ(picker.headingVal1(), 90);
  EXPECT_DOUBLE_EQ(picker.headingVal2(), 90);

  EXPECT_TRUE(picker.setHdgConfig("-45:45"));
  EXPECT_DOUBLE_EQ(picker.headingVal1(), -45);
  EXPECT_DOUBLE_EQ(picker.headingVal2(), 45);

  EXPECT_TRUE(picker.setHdgConfig("10,20,30"));
  EXPECT_EQ(picker.headingType(), "rbng");
  EXPECT_DOUBLE_EQ(picker.headingVal3(), 30);

  EXPECT_FALSE(picker.setHdgConfig("361"));
  EXPECT_FALSE(picker.setHdgConfig("45:-45"));
  EXPECT_FALSE(picker.setHdgConfig("10,20,400"));

  TestablePickPos unknown;
  EXPECT_TRUE(unknown.setHdgConfig("north"));
  EXPECT_EQ(unknown.headingType(), "none");
}

// Covers speed parser forms, invalid ranges, and the accepted no-op unknown
// string path.
TEST(PickPosTest, ParsesSpeedConfiguration)
{
  TestablePickPos picker;

  EXPECT_TRUE(picker.setSpdConfig("2.5"));
  EXPECT_EQ(picker.speedType(), "rand");
  EXPECT_DOUBLE_EQ(picker.speedVal1(), 2.5);
  EXPECT_DOUBLE_EQ(picker.speedVal2(), 2.5);

  EXPECT_TRUE(picker.setSpdConfig("1:3"));
  EXPECT_DOUBLE_EQ(picker.speedVal1(), 1);
  EXPECT_DOUBLE_EQ(picker.speedVal2(), 3);
  EXPECT_FALSE(picker.setSpdConfig("-1"));
  EXPECT_FALSE(picker.setSpdConfig("4:2"));
  EXPECT_FALSE(picker.setSpdConfig("1:bad"));

  TestablePickPos unknown;
  EXPECT_TRUE(unknown.setSpdConfig("cruise"));
  EXPECT_EQ(unknown.speedType(), "none");
}

// Covers vehicle-name, color, and alternating group configuration validation.
TEST(PickPosTest, ValidatesNamesColorsAndAlternatingGroups)
{
  TestablePickPos picker;

  EXPECT_TRUE(picker.setVNames("abe,ben,cal"));
  EXPECT_FALSE(picker.setVNames("bad name"));
  EXPECT_FALSE(picker.setVNames("bad$name"));
  EXPECT_TRUE(picker.setColors("red,blue"));
  EXPECT_FALSE(picker.setColors("not_a_color"));

  ASSERT_TRUE(picker.setPickAmt("3"));
  ASSERT_TRUE(picker.setGroups("red,blue:alt"));
  EXPECT_EQ(picker.groupType(), "alternating");
  picker.pickGroupNames();
  ASSERT_EQ(picker.pickGroups().size(), 3u);
  EXPECT_EQ(picker.pickGroups()[0], "red");
  EXPECT_EQ(picker.pickGroups()[1], "blue");
  EXPECT_EQ(picker.pickGroups()[2], "red");
}

// Covers vehicle-name start offset and explicit index picking.
TEST(PickPosTest, PicksNamesAndIndicesFromConfiguredStartOffset)
{
  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("2"));
  ASSERT_TRUE(picker.setVNameStartIX("1"));
  ASSERT_TRUE(picker.setVNames("abe,ben,cal"));

  picker.pickVehicleNames();
  ASSERT_EQ(picker.pickVNames().size(), 2u);
  EXPECT_EQ(picker.pickVNames()[0], "ben");
  EXPECT_EQ(picker.pickVNames()[1], "cal");

  picker.pickIndices();
  ASSERT_EQ(picker.pickIndicesOut().size(), 2u);
  EXPECT_EQ(picker.pickIndicesOut()[0], 1);
  EXPECT_EQ(picker.pickIndicesOut()[1], 2);

  EXPECT_FALSE(picker.setVNameStartIX("-1"));
  EXPECT_FALSE(picker.setVNameStartIX("bad"));
}

// Covers default-name cache variants, wraparound, and reverse-name indexing.
TEST(PickPosTest, PicksVehicleNamesFromAllBuiltInCaches)
{
  TestablePickPos one;
  ASSERT_TRUE(one.setPickAmt("2"));
  one.pickVehicleNames();
  ASSERT_EQ(one.pickVNames().size(), 2u);
  EXPECT_EQ(one.pickVNames()[0], "abe");
  EXPECT_EQ(one.pickVNames()[1], "ben");

  TestablePickPos four;
  ASSERT_TRUE(four.setVNameCacheFour());
  ASSERT_TRUE(four.setPickAmt("2"));
  ASSERT_TRUE(four.setVNameStartIX("51"));
  four.pickVehicleNames();
  ASSERT_EQ(four.pickVNames().size(), 2u);
  EXPECT_EQ(four.pickVNames()[0], "zeke");
  EXPECT_EQ(four.pickVNames()[1], "art");

  TestablePickPos reversed;
  ASSERT_TRUE(reversed.setPickAmt("2"));
  ASSERT_TRUE(reversed.setReverseNames());
  reversed.pickVehicleNames();
  ASSERT_EQ(reversed.pickVNames().size(), 2u);
  EXPECT_EQ(reversed.pickVNames()[0], "zahl");
  EXPECT_EQ(reversed.pickVNames()[1], "york");
}

// Covers deterministic pick helpers for fixed heading/speed/color values.
TEST(PickPosTest, PicksFixedHeadingSpeedAndConfiguredColors)
{
  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("2"));
  ASSERT_TRUE(picker.setHdgConfig("45"));
  ASSERT_TRUE(picker.setSpdConfig("2.5"));
  ASSERT_TRUE(picker.setColors("red,blue"));

  picker.pickHeadingVals();
  picker.pickSpeedVals();
  picker.pickColors();

  ASSERT_EQ(picker.pickHeadings().size(), 2u);
  EXPECT_DOUBLE_EQ(picker.pickHeadings()[0], 45);
  EXPECT_DOUBLE_EQ(picker.pickHeadings()[1], 45);
  ASSERT_EQ(picker.pickSpeeds().size(), 2u);
  EXPECT_DOUBLE_EQ(picker.pickSpeeds()[0], 2.5);
  EXPECT_DOUBLE_EQ(picker.pickSpeeds()[1], 2.5);
  ASSERT_EQ(picker.pickColorsOut().size(), 2u);
  EXPECT_EQ(picker.pickColorsOut()[0], "red");
  EXPECT_EQ(picker.pickColorsOut()[1], "blue");
}

// Covers random heading and speed ranges without relying on a specific random
// draw.
TEST(PickPosTest, PicksRandomHeadingAndSpeedValuesWithinConfiguredRanges)
{
  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("20"));
  ASSERT_TRUE(picker.setHdgConfig("-10:10"));
  ASSERT_TRUE(picker.setSpdConfig("1.25:1.75"));

  picker.pickHeadingVals();
  picker.pickSpeedVals();

  ASSERT_EQ(picker.pickHeadings().size(), 20u);
  ASSERT_EQ(picker.pickSpeeds().size(), 20u);
  for(double heading : picker.pickHeadings()) {
    EXPECT_GE(heading, -10);
    EXPECT_LT(heading, 10);
  }
  for(double speed : picker.pickSpeeds()) {
    EXPECT_GE(speed, 1.25);
    EXPECT_LT(speed, 1.75);
  }
}

// Covers relative-bearing heading generation from already-picked positions.
TEST(PickPosTest, PicksRelativeBearingHeadingsFromPositionFile)
{
  TempDir temp("pickpos_rbng");
  const std::string pos_file = temp.writeFile("positions.txt", "x=10,y=0\n");

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("1"));
  ASSERT_TRUE(picker.addPosFile(pos_file));
  picker.pickPosByFile();
  ASSERT_TRUE(picker.setHdgConfig("0,0,0"));

  picker.pickHeadingVals();

  ASSERT_EQ(picker.pickHeadings().size(), 1u);
  EXPECT_DOUBLE_EQ(picker.pickHeadings()[0], -90);
}

// Covers position and line file loaders while avoiding the fatal over-pick
// branches in pickPosByFile() and pickFileLines().
TEST(PickPosTest, LoadsOnlyValidPositionAndLineFileRows)
{
  TempDir temp("pickpos_loaders");
  const std::string pos_file =
      temp.writeFile("positions.txt",
                     WithNewlines({"x=1,y=2",
                                   "x=bad,y=2",
                                   "x=3,y=4,hdg=90",
                                   "x=5,y=6,unknown=7",
                                   "x=7,y=8,hdg=bad"}));
  const std::string line_file =
      temp.writeFile("lines.txt", WithNewlines({" apple ", "", "// pear", "grape"}));

  TestablePickPos picker;
  ASSERT_TRUE(picker.addPosFile(pos_file));
  ASSERT_TRUE(picker.addLineFile(line_file));

  ASSERT_EQ(picker.filePositions().size(), 2u);
  EXPECT_EQ(picker.filePositions()[0], "x=1,y=2");
  EXPECT_EQ(picker.filePositions()[1], "x=3,y=4,hdg=90");
  ASSERT_EQ(picker.fileLines().size(), 2u);
  EXPECT_EQ(picker.fileLines()[0], "apple");
  EXPECT_EQ(picker.fileLines()[1], "grape");
}

// Covers non-fatal file picking with pick_amt inside the available choices.
TEST(PickPosTest, PicksFromPositionFileWithinAvailableChoices)
{
  TempDir temp("pickpos_file_pick");
  const std::string pos_file =
      temp.writeFile("positions.txt", WithNewlines({"x=1,y=2", "x=3,y=4"}));

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("2"));
  ASSERT_TRUE(picker.addPosFile(pos_file));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(picker.pick());
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(picker.pickPositions().size(), 2u);
}

// Covers line-file picking and comment/blank filtering.
TEST(PickPosTest, PicksFromLineFileWithinAvailableChoices)
{
  TempDir temp("pickpos_line_pick");
  const std::string line_file =
      temp.writeFile("lines.txt", WithNewlines({"alpha", "", "// beta", "gamma"}));

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("2"));
  ASSERT_TRUE(picker.addLineFile(line_file));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(picker.pick());
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(picker.pickLines().size(), 2u);
}

// Covers fatal over-pick validation for position-file and line-file sources.
TEST(PickPosDeathTest, ExitsWhenPickAmountExceedsFileChoices)
{
  TempDir temp("pickpos_file_death");
  const std::string pos_file = temp.writeFile("positions.txt", "x=1,y=2\n");

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("2"));
  ASSERT_TRUE(picker.addPosFile(pos_file));

  EXPECT_EXIT(picker.pickPosByFile(), ::testing::ExitedWithCode(1), ".*");
}

TEST(PickPosDeathTest, ExitsWhenPickAmountExceedsLineChoices)
{
  TempDir temp("pickpos_line_death");
  const std::string line_file = temp.writeFile("lines.txt", "alpha\n");

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("2"));
  ASSERT_TRUE(picker.addLineFile(line_file));

  EXPECT_EXIT(picker.pickFileLines(), ::testing::ExitedWithCode(1), ".*");
}

// Covers a small polygon-backed pick without invoking fatal failure paths.
TEST(PickPosTest, PicksFromConfiguredPolygon)
{
  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("1"));
  ASSERT_TRUE(picker.addPolygon("pts={0,0:100,0:100,100:0,100}"));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(picker.pick());
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(picker.pickPositions().size(), 1u);
}

// Covers circle validation and current upstream behavior: pickPosByCircle()
// shadows the configured circle members with hard-coded 10,20,50 constants.
TEST(PickPosTest, CirclePickerUsesHardCodedGenerationCircle)
{
  TestablePickPos picker;

  EXPECT_FALSE(picker.setCircle("x=1,y=2"));
  EXPECT_FALSE(picker.setCircle("x=1,y=bad,rad=3"));
  EXPECT_TRUE(picker.setCircle("x=0,y=0,rad=-5"));
  EXPECT_DOUBLE_EQ(picker.circleRad(), -5);
  ASSERT_TRUE(picker.setCircle("x=100,y=200,rad=7"));
  EXPECT_TRUE(picker.circleSet());
  EXPECT_DOUBLE_EQ(picker.circleX(), 100);
  EXPECT_DOUBLE_EQ(picker.circleY(), 200);
  EXPECT_DOUBLE_EQ(picker.circleRad(), 7);

  ASSERT_TRUE(picker.setPickAmt("4"));
  EXPECT_TRUE(picker.pickPosByCircle());
  ASSERT_EQ(picker.pickPositions().size(), 4u);
  for(const std::string& point : picker.pickPositions()) {
    const double dx = FieldValue(point, "x") - 10;
    const double dy = FieldValue(point, "y") - 20;
    EXPECT_NEAR(std::sqrt((dx * dx) + (dy * dy)), 50, 0.02);
  }
}

// Covers the unimplemented positive-min-separation circle path.
TEST(PickPosTest, CirclePickerPositiveMinSepAddsNoPositions)
{
  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("3"));

  EXPECT_TRUE(picker.pickPosByCircle(10));
  EXPECT_TRUE(picker.pickPositions().empty());
}

// Covers result-file reuse and the early return in printChoices().
TEST(PickPosTest, ReusesExistingResultFileWithoutPrintingNewChoices)
{
  TempDir temp("pickpos_reuse");
  const std::string result_file = temp.writeFile("result.txt", "stale\n");

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("1"));
  ASSERT_TRUE(picker.setIndices());
  ASSERT_TRUE(picker.setResultFile(result_file));
  ASSERT_TRUE(picker.setReuse());

  testing::internal::CaptureStdout();
  EXPECT_TRUE(picker.pick());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
  std::fflush(nullptr);

  std::ifstream input(result_file);
  std::string contents((std::istreambuf_iterator<char>(input)),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(contents, "stale\n");
}

// Covers composite output formatting to a result file, including terse removal
// of field labels.
TEST(PickPosTest, WritesTerseCompositeChoicesToResultFile)
{
  TempDir temp("pickpos_result_file");
  const std::string result_file = temp.filePath("result.txt");

  TestablePickPos picker;
  ASSERT_TRUE(picker.setPickAmt("1"));
  ASSERT_TRUE(picker.setCircle("x=1,y=2,rad=3"));
  ASSERT_TRUE(picker.setHdgConfig("45"));
  ASSERT_TRUE(picker.setSpdConfig("2"));
  ASSERT_TRUE(picker.setVNames("abe"));
  ASSERT_TRUE(picker.setColors("red"));
  ASSERT_TRUE(picker.setGroups("alpha"));
  ASSERT_TRUE(picker.setOutputType("terse"));
  ASSERT_TRUE(picker.setResultFile(result_file));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(picker.pick());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");

  std::ifstream input(result_file);
  std::string line;
  ASSERT_TRUE(std::getline(input, line));
  EXPECT_EQ(line.find("x="), std::string::npos);
  EXPECT_EQ(line.find("heading="), std::string::npos);
  EXPECT_NE(line.find("45"), std::string::npos);
  EXPECT_NE(line.find("abe"), std::string::npos);
  EXPECT_NE(line.find("red"), std::string::npos);
  EXPECT_NE(line.find("alpha"), std::string::npos);
}
