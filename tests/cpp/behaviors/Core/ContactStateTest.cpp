#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ContactState.h"

// Covers contact state behavior: defaults to no contact geometry or events.
TEST(ContactStateTest, DefaultsToNoContactGeometryOrEvents)
{
  ContactState state;

  EXPECT_DOUBLE_EQ(state.cnutc(), 0);
  EXPECT_FALSE(state.update_ok());
  EXPECT_EQ(state.helm_iter(), 0u);
  EXPECT_EQ(state.get_index(), 0u);
  EXPECT_DOUBLE_EQ(state.range(), 0);
  EXPECT_FALSE(state.os_fore_of_cn());
  EXPECT_FALSE(state.cn_passes_os());
  EXPECT_FALSE(state.os_crosses_cn_bow());
  EXPECT_DOUBLE_EQ(state.os_curr_cpa_dist(), 0);
}

// Covers contact state behavior: stores relative geometry and macro expands contact fields.
TEST(ContactStateTest, StoresRelativeGeometryAndMacroExpandsContactFields)
{
  ContactState state;
  state.set_cnutc(123.4);
  state.set_update_ok(true);
  state.set_helm_iter(7);
  state.set_index(3);
  state.set_range(42.5);
  state.set_os_fore_of_cn(true);
  state.set_cn_port_of_os(true);
  state.set_cn_spd_in_os_pos(1.24);
  state.set_os_cn_rel_bng(17.28);
  state.set_cn_os_rel_bng(202.4);
  state.set_bearing_rate(-3.21);
  state.set_rate_of_closure(0.49);
  state.set_os_crosses_cn_bow(true);
  state.set_os_crosses_cn_bow_dist(14.6);
  state.set_cn_crosses_os_stern(true);
  state.set_os_curr_cpa_dist(9.8);

  EXPECT_DOUBLE_EQ(state.cnutc(), 123.4);
  EXPECT_TRUE(state.update_ok());
  EXPECT_EQ(state.helm_iter(), 7u);
  EXPECT_EQ(state.get_index(), 3u);
  EXPECT_DOUBLE_EQ(state.range(), 42.5);
  EXPECT_TRUE(state.os_fore_of_cn());
  EXPECT_TRUE(state.cn_port_of_os());
  EXPECT_DOUBLE_EQ(state.os_crosses_cn_bow_dist(), 14.6);
  EXPECT_TRUE(state.cn_crosses_os_stern());
  EXPECT_DOUBLE_EQ(state.os_curr_cpa_dist(), 9.8);

  std::string text = "$[ROC]|$[OS_CN_REL_BNG]|$[CN_OS_REL_BNG]|"
                     "$[BNG_RATE]|$[OS_FORE_OF_CN]|$[CN_PORT_OF_OS]|"
                     "$[CN_SPD_IN_OS_POS]";
  text = state.cnMacroExpand(text, "ROC");
  text = state.cnMacroExpand(text, "OS_CN_REL_BNG");
  text = state.cnMacroExpand(text, "CN_OS_REL_BNG");
  text = state.cnMacroExpand(text, "BNG_RATE");
  text = state.cnMacroExpand(text, "OS_FORE_OF_CN");
  text = state.cnMacroExpand(text, "CN_PORT_OF_OS");
  text = state.cnMacroExpand(text, "CN_SPD_IN_OS_POS");

  EXPECT_EQ(text, "0.5|17.3|202.4|-3.2|true|true|1.2");
}

// Covers contact state behavior: expands all relative position macros used by contact behaviors.
TEST(ContactStateTest, ExpandsAllRelativePositionMacrosUsedByContactBehaviors)
{
  ContactState state;
  state.set_os_fore_of_cn(true);
  state.set_os_aft_of_cn(false);
  state.set_os_port_of_cn(true);
  state.set_os_star_of_cn(false);
  state.set_cn_fore_of_os(false);
  state.set_cn_aft_of_os(true);
  state.set_cn_port_of_os(false);
  state.set_cn_star_of_os(true);

  std::string text = "$[OS_FORE_OF_CN]|$[OS_AFT_OF_CN]|"
                     "$[OS_PORT_OF_CN]|$[OS_STAR_OF_CN]|"
                     "$[CN_FORE_OF_OS]|$[CN_AFT_OF_OS]|"
                     "$[CN_PORT_OF_OS]|$[CN_STAR_OF_OS]|"
                     "$[UNKNOWN_CONTACT_MACRO]";
  const std::vector<std::string> macros = {
      "OS_FORE_OF_CN", "OS_AFT_OF_CN", "OS_PORT_OF_CN", "OS_STAR_OF_CN",
      "CN_FORE_OF_OS", "CN_AFT_OF_OS", "CN_PORT_OF_OS", "CN_STAR_OF_OS",
      "UNKNOWN_CONTACT_MACRO"};

  for(const std::string& macro : macros)
    text = state.cnMacroExpand(text, macro);

  EXPECT_EQ(text,
            "true|false|true|false|false|true|false|true|"
            "$[UNKNOWN_CONTACT_MACRO]");
}

// Covers contact state behavior: stores predicted passing crossing and CPA quantities.
TEST(ContactStateTest, StoresPredictedPassingCrossingAndCpaQuantities)
{
  ContactState state;
  state.set_os_passes_cn(true);
  state.set_os_passes_cn_port(true);
  state.set_os_passes_cn_star(false);
  state.set_cn_passes_os(true);
  state.set_cn_passes_os_port(false);
  state.set_cn_passes_os_star(true);
  state.set_os_crosses_cn(true);
  state.set_os_crosses_cn_stern(false);
  state.set_os_crosses_cn_bow(true);
  state.set_os_crosses_cn_bow_dist(27.5);
  state.set_cn_crosses_os(true);
  state.set_cn_crosses_os_bow(false);
  state.set_cn_crosses_os_stern(true);
  state.set_cn_crosses_os_bow_dist(13.25);
  state.set_os_curr_cpa_dist(4.5);

  EXPECT_TRUE(state.os_passes_cn());
  EXPECT_TRUE(state.os_passes_cn_port());
  EXPECT_FALSE(state.os_passes_cn_star());
  EXPECT_TRUE(state.cn_passes_os());
  EXPECT_FALSE(state.cn_passes_os_port());
  EXPECT_TRUE(state.cn_passes_os_star());
  EXPECT_TRUE(state.os_crosses_cn());
  EXPECT_FALSE(state.os_crosses_cn_stern());
  EXPECT_TRUE(state.os_crosses_cn_bow());
  EXPECT_DOUBLE_EQ(state.os_crosses_cn_bow_dist(), 27.5);
  EXPECT_TRUE(state.cn_crosses_os());
  EXPECT_FALSE(state.cn_crosses_os_bow());
  EXPECT_TRUE(state.cn_crosses_os_stern());
  EXPECT_DOUBLE_EQ(state.cn_crosses_os_bow_dist(), 13.25);
  EXPECT_DOUBLE_EQ(state.os_curr_cpa_dist(), 4.5);
}
