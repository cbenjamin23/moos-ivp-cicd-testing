#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ColorPack.h"
#include "ColorParse.h"
#include "NumericAssertions.h"

// Covers color parse behavior: parses named hex and decimal colors.
TEST(ColorParseTest, ParsesNamedHexAndDecimalColors)
{
  std::vector<double> red = colorParse("red");
  ASSERT_EQ(red.size(), 3u);
  EXPECT_NEAR(red[0], 1.0, kGeomTol);
  EXPECT_NEAR(red[1], 0.0, kGeomTol);
  EXPECT_NEAR(red[2], 0.0, kGeomTol);

  std::vector<double> dark_khaki = colorParse("Dark_Khaki");
  EXPECT_NEAR(dark_khaki[0], 0xbd / 255.0, kLooseGeomTol);
  EXPECT_NEAR(dark_khaki[1], 0xb7 / 255.0, kLooseGeomTol);
  EXPECT_NEAR(dark_khaki[2], 0x6b / 255.0, kLooseGeomTol);

  std::vector<double> decimal = colorParse("0.25:0.5:1.5");
  EXPECT_NEAR(decimal[0], 0.25, kGeomTol);
  EXPECT_NEAR(decimal[1], 0.5, kGeomTol);
  EXPECT_NEAR(decimal[2], 1.0, kGeomTol);
}

// Covers color parse behavior: parses viewer color forms and rejects malformed rgb.
TEST(ColorParseTest, ParsesViewerColorFormsAndRejectsMalformedRgb)
{
  std::vector<double> macpurple = colorParse("mac_purple");
  EXPECT_NEAR(macpurple[0], 0x49 / 255.0, kLooseGeomTol);
  EXPECT_NEAR(macpurple[1], 0x3d / 255.0, kLooseGeomTol);
  EXPECT_NEAR(macpurple[2], 0x78 / 255.0, kLooseGeomTol);

  std::vector<double> hex = colorHexToVector("hex: AD, ff, 2f");
  EXPECT_NEAR(hex[0], 0xad / 255.0, kLooseGeomTol);
  EXPECT_NEAR(hex[1], 1.0, kGeomTol);
  EXPECT_NEAR(hex[2], 0x2f / 255.0, kLooseGeomTol);

  std::vector<double> percent = colorParse("0.1%0.2%0.3");
  EXPECT_NEAR(percent[0], 0.1, kGeomTol);
  EXPECT_NEAR(percent[1], 0.2, kGeomTol);
  EXPECT_NEAR(percent[2], 0.3, kGeomTol);

  std::vector<double> dollar = colorDecToVector("-0.5 $ 0.5 $ 2.0");
  EXPECT_DOUBLE_EQ(dollar[0], 0.0);
  EXPECT_DOUBLE_EQ(dollar[1], 0.5);
  EXPECT_DOUBLE_EQ(dollar[2], 1.0);

  EXPECT_EQ(colorHexToVector("hex: gg,00,00"), std::vector<double>({0, 0, 0}));
  EXPECT_EQ(colorHexToVector("hex: f,00,00"), std::vector<double>({0, 0, 0}));
  EXPECT_EQ(colorDecToVector("0.1,0.2"), std::vector<double>({0, 0, 0}));
  EXPECT_EQ(colorDecToVector("0.1,blue,0.3"), std::vector<double>({0, 0, 0}));
}

// Covers color parse behavior: validates colors and converts back to strings.
TEST(ColorParseTest, ValidatesColorsAndConvertsBackToStrings)
{
  EXPECT_TRUE(isColor("blue"));
  EXPECT_TRUE(isColor("hex:00,80,ff"));
  EXPECT_TRUE(isColor("0.1,0.2,0.3"));
  EXPECT_TRUE(isColor("invisible"));
  EXPECT_FALSE(isColor("not_a_color"));

  EXPECT_EQ(colorNameToHex("dark_green"), "hex:00,64,00");
  EXPECT_EQ(colorVectorToString({1.2, 0.25, -0.1}), "1.000,0.250,0.000");
  EXPECT_EQ(colorVectorToString({0.1, 0.2}), "0,0,0");

  std::string color = "red";
  EXPECT_TRUE(setColorOnString(color, "green"));
  EXPECT_EQ(color, "green");
  EXPECT_FALSE(setColorOnString(color, "not_a_color"));
  EXPECT_EQ(color, "green");
}

// Covers color pack behavior: stores visibility named colors and vector colors.
TEST(ColorPackTest, StoresVisibilityNamedColorsAndVectorColors)
{
  ColorPack unset;
  EXPECT_FALSE(unset.set());
  EXPECT_TRUE(unset.visible());
  EXPECT_EQ(unset.str(), "black");

  ColorPack red("red");
  EXPECT_TRUE(red.set());
  EXPECT_TRUE(red.visible());
  EXPECT_EQ(red.str(), "red");
  EXPECT_NEAR(red.red(), 1.0, kGeomTol);
  EXPECT_NEAR(red.grn(), 0.0, kGeomTol);
  EXPECT_NEAR(red.blu(), 0.0, kGeomTol);

  ColorPack invisible("invisible");
  EXPECT_TRUE(invisible.set());
  EXPECT_FALSE(invisible.visible());
  EXPECT_EQ(invisible.str(), "invisible");

  ColorPack vector_color(0.1, 0.2, 0.3);
  EXPECT_EQ(vector_color.str(':'), "0.1:0.2:0.3");
}

// Covers color pack behavior: handles viewer invisible clear and invalid updates.
TEST(ColorPackTest, HandlesViewerInvisibleClearAndInvalidUpdates)
{
  ColorPack off("off");
  EXPECT_TRUE(off.set());
  EXPECT_FALSE(off.visible());
  EXPECT_EQ(off.str(), "invisible");

  ColorPack vector_color(std::vector<double>{0.2, 0.4, 0.6});
  EXPECT_TRUE(vector_color.set());
  EXPECT_EQ(vector_color.str('|'), "0.2|0.4|0.6");

  ColorPack bad_vector(std::vector<double>{0.2, 0.4});
  EXPECT_FALSE(bad_vector.set());
  EXPECT_TRUE(bad_vector.visible());
  EXPECT_EQ(bad_vector.str(), "0,0,0");

  ColorPack color("yellow");
  color.setColor("not_a_color");
  EXPECT_EQ(color.str(), "yellow");
  color.clear();
  EXPECT_FALSE(color.set());
  EXPECT_TRUE(color.visible());
  EXPECT_EQ(color.str(), "yellow");
  color.setColor("empty");
  EXPECT_TRUE(color.set());
  EXPECT_FALSE(color.visible());
  EXPECT_EQ(color.str(), "invisible");
}

// Covers color pack behavior: shades and grays colors with clipping.
TEST(ColorPackTest, ShadesAndGraysColorsWithClipping)
{
  ColorPack color(0.25, 0.5, 1.0);
  color.shade(0.5);
  EXPECT_NEAR(color.red(), 0.375, kGeomTol);
  EXPECT_NEAR(color.grn(), 0.75, kGeomTol);
  EXPECT_NEAR(color.blu(), 1.0, kGeomTol);

  color.shade(-2.0);
  EXPECT_NEAR(color.red(), 0.0, kGeomTol);
  EXPECT_NEAR(color.grn(), 0.0, kGeomTol);
  EXPECT_NEAR(color.blu(), 0.0, kGeomTol);

  ColorPack gray(1.0, 0.0, 0.5);
  gray.moregray(1.0);
  EXPECT_NEAR(gray.red(), 0.5, kGeomTol);
  EXPECT_NEAR(gray.grn(), 0.5, kGeomTol);
  EXPECT_NEAR(gray.blu(), 0.5, kGeomTol);
}

// Covers term color behavior: removes terminal color codes.
TEST(TermColorTest, RemovesTerminalColorCodes)
{
  EXPECT_FALSE(isTermColor("light_blue"));
  EXPECT_TRUE(isTermColor("lightblue"));
  EXPECT_TRUE(isTermColor("reversegreen"));
  EXPECT_FALSE(isTermColor("chartreuse"));

  std::string colored = termColor("red") + std::string("ALERT") + termColor();
  EXPECT_EQ(removeTermColors(colored), "ALERT");

  EXPECT_EQ(termColor("light_blue"), "\33[94m");
  EXPECT_EQ(termColor("unknown"), "\33[0m");
  std::string status =
      termColor("reverse_green") + std::string("DEPLOY") +
      termColor("lightcyan") + std::string(":true") + termColor("nocolor");
  EXPECT_EQ(removeTermColors(status), "DEPLOY:true");
}
