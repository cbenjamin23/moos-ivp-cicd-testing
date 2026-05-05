#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "FldProjector.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

std::vector<std::string> Lines(const std::string& text)
{
  std::vector<std::string> lines;
  std::stringstream stream(text);
  std::string line;
  while(std::getline(stream, line))
    lines.push_back(line);
  return lines;
}

}  // namespace

// Source audit note: this suite covers FldProjector's deterministic projection
// geometry, setter propagation into printed headers, label generation, and
// empty-map printing against app_projfield. CLI option parsing is intentionally
// outside these direct component tests.

// Covers header output before any projection has been built.
TEST(FldProjectorTest, PrintBeforeBuildEmitsOnlyHeader)
{
  FldProjector projector(1, 2, 3);
  projector.setGrid(4);
  projector.setOffset(5);
  projector.setCellsX(6);
  projector.setCellsY(7);

  testing::internal::CaptureStdout();
  projector.print();
  const std::vector<std::string> lines =
      Lines(testing::internal::GetCapturedStdout());

  ASSERT_EQ(lines.size(), 1u);
  EXPECT_TRUE(Contains(lines[0], "--x=1"));
  EXPECT_TRUE(Contains(lines[0], "--cy=7"));
}

// Covers a small deterministic 2x2 projection at angle 0.
TEST(FldProjectorTest, BuildsExpectedCardinalProjection)
{
  FldProjector projector(0, 0, 0);
  projector.setGrid(10);
  projector.setOffset(20);
  projector.setCellsX(2);
  projector.setCellsY(2);
  projector.buildProjection();

  testing::internal::CaptureStdout();
  projector.print();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "AA=x=20,y=-5\n"));
  EXPECT_TRUE(Contains(output, "AB=x=20,y=5\n"));
  EXPECT_TRUE(Contains(output, "BA=x=30,y=-5\n"));
  EXPECT_TRUE(Contains(output, "BB=x=30,y=5\n"));
}

// Covers rotation of the projected grid by a right angle.
TEST(FldProjectorTest, BuildsExpectedRotatedProjection)
{
  FldProjector projector(0, 0, 90);
  projector.setGrid(10);
  projector.setOffset(20);
  projector.setCellsX(2);
  projector.setCellsY(2);
  projector.buildProjection();

  testing::internal::CaptureStdout();
  projector.print();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "AA=x=-5,y=-20\n"));
  EXPECT_TRUE(Contains(output, "AB=x=5,y=-20\n"));
  EXPECT_TRUE(Contains(output, "BA=x=-5,y=-30\n"));
  EXPECT_TRUE(Contains(output, "BB=x=5,y=-30\n"));
}

// Covers label and terse-label pairs for each generated cell.
TEST(FldProjectorTest, PrintsFullAndTerseRowsForEachCell)
{
  FldProjector projector(0, 0, 0);
  projector.setCellsX(3);
  projector.setCellsY(2);
  projector.buildProjection();

  testing::internal::CaptureStdout();
  projector.print();
  const std::vector<std::string> lines =
      Lines(testing::internal::GetCapturedStdout());

  ASSERT_EQ(lines.size(), 1u + (3u * 2u * 2u));
  EXPECT_NE(std::find(lines.begin(), lines.end(), "AA=x=20,y=-10"),
            lines.end());
  EXPECT_NE(std::find(lines.begin(), lines.end(), "AAT=20,-10"),
            lines.end());
  EXPECT_NE(std::find(lines.begin(), lines.end(), "CB=x=40,y=0"),
            lines.end());
  EXPECT_NE(std::find(lines.begin(), lines.end(), "CBT=40,0"),
            lines.end());
}

// Covers repeated builds overwriting the same projection keys instead of
// accumulating duplicate output.
TEST(FldProjectorTest, RepeatedBuildsKeepOneProjectionPerCell)
{
  FldProjector projector(0, 0, 0);
  projector.setCellsX(2);
  projector.setCellsY(2);
  projector.buildProjection();
  projector.buildProjection();

  testing::internal::CaptureStdout();
  projector.print();
  const std::vector<std::string> lines =
      Lines(testing::internal::GetCapturedStdout());

  EXPECT_EQ(lines.size(), 1u + (2u * 2u * 2u));
}

// Covers setter propagation after construction.
TEST(FldProjectorTest, SettersOverrideConstructorValues)
{
  FldProjector projector(1, 2, 3);
  projector.setRootX(-4);
  projector.setRootY(5);
  projector.setAngle(90);
  projector.setGrid(6);
  projector.setOffset(7);
  projector.setCellsX(1);
  projector.setCellsY(1);

  testing::internal::CaptureStdout();
  projector.print();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "--x=-4"));
  EXPECT_TRUE(Contains(output, "--y=5"));
  EXPECT_TRUE(Contains(output, "--ang=90"));
  EXPECT_TRUE(Contains(output, "--grid=6"));
  EXPECT_TRUE(Contains(output, "--offset=7"));
}
