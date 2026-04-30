#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "PolarPlot.h"
#include "PopulatorPolarPlot.h"
#include "TestFileUtils.h"
#include "WindModel.h"

namespace {

std::string writeFixture(const std::string& name, const std::string& content)
{
  // Keep file-backed polar fixtures alive for PopulatorPolarPlot reads.
  static std::vector<std::unique_ptr<TempFile>> files;
  files.emplace_back(new TempFile(name, content));
  return files.back()->path();
}

}  // namespace

// Covers polar plot behavior: parses uSim marine polar plot configuration.
TEST(PolarPlotTest, ParsesUSimMarinePolarPlotConfiguration)
{
  PolarPlot plot;
  EXPECT_FALSE(plot.set());
  // uSimMarine-style polar plots map relative wind headings to speed fractions.
  EXPECT_TRUE(plot.addSetPoints("0,0: 20,40: 45,65: 90,80: 110,90: 135,83: 150,83: 165,60: 180,50"));
  EXPECT_TRUE(plot.set());
  EXPECT_EQ(plot.size(), 9u);
  EXPECT_EQ(plot.getSetHdgs(), (std::vector<double>{0, 20, 45, 90, 110, 135, 150, 165, 180}));
  EXPECT_EQ(plot.getSetVals(), (std::vector<double>{0, 40, 65, 80, 90, 83, 83, 60, 50}));
  EXPECT_EQ(plot.getSpec(),
            "0,0:20,40:45,65:90,80:110,90:135,83:150,83:165,60:180,50");
}

// Covers polar plot behavior: rejects malformed and duplicate set points atomically.
TEST(PolarPlotTest, RejectsMalformedAndDuplicateSetPointsAtomically)
{
  PolarPlot plot;
  EXPECT_FALSE(plot.addSetPoints(""));
  EXPECT_FALSE(plot.addSetPoint(-1, 10));
  EXPECT_FALSE(plot.addSetPoint(181, 10));
  EXPECT_TRUE(plot.addSetPoint(90, 80));
  EXPECT_FALSE(plot.addSetPoint(90, 81));
  EXPECT_FALSE(plot.addSetPoints("0,0:bad,10"));
  EXPECT_FALSE(plot.addSetPoints("0,0:200,10"));
  EXPECT_FALSE(plot.addSetPoints("0,0:45,-1"));
  EXPECT_EQ(plot.size(), 1u);
  EXPECT_EQ(plot.getSpec(), "90,80");
}

// Covers polar plot behavior: direct set point insertion allows values string parser rejects.
TEST(PolarPlotTest, DirectSetPointInsertionAllowsValuesStringParserRejects)
{
  PolarPlot plot;
  // Direct insertion and string parsing do not enforce exactly the same value
  // range rules; this pins the looser setter path.
  EXPECT_TRUE(plot.addSetPoint(45, -10));
  EXPECT_TRUE(plot.addSetPoint(90, 150));
  EXPECT_EQ(plot.getSpec(), "45,-10:90,150");

  PolarPlot parsed;
  EXPECT_FALSE(parsed.addSetPoints("45,-10"));
  EXPECT_TRUE(parsed.addSetPoints("90,150"));
  EXPECT_EQ(parsed.getSpec(), "90,150");
  EXPECT_NEAR(parsed.getPolarPct(90), 1.0, 1e-9);
}

// Covers polar plot behavior: batch duplicate headings return success but only first value persists.
TEST(PolarPlotTest, BatchDuplicateHeadingsReturnSuccessButOnlyFirstValuePersists)
{
  PolarPlot plot;
  EXPECT_TRUE(plot.addSetPoints("0,0:45,50:45,60:90,80"));
  EXPECT_EQ(plot.size(), 3u);
  EXPECT_EQ(plot.getSpec(), "0,0:45,50:90,80");

  EXPECT_FALSE(plot.addSetPoints("135,70:90,81"));
  EXPECT_EQ(plot.getSpec(), "0,0:45,50:90,80");
}

// Covers polar plot behavior: interpolates symmetric headings and clips percent at one.
TEST(PolarPlotTest, InterpolatesSymmetricHeadingsAndClipsPercentAtOne)
{
  PolarPlot plot = stringToPolarPlot("0,0:90,80:180,120");

  EXPECT_NEAR(plot.getPolarPct(0), 0, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(45), 0.4, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(90), 0.8, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(135), 1.0, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(180), 1.0, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(270), 0.8, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(315), 0.4, 1e-9);
  EXPECT_EQ(plot.getAllHdgs().size(), 360u);
  EXPECT_EQ(plot.getAllVals().size(), 360u);
}

// Covers polar plot behavior: rounds heading queries and wraps only before integer conversion.
TEST(PolarPlotTest, RoundsHeadingQueriesAndWrapsOnlyBeforeIntegerConversion)
{
  PolarPlot plot = stringToPolarPlot("0,10:90,100:180,10");

  // Heading queries wrap before integer conversion, so values near 0/360 land
  // on different cached headings depending on the fractional side.
  EXPECT_NEAR(plot.getPolarPct(-0.51), 0.11, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(-0.49), 0.0, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(0.49), 0.1, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(0.5), 0.11, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(359.49), 0.11, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(359.5), 0.0, 1e-9);
}

// Covers polar plot behavior: rotates set and interpolated headings with wind angle.
TEST(PolarPlotTest, RotatesSetAndInterpolatedHeadingsWithWindAngle)
{
  PolarPlot plot = stringToPolarPlot("0,0:90,80:180,50");
  plot.setWindAngle(350);
  EXPECT_EQ(plot.getWindAngle(), 350);
  EXPECT_EQ(plot.getSetHdgsRotated(), (std::vector<double>{350, 80, 170}));

  plot.modWindAngle(25);
  EXPECT_EQ(plot.getWindAngle(), 15);
  EXPECT_EQ(plot.getSetHdgsRotated(), (std::vector<double>{15, 105, 195}));

  EXPECT_NEAR(plot.getPolarPct(105), 0.8, 1e-9);
}

// Covers polar plot behavior: modifies and removes nearest set point.
TEST(PolarPlotTest, ModifiesAndRemovesNearestSetPoint)
{
  PolarPlot plot = stringToPolarPlot("0,0:45,50:90,80:180,50");

  plot.modPoint(46, 55);
  EXPECT_EQ(plot.size(), 4u);
  EXPECT_EQ(plot.getSpec(), "0,0:46,55:90,80:180,50");

  plot.removePoint(47, 54);
  EXPECT_EQ(plot.size(), 3u);
  EXPECT_EQ(plot.getSpec(), "0,0:90,80:180,50");
}

// Covers polar plot behavior: empty plot modification adds or no ops around heading zero.
TEST(PolarPlotTest, EmptyPlotModificationAddsOrNoOpsAroundHeadingZero)
{
  PolarPlot plot;
  plot.removePoint(90, 80);
  EXPECT_EQ(plot.size(), 0u);

  plot.modPoint(90, 80);
  EXPECT_EQ(plot.getSpec(), "90,80");

  plot.modPoint(45, 50);
  EXPECT_EQ(plot.getSpec(), "45,50");
}

// Covers polar plot behavior: wind angle changes invalidate cached interpolated values.
TEST(PolarPlotTest, WindAngleChangesInvalidateCachedInterpolatedValues)
{
  PolarPlot plot = stringToPolarPlot("0,0:90,100:180,0");
  EXPECT_NEAR(plot.getPolarPct(90), 1.0, 1e-9);

  plot.setWindAngle(90);
  EXPECT_NEAR(plot.getPolarPct(180), 1.0, 1e-9);
  EXPECT_NEAR(plot.getPolarPct(90), 0.0, 1e-9);

  plot.modWindAngle(-90);
  EXPECT_NEAR(plot.getPolarPct(90), 1.0, 1e-9);
}

// Covers wind model behavior: parses and serializes uniform wind conditions.
TEST(WindModelTest, ParsesAndSerializesUniformWindConditions)
{
  WindModel model;
  EXPECT_FALSE(model.set());
  EXPECT_TRUE(model.setConditions("spd=2.4,dir=135"));
  EXPECT_TRUE(model.set());
  EXPECT_DOUBLE_EQ(model.getWindSpd(), 2.4);
  EXPECT_DOUBLE_EQ(model.getWindDir(), 135);
  EXPECT_EQ(model.getWindSpd(10, 20), 2.4);
  EXPECT_EQ(model.getWindDir(10, 20), 135);
  EXPECT_EQ(model.getModelSpec(), "spd=2.4, dir=135");

  model.setTimeUTC(123.5);
  EXPECT_DOUBLE_EQ(model.getTimeUTC(), 123.5);
  EXPECT_TRUE(model.getArrowSpec().find("label=windmod") != std::string::npos);
  EXPECT_TRUE(model.getArrowSpec(10, 20).find("x=10") != std::string::npos);
}

// Covers wind model behavior: rejects bad condition specs and clips speed mods.
TEST(WindModelTest, RejectsBadConditionSpecsAndClipsSpeedMods)
{
  WindModel model;
  EXPECT_TRUE(model.setConditions("spd=-1,dir=90"));
  EXPECT_TRUE(model.set());
  EXPECT_DOUBLE_EQ(model.getWindSpd(), 0);
  EXPECT_DOUBLE_EQ(model.getWindDir(), 90);

  WindModel bad_model;
  EXPECT_FALSE(model.setConditions("spd=1,dir=-10"));
  EXPECT_FALSE(model.setConditions("speed=1,dir=90"));
  EXPECT_FALSE(bad_model.setConditions("speed=1,dir=90"));
  EXPECT_FALSE(bad_model.set());

  EXPECT_TRUE(model.setWindSpd(3));
  EXPECT_FALSE(model.setWindSpd(-1));
  EXPECT_DOUBLE_EQ(model.getWindSpd(), 3);
  EXPECT_TRUE(model.modWindSpd(-10));
  EXPECT_DOUBLE_EQ(model.getWindSpd(), 0);
  EXPECT_TRUE(model.setWindDir(370));
  EXPECT_DOUBLE_EQ(model.getWindDir(), 10);
  EXPECT_TRUE(model.modWindDir(-20));
  EXPECT_DOUBLE_EQ(model.getWindDir(), 350);
}

// Covers wind model behavior: failed condition parse may leave partial state.
TEST(WindModelTest, FailedConditionParseMayLeavePartialState)
{
  WindModel model;
  // setConditions() may reject the whole spec while leaving earlier parsed
  // fields applied to the model.
  EXPECT_FALSE(model.setConditions("spd=4,dir=-10"));
  EXPECT_TRUE(model.set());
  EXPECT_DOUBLE_EQ(model.getWindSpd(), 4);
  EXPECT_DOUBLE_EQ(model.getWindDir(), 0);
  EXPECT_EQ(model.getModelSpec(), "spd=4, dir=0");

  EXPECT_FALSE(model.setConditions("dir=90,extra=value"));
  EXPECT_DOUBLE_EQ(model.getWindDir(), 90);
  EXPECT_EQ(model.getModelSpec(), "spd=4, dir=90");
}

// Covers wind model behavior: computes max sailing speed from polar plot.
TEST(WindModelTest, ComputesMaxSailingSpeedFromPolarPlot)
{
  WindModel model = stringToWindModel("spd=5,dir=0");
  PolarPlot plot = stringToPolarPlot("0,0:90,80:180,50");

  EXPECT_NEAR(model.getMaxSpeed(plot, 45), 2.0, 1e-9);
  EXPECT_NEAR(model.getMaxSpeed(plot, 90), 4.0, 1e-9);
  EXPECT_NEAR(model.getMaxSpeed(plot, 180), 2.5, 1e-9);
}

// Covers wind model behavior: applies temporary arrow parameters and falls back on invalid ones.
TEST(WindModelTest, AppliesTemporaryArrowParametersAndFallsBackOnInvalidOnes)
{
  WindModel model = stringToWindModel("spd=2,dir=90");
  std::string base = model.getArrowSpec();
  std::string custom = model.getArrowSpec("label=test_wind,fill_color=blue");

  EXPECT_NE(custom.find("label=test_wind"), std::string::npos);
  EXPECT_NE(custom.find("fill_color=blue"), std::string::npos);
  EXPECT_EQ(model.getArrowSpec("bad_param=value"), base);
}

// Covers wind model behavior: string conversion ignores unknown malformed and negative speed fields.
TEST(WindModelTest, StringConversionIgnoresUnknownMalformedAndNegativeSpeedFields)
{
  WindModel model = stringToWindModel("spd=5,dir=450,bad=value,spd=-2");

  EXPECT_TRUE(model.set());
  EXPECT_DOUBLE_EQ(model.getWindSpd(), 5);
  EXPECT_DOUBLE_EQ(model.getWindDir(), 90);
  EXPECT_EQ(model.getWindSpd(100, -50), 5);
  EXPECT_EQ(model.getWindDir(100, -50), 90);
}

// Covers populator polar plot behavior: reads file backed polar plots.
TEST(PopulatorPolarPlotTest, ReadsFileBackedPolarPlots)
{
  const std::string path = writeFixture(
      "polar_plot.txt",
      "// heading=speed fraction\n"
      "0=0\n"
      "45=0.5\n"
      "90=0.8\n"
      "180=0.4");

  PopulatorPolarPlot populator;
  ASSERT_TRUE(populator.readFile(path));
  PolarPlot plot = populator.getPolarPlot();
  EXPECT_EQ(plot.size(), 4u);
  EXPECT_EQ(plot.getSpec(), "0,0:45,0.5:90,0.8:180,0.4");
}

// Covers populator polar plot behavior: rejects bad files but may keep prior good plot.
TEST(PopulatorPolarPlotTest, RejectsBadFilesButMayKeepPriorGoodPlot)
{
  const std::string good = writeFixture("polar_plot_good.txt", "0=0\n90=0.8");
  const std::string bad = writeFixture("polar_plot_bad.txt", "0=0\n90=1.2");

  PopulatorPolarPlot populator;
  EXPECT_TRUE(populator.readFile(good));
  // A failed read reports false but does not necessarily clear the last good plot.
  EXPECT_FALSE(populator.readFile(bad));
  EXPECT_EQ(populator.getPolarPlot().getSpec(), "0,0:90,0.8");
  EXPECT_FALSE(populator.readFile(std::string(::testing::TempDir()) + "/missing_polar_plot.txt"));
}

// Covers populator polar plot behavior: accepts loose file heading range but polar plot keeps valid subset.
TEST(PopulatorPolarPlotTest, AcceptsLooseFileHeadingRangeButPolarPlotKeepsValidSubset)
{
  const std::string path = writeFixture(
      "polar_plot_loose_headings.txt",
      "-180=0.2\n"
      "0=0\n"
      "180=0.4\n"
      "270=0.6\n"
      "360=0.8");

  PopulatorPolarPlot populator;
  EXPECT_TRUE(populator.readFile(path));
  EXPECT_EQ(populator.getPolarPlot().getSpec(), "0,0:180,0.4");
}
