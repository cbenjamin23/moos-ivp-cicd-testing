#include <gtest/gtest.h>

#include <string>

#include "NodeMessage.h"
#include "NodeMessageUtils.h"

TEST(NodeMessageTest, BuildsValidNodeToNodeStringMessageSpecs)
{
  NodeMessage msg("abe", "ben", "RETURN_HOME");
  msg.setSourceApp("pHelmIvP");
  msg.setSourceBehavior("waypt_return");
  msg.setStringVal("true");
  msg.setColor("red");
  msg.setMessageID("ack-1");
  msg.setAckRequested(true);

  EXPECT_TRUE(msg.valid());
  EXPECT_EQ(msg.getSpec(),
            "ack_id=ack-1,ack=true,src_node=abe,dest_node=ben,"
            "src_app=pHelmIvP,src_bhv=waypt_return,var_name=RETURN_HOME,"
            "color=red,string_val=true");
  EXPECT_EQ(msg.getSpec(false),
            "src_node=abe,dest_node=ben,src_app=pHelmIvP,"
            "src_bhv=waypt_return,var_name=RETURN_HOME,color=red,"
            "string_val=true");
}

TEST(NodeMessageTest, QuotesCommaBearingStringValuesForTransmission)
{
  NodeMessage msg("abe", "ben", "SAY");
  msg.setStringVal("hold,station");

  EXPECT_TRUE(msg.valid());
  EXPECT_EQ(msg.getStringVal(), "\"hold,station\"");
  EXPECT_EQ(msg.getStringValX(), "hold,station");
  EXPECT_EQ(msg.getSpec(),
            "src_node=abe,dest_node=ben,var_name=SAY,"
            "string_val=\"hold,station\"");
}

TEST(NodeMessageTest, ParsesMoosStyleNodeMessageStrings)
{
  NodeMessage msg = string2NodeMessage(
      "src_node=abe,dest_group=blue,var_name=SAY,"
      "string_val=\"hold,station\",string_val_quoted=true,"
      "color=green,ack=true,ack_id=42");

  EXPECT_TRUE(msg.valid());
  EXPECT_EQ(msg.getSourceNode(), "abe");
  EXPECT_EQ(msg.getDestGroup(), "blue");
  EXPECT_EQ(msg.getVarName(), "SAY");
  EXPECT_EQ(msg.getStringVal(), "\"hold,station\"");
  EXPECT_EQ(msg.getStringValX(), "hold,station");
  EXPECT_EQ(msg.getColor(), "green");
  EXPECT_TRUE(msg.getAckRequested());
  EXPECT_EQ(msg.getMessageID(), "42");
}

TEST(NodeMessageTest, ParsesNumericMessagesAndRejectsMissingRequiredFields)
{
  NodeMessage numeric = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=DESIRED_SPEED,double_val=2.5");
  EXPECT_TRUE(numeric.valid());
  EXPECT_EQ(numeric.getSourceNode(), "abe");
  EXPECT_EQ(numeric.getDestNode(), "ben");
  EXPECT_EQ(numeric.getVarName(), "DESIRED_SPEED");
  EXPECT_DOUBLE_EQ(numeric.getDoubleVal(), 2.5);

  EXPECT_FALSE(string2NodeMessage(
      "src_node=abe,var_name=DESIRED_SPEED,double_val=2.5").valid());
  EXPECT_FALSE(string2NodeMessage(
      "dest_node=ben,var_name=DESIRED_SPEED,double_val=2.5").valid());
  EXPECT_FALSE(string2NodeMessage(
      "src_node=abe,dest_node=ben,double_val=2.5").valid());
}

TEST(NodeMessageTest, SupportsGroupDestinationsAndPayloadLengthAccounting)
{
  NodeMessage group_msg("abe", "", "NODE_MESSAGE_LOCAL");
  group_msg.setDestGroup("blue_team");
  group_msg.setStringVal("survey");

  EXPECT_TRUE(group_msg.valid());
  EXPECT_EQ(group_msg.length(), 6u);
  EXPECT_EQ(group_msg.getSpec(),
            "src_node=abe,dest_group=blue_team,var_name=NODE_MESSAGE_LOCAL,"
            "string_val=survey");

  NodeMessage double_msg("abe", "ben", "DESIRED_HEADING");
  double_msg.setDoubleVal(270);
  EXPECT_TRUE(double_msg.valid());
  EXPECT_EQ(double_msg.length(), 2u);
  EXPECT_EQ(double_msg.getSpec(),
            "src_node=abe,dest_node=ben,var_name=DESIRED_HEADING,"
            "double_val=270");
}

TEST(NodeMessageTest, PinsParserQuirksForQuotedAndMalformedPayloads)
{
  NodeMessage raw_quoted = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=SAY,string_val=\"hold,station\"");
  EXPECT_TRUE(raw_quoted.valid());
  EXPECT_EQ(raw_quoted.getStringVal(), "\"hold,station\"");
  EXPECT_EQ(raw_quoted.getSpec(),
            "src_node=abe,dest_node=ben,var_name=SAY,"
            "string_val=\"hold,station\"");

  NodeMessage invalid_double = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=SPD,double_val=fast");
  EXPECT_TRUE(invalid_double.valid());
  EXPECT_DOUBLE_EQ(invalid_double.getDoubleVal(), 0);

  NodeMessage no_payload("abe", "ben", "PING");
  EXPECT_TRUE(no_payload.valid());
  EXPECT_EQ(no_payload.getSpec(), "src_node=abe,dest_node=ben,var_name=PING");
}

TEST(NodeMessageTest, IgnoresInvalidColorsAndRejectsConflictingPayloadTypes)
{
  NodeMessage msg("abe", "ben", "FOO");
  msg.setColor("not_a_color");
  msg.setStringVal("bar");
  EXPECT_TRUE(msg.valid());
  EXPECT_EQ(msg.getColor(), "");

  msg.setDoubleVal(3.0);
  EXPECT_FALSE(msg.valid());
}
