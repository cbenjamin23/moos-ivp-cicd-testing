#include <gtest/gtest.h>

#include "InfoCastSettings.h"

// Covers info cast settings behavior: starts with app cast defaults used by viewer tools.
TEST(InfoCastSettingsTest, StartsWithAppCastDefaultsUsedByViewerTools)
{
  InfoCastSettings settings;
  EXPECT_TRUE(settings.getInfoCastViewable());
  EXPECT_FALSE(settings.getFullScreen());
  EXPECT_EQ(settings.getContentMode(), "appcast");
  EXPECT_EQ(settings.getRefreshMode(), "events");
  EXPECT_EQ(settings.getInfoCastLayout(), "regular");
  EXPECT_EQ(settings.getInfoCastFontSize(), "medium");
  EXPECT_EQ(settings.getNodesFontSize(), "large");
  EXPECT_EQ(settings.getProcsFontSize(), "large");
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 70);
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 30);
  EXPECT_DOUBLE_EQ(settings.getInfoCastNodeWidth(), 300);
  EXPECT_EQ(settings.getAppCastColorScheme(), "dark_indigo");
  EXPECT_EQ(settings.getRealmCastColorScheme(), "hillside");
  EXPECT_TRUE(settings.getShowRealmCastSource());
  EXPECT_TRUE(settings.getShowRealmCastCommunity());
  EXPECT_TRUE(settings.getShowRealmCastSubs());
  EXPECT_TRUE(settings.getShowRealmCastMasked());
  EXPECT_FALSE(settings.getWrapRealmCastContent());
  EXPECT_FALSE(settings.getTruncRealmCastContent());
  EXPECT_FALSE(settings.getRealmCastTimeFormatUTC());
}

// Covers info cast settings behavior: parses booleans with MOOS config style synonyms.
TEST(InfoCastSettingsTest, ParsesBooleansWithMoosConfigStyleSynonyms)
{
  InfoCastSettings settings;
  EXPECT_TRUE(settings.setInfoCastViewable("false"));
  EXPECT_FALSE(settings.getInfoCastViewable());
  EXPECT_TRUE(settings.setInfoCastViewable("on"));
  EXPECT_TRUE(settings.getInfoCastViewable());
  EXPECT_TRUE(settings.setFullScreen("true"));
  EXPECT_TRUE(settings.getFullScreen());
  EXPECT_FALSE(settings.setFullScreen("maybe"));
  EXPECT_TRUE(settings.getFullScreen());

  EXPECT_TRUE(settings.setShowRealmCastSource("off"));
  EXPECT_FALSE(settings.getShowRealmCastSource());
  EXPECT_TRUE(settings.setShowRealmCastCommunity("false"));
  EXPECT_FALSE(settings.getShowRealmCastCommunity());
  EXPECT_TRUE(settings.setShowRealmCastSubs("yes"));
  EXPECT_TRUE(settings.getShowRealmCastSubs());
  EXPECT_TRUE(settings.setShowRealmCastMasked("no"));
  EXPECT_FALSE(settings.getShowRealmCastMasked());
  EXPECT_TRUE(settings.setWrapRealmCastContent("true"));
  EXPECT_TRUE(settings.getWrapRealmCastContent());
  EXPECT_TRUE(settings.setTruncRealmCastContent("true"));
  EXPECT_TRUE(settings.getTruncRealmCastContent());
  EXPECT_TRUE(settings.setRealmCastTimeFormatUTC("true"));
  EXPECT_TRUE(settings.getRealmCastTimeFormatUTC());
}

// Covers info cast settings behavior: toggles content refresh and layout modes.
TEST(InfoCastSettingsTest, TogglesContentRefreshAndLayoutModes)
{
  InfoCastSettings settings;
  EXPECT_TRUE(settings.setContentMode("realmcast"));
  EXPECT_EQ(settings.getContentMode(), "realmcast");
  EXPECT_TRUE(settings.setContentMode("toggle"));
  EXPECT_EQ(settings.getContentMode(), "appcast");
  EXPECT_FALSE(settings.setContentMode("summary"));
  EXPECT_EQ(settings.getContentMode(), "appcast");

  EXPECT_TRUE(settings.setRefreshMode("streaming"));
  EXPECT_EQ(settings.getRefreshMode(), "streaming");
  EXPECT_TRUE(settings.setRefreshMode("PAUSED"));
  EXPECT_EQ(settings.getRefreshMode(), "paused");
  EXPECT_FALSE(settings.setRefreshMode("rapid"));
  EXPECT_EQ(settings.getRefreshMode(), "paused");

  EXPECT_TRUE(settings.setInfoCastLayout("swarm"));
  EXPECT_EQ(settings.getInfoCastLayout(), "swarm");
  EXPECT_TRUE(settings.setInfoCastLayout("toggle"));
  EXPECT_EQ(settings.getInfoCastLayout(), "fullcast");
  EXPECT_TRUE(settings.setInfoCastLayout("toggle"));
  EXPECT_EQ(settings.getInfoCastLayout(), "regular");
  EXPECT_FALSE(settings.setInfoCastLayout("stacked"));
  EXPECT_EQ(settings.getInfoCastLayout(), "regular");
}

// Covers info cast settings behavior: pins mode case whitespace and realm default quirks.
TEST(InfoCastSettingsTest, PinsModeCaseWhitespaceAndRealmDefaultQuirks)
{
  InfoCastSettings settings;

  EXPECT_TRUE(settings.setContentMode("REALMCAST"));
  EXPECT_EQ(settings.getContentMode(), "realmcast");
  EXPECT_FALSE(settings.setContentMode(" realmcast "));
  EXPECT_EQ(settings.getContentMode(), "realmcast");

  EXPECT_FALSE(settings.setInfoCastLayout("SWARM"));
  EXPECT_EQ(settings.getInfoCastLayout(), "regular");
  EXPECT_FALSE(settings.setRefreshMode(" events "));
  EXPECT_EQ(settings.getRefreshMode(), "events");

  EXPECT_TRUE(settings.setRealmCastColorScheme("indigo"));
  EXPECT_EQ(settings.getRealmCastColorScheme(), "indigo");
  EXPECT_TRUE(settings.setRealmCastColorScheme("default"));
  EXPECT_EQ(settings.getRealmCastColorScheme(), "indigo");
  EXPECT_EQ(settings.getAppCastColorScheme(), "hillside");
}

// Covers info cast settings behavior: clips and snaps info cast panel dimensions.
TEST(InfoCastSettingsTest, ClipsAndSnapsInfoCastPanelDimensions)
{
  InfoCastSettings settings;

  EXPECT_TRUE(settings.setInfoCastHeight("83"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 85);
  EXPECT_TRUE(settings.setInfoCastHeight("delta:20"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 90);
  EXPECT_TRUE(settings.setInfoCastHeight("delta:-100"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 30);
  EXPECT_FALSE(settings.setInfoCastHeight("delta:bad"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 30);

  EXPECT_TRUE(settings.setInfoCastWidth("46"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 45);
  EXPECT_TRUE(settings.setInfoCastWidth("delta:100"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 70);
  EXPECT_TRUE(settings.setInfoCastWidth("delta:-100"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 20);
  EXPECT_FALSE(settings.setInfoCastWidth("wide"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 20);

  EXPECT_TRUE(settings.setInfoCastNodeWidth("234"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastNodeWidth(), 225);
  EXPECT_TRUE(settings.setInfoCastNodeWidth("delta:100"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastNodeWidth(), 250);
  EXPECT_TRUE(settings.setInfoCastNodeWidth("delta:-300"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastNodeWidth(), 100);
}

// Covers info cast settings behavior: pins panel dimension boundary and parsing rules.
TEST(InfoCastSettingsTest, PinsPanelDimensionBoundaryAndParsingRules)
{
  InfoCastSettings settings;

  EXPECT_TRUE(settings.setInfoCastHeight("29"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 30);
  EXPECT_TRUE(settings.setInfoCastHeight("91"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 90);
  EXPECT_FALSE(settings.setInfoCastHeight(" delta:5"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastHeight(), 90);

  EXPECT_TRUE(settings.setInfoCastWidth("-1"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 20);
  EXPECT_TRUE(settings.setInfoCastWidth("72"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 70);
  EXPECT_FALSE(settings.setInfoCastWidth("DELTA:5"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastWidth(), 70);

  EXPECT_TRUE(settings.setInfoCastNodeWidth("99"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastNodeWidth(), 100);
  EXPECT_TRUE(settings.setInfoCastNodeWidth("251"));
  EXPECT_DOUBLE_EQ(settings.getInfoCastNodeWidth(), 250);
}

// Covers info cast settings behavior: cycles font sizes and stops at edges for bigger smaller.
TEST(InfoCastSettingsTest, CyclesFontSizesAndStopsAtEdgesForBiggerSmaller)
{
  InfoCastSettings settings;
  EXPECT_TRUE(settings.setInfoCastFontSize("bigger"));
  EXPECT_EQ(settings.getInfoCastFontSize(), "large");
  EXPECT_TRUE(settings.setInfoCastFontSize("bigger"));
  EXPECT_EQ(settings.getInfoCastFontSize(), "xlarge");
  EXPECT_TRUE(settings.setInfoCastFontSize("bigger"));
  EXPECT_EQ(settings.getInfoCastFontSize(), "xlarge");
  EXPECT_TRUE(settings.setInfoCastFontSize("toggle"));
  EXPECT_EQ(settings.getInfoCastFontSize(), "xsmall");
  EXPECT_TRUE(settings.setInfoCastFontSize("smaller"));
  EXPECT_EQ(settings.getInfoCastFontSize(), "xsmall");

  EXPECT_TRUE(settings.setNodesFontSize("small"));
  EXPECT_EQ(settings.getNodesFontSize(), "small");
  EXPECT_TRUE(settings.setProcsFontSize("xsmall"));
  EXPECT_EQ(settings.getProcsFontSize(), "xsmall");
  EXPECT_FALSE(settings.setProcsFontSize("huge"));
  EXPECT_EQ(settings.getProcsFontSize(), "xsmall");
}

// Covers info cast settings behavior: pins independent font cycles for nodes procs and content.
TEST(InfoCastSettingsTest, PinsIndependentFontCyclesForNodesProcsAndContent)
{
  InfoCastSettings settings;

  EXPECT_TRUE(settings.setNodesFontSize("SMALLER"));
  EXPECT_EQ(settings.getNodesFontSize(), "medium");
  EXPECT_TRUE(settings.setNodesFontSize("toggle"));
  EXPECT_EQ(settings.getNodesFontSize(), "large");

  EXPECT_TRUE(settings.setProcsFontSize("toggle"));
  EXPECT_EQ(settings.getProcsFontSize(), "xlarge");
  EXPECT_TRUE(settings.setProcsFontSize("bigger"));
  EXPECT_EQ(settings.getProcsFontSize(), "xlarge");

  EXPECT_TRUE(settings.setInfoCastFontSize("xsmall"));
  EXPECT_TRUE(settings.setInfoCastFontSize("toggle"));
  EXPECT_EQ(settings.getInfoCastFontSize(), "small");
}

// Covers info cast settings behavior: accepts and cycles known color schemes.
TEST(InfoCastSettingsTest, AcceptsAndCyclesKnownColorSchemes)
{
  InfoCastSettings settings;
  EXPECT_TRUE(settings.setAppCastColorScheme("white"));
  EXPECT_EQ(settings.getAppCastColorScheme(), "white");
  EXPECT_TRUE(settings.setAppCastColorScheme("toggle"));
  EXPECT_EQ(settings.getAppCastColorScheme(), "indigo");
  EXPECT_TRUE(settings.setAppCastColorScheme("toggle"));
  EXPECT_EQ(settings.getAppCastColorScheme(), "dark_indigo");
  EXPECT_TRUE(settings.setAppCastColorScheme("default"));
  EXPECT_EQ(settings.getAppCastColorScheme(), "indigo");
  EXPECT_FALSE(settings.setAppCastColorScheme("chartreuse"));
  EXPECT_EQ(settings.getAppCastColorScheme(), "indigo");

  EXPECT_TRUE(settings.setRealmCastColorScheme("white"));
  EXPECT_EQ(settings.getRealmCastColorScheme(), "white");
  EXPECT_TRUE(settings.setRealmCastColorScheme("toggle"));
  EXPECT_EQ(settings.getRealmCastColorScheme(), "hillside");
  EXPECT_TRUE(settings.setRealmCastColorScheme("toggle"));
  EXPECT_EQ(settings.getRealmCastColorScheme(), "beige");
  EXPECT_FALSE(settings.setRealmCastColorScheme("night"));
  EXPECT_EQ(settings.getRealmCastColorScheme(), "beige");
}
