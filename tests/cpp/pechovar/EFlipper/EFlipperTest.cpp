#include <gtest/gtest.h>

#include <string>

#include "EFlipper.h"

namespace {

EFlipper ConfiguredFlipper()
{
  EFlipper flipper;
  EXPECT_TRUE(flipper.setParam("key", "click"));
  EXPECT_TRUE(flipper.setParam("source_variable", "MVIEWER_LCLICK"));
  EXPECT_TRUE(flipper.setParam("dest_variable", "UP_LOITER"));
  EXPECT_TRUE(flipper.setParam("component", "x -> xcenter_assign"));
  EXPECT_TRUE(flipper.setParam("component", "y -> ycenter_assign"));
  return flipper;
}

}  // namespace

// Source audit note: this suite covers EFlipper's deterministic parameter
// parser, required-field validation, source/destination alias guard, component
// and filter maps, separator handling, and flip filtering semantics. EchoVar's
// MOOS mail handling remains harness territory because it depends on runtime
// app state rather than this pure helper.

TEST(EFlipperTest, DefaultsAndRequiredFieldsControlValidity)
{
  EFlipper flipper;

  EXPECT_EQ(flipper.getSourceSep(), ",");
  EXPECT_EQ(flipper.getDestSep(), ",");
  EXPECT_FALSE(flipper.valid());

  EXPECT_TRUE(flipper.setParam("key", "click"));
  EXPECT_TRUE(flipper.setParam("source_variable", "MVIEWER_LCLICK"));
  EXPECT_TRUE(flipper.setParam("dest_variable", "UP_LOITER"));
  EXPECT_FALSE(flipper.valid());

  EXPECT_TRUE(flipper.setParam("component", "x -> xcenter_assign"));
  EXPECT_TRUE(flipper.valid());
}

TEST(EFlipperTest, ParameterNamesAreTrimmedAndCaseInsensitive)
{
  EFlipper flipper;

  EXPECT_TRUE(flipper.setParam(" KEY ", "  click  "));
  EXPECT_TRUE(flipper.setParam(" SOURCE_VARIABLE ", "  SRC  "));
  EXPECT_TRUE(flipper.setParam(" Dest_Variable ", "  DEST  "));
  EXPECT_TRUE(flipper.setParam(" Component ", "  x  ->  out_x  "));

  EXPECT_EQ(flipper.getKey(), "click");
  EXPECT_EQ(flipper.getSourceVar(), "SRC");
  EXPECT_EQ(flipper.getDestVar(), "DEST");
  EXPECT_EQ(flipper.getComponents(), "x:out_x");
  EXPECT_TRUE(flipper.valid());
}

TEST(EFlipperTest, RejectsSourceDestinationAliasWithoutMutation)
{
  EFlipper flipper;

  EXPECT_TRUE(flipper.setParam("source_variable", "SRC"));
  EXPECT_FALSE(flipper.setParam("dest_variable", "SRC"));
  EXPECT_EQ(flipper.getDestVar(), "");

  EXPECT_TRUE(flipper.setParam("dest_variable", "DEST"));
  EXPECT_FALSE(flipper.setParam("source_variable", "DEST"));
  EXPECT_EQ(flipper.getSourceVar(), "SRC");
}

TEST(EFlipperTest, UnknownParametersAreAcceptedWithoutChangingConfiguration)
{
  EFlipper flipper = ConfiguredFlipper();

  EXPECT_TRUE(flipper.setParam("unknown", "value"));

  EXPECT_EQ(flipper.getKey(), "click");
  EXPECT_EQ(flipper.getSourceVar(), "MVIEWER_LCLICK");
  EXPECT_EQ(flipper.getDestVar(), "UP_LOITER");
  EXPECT_EQ(flipper.getComponents(), "x:xcenter_assign,y:ycenter_assign");
}

TEST(EFlipperTest, ComponentMapRejectsMalformedMappings)
{
  EFlipper flipper;

  EXPECT_FALSE(flipper.setParam("component", "x"));
  EXPECT_FALSE(flipper.setParam("component", "x->y->z"));
  EXPECT_FALSE(flipper.setParam("component", "x=>y"));
  EXPECT_EQ(flipper.getComponents(), "");
}

TEST(EFlipperTest, ComponentMapIsCaseSensitiveSortedAndOverwritable)
{
  EFlipper flipper;

  EXPECT_TRUE(flipper.setParam("component", "x -> out_x"));
  EXPECT_TRUE(flipper.setParam("component", "Y -> out_y"));
  EXPECT_TRUE(flipper.setParam("component", "x -> final_x"));

  EXPECT_EQ(flipper.getComponents(), "Y:out_y,x:final_x");
  EXPECT_EQ(flipper.flip("x=10,Y=20,y=30"), "final_x=10,out_y=20");
}

TEST(EFlipperTest, EmptyLeftComponentIsStoredButEmptyRightIsRejected)
{
  EFlipper flipper;

  EXPECT_TRUE(flipper.setParam("component", " -> empty_left"));
  EXPECT_FALSE(flipper.setParam("component", "empty_right -> "));

  EXPECT_EQ(flipper.getComponents(), ":empty_left");
  EXPECT_EQ(flipper.flip("=1,empty_right=2"), "empty_left=1");
}

TEST(EFlipperTest, FilterMapRejectsMalformedMappings)
{
  EFlipper flipper;

  EXPECT_FALSE(flipper.setParam("filter", "mode"));
  EXPECT_FALSE(flipper.setParam("filter", "mode==survey==extra"));
  EXPECT_FALSE(flipper.setParam("filter", "mode=>survey"));
  EXPECT_EQ(flipper.getFilters(), "");
}

TEST(EFlipperTest, FilterMapLowercasesSortsAndOverwrites)
{
  EFlipper flipper;

  EXPECT_TRUE(flipper.setParam("filter", "MODE == SURVEY"));
  EXPECT_TRUE(flipper.setParam("filter", " vehicle == HENRY "));
  EXPECT_TRUE(flipper.setParam("filter", "mode == RETURN"));

  EXPECT_EQ(flipper.getFilters(), "mode:return,vehicle:henry");
}

TEST(EFlipperTest, EmptyLeftFilterIsStoredButEmptyRightIsRejected)
{
  EFlipper flipper;

  EXPECT_TRUE(flipper.setParam("filter", " == survey"));
  EXPECT_FALSE(flipper.setParam("filter", "mode == "));

  EXPECT_EQ(flipper.getFilters(), ":survey");
}

TEST(EFlipperTest, FlipMapsOnlyConfiguredComponentsAndTrimsValues)
{
  EFlipper flipper = ConfiguredFlipper();

  EXPECT_EQ(flipper.flip(" x = 10 , ignored=5 , y = 20 "),
            "xcenter_assign=10,ycenter_assign=20");
}

TEST(EFlipperTest, FlipPreservesDuplicateMappedComponents)
{
  EFlipper flipper = ConfiguredFlipper();

  EXPECT_EQ(flipper.flip("x=10,x=11,y=20"),
            "xcenter_assign=10,xcenter_assign=11,ycenter_assign=20");
}

TEST(EFlipperTest, MatchingFiltersAllowMappedOutput)
{
  EFlipper flipper = ConfiguredFlipper();
  ASSERT_TRUE(flipper.setParam("filter", "mode == survey"));

  EXPECT_EQ(flipper.flip("x=10,mode=SURVEY,y=20"),
            "xcenter_assign=10,ycenter_assign=20");
}

TEST(EFlipperTest, MissingFiltersDoNotBlockMappedOutput)
{
  EFlipper flipper = ConfiguredFlipper();
  ASSERT_TRUE(flipper.setParam("filter", "mode == survey"));

  EXPECT_EQ(flipper.flip("x=10,y=20"), "xcenter_assign=10,ycenter_assign=20");
}

TEST(EFlipperTest, MismatchedFilterSuppressesEntireOutput)
{
  EFlipper flipper = ConfiguredFlipper();
  ASSERT_TRUE(flipper.setParam("filter", "mode == survey"));

  EXPECT_EQ(flipper.flip("x=10,mode=return,y=20"), "");
}

TEST(EFlipperTest, MultiCharacterSeparatorsAreTrimmedAndSupported)
{
  EFlipper flipper = ConfiguredFlipper();
  ASSERT_TRUE(flipper.setParam("source_separator", " # "));
  ASSERT_TRUE(flipper.setParam("dest_separator", " | "));

  EXPECT_EQ(flipper.getSourceSep(), "#");
  EXPECT_EQ(flipper.getDestSep(), "|");
  EXPECT_EQ(flipper.flip("x=10#y=20"), "xcenter_assign=10|ycenter_assign=20");
}

TEST(EFlipperTest, EmptySeparatorsAreAcceptedByConfiguration)
{
  EFlipper flipper = ConfiguredFlipper();

  EXPECT_TRUE(flipper.setParam("source_separator", " "));
  EXPECT_TRUE(flipper.setParam("dest_separator", " "));

  EXPECT_EQ(flipper.getSourceSep(), "");
  EXPECT_EQ(flipper.getDestSep(), "");
  EXPECT_TRUE(flipper.valid());
}

TEST(EFlipperTest, UnmappedOrEmptyInputProducesNoOutput)
{
  EFlipper flipper = ConfiguredFlipper();

  EXPECT_EQ(flipper.flip("z=10"), "");
  EXPECT_EQ(flipper.flip(""), "");
}

TEST(EFlipperTest, MissingEqualsForMappedComponentProducesEmptyValue)
{
  EFlipper flipper = ConfiguredFlipper();

  EXPECT_EQ(flipper.flip("x"), "xcenter_assign=");
}
