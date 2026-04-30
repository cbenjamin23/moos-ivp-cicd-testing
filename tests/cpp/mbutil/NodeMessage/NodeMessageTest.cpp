#include <gtest/gtest.h>

#include <string>

#include "NodeMessage.h"
#include "NodeMessageUtils.h"

// Covers node message behavior: builds valid node to node string message specs.
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

// Covers node message behavior: quotes comma bearing string values for transmission.
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

// Covers node message behavior: parses MOOS style node message strings.
TEST(NodeMessageTest, ParsesMoosStyleNodeMessageStrings)
{
  // NODE_MESSAGE payloads are comma-separated MOOS mail specs, and quoted
  // string values may contain commas that must stay inside the payload.
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

// Covers node message behavior: parses mission examples case insensitively and preserves values.
TEST(NodeMessageTest, ParsesMissionExamplesCaseInsensitivelyAndPreservesValues)
{
  NodeMessage msg = string2NodeMessage(
      "SRC_NODE=henry,DEST_NODE=ike,SRC_APP=uFldNodeComms,"
      "SRC_BHV=avoid_collision,VAR_NAME=RETURN,string_val=true,"
      "COLOR=yellow,ACK=TRUE,ACK_ID=ret-17,ignored=field");

  ASSERT_TRUE(msg.valid());
  EXPECT_EQ(msg.getSourceNode(), "henry");
  EXPECT_EQ(msg.getDestNode(), "ike");
  EXPECT_EQ(msg.getSourceApp(), "uFldNodeComms");
  EXPECT_EQ(msg.getSourceBehavior(), "avoid_collision");
  EXPECT_EQ(msg.getVarName(), "RETURN");
  EXPECT_EQ(msg.getStringVal(), "true");
  EXPECT_EQ(msg.getColor(), "yellow");
  EXPECT_TRUE(msg.getAckRequested());
  EXPECT_EQ(msg.getMessageID(), "ret-17");
  EXPECT_EQ(msg.getSpec(),
            "ack_id=ret-17,ack=true,src_node=henry,dest_node=ike,"
            "src_app=uFldNodeComms,src_bhv=avoid_collision,var_name=RETURN,"
            "color=yellow,string_val=true");
}

// Covers node message behavior: parses numeric messages and rejects missing required fields.
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

// Covers node message behavior: rejects conflicting payloads but allows dual destinations.
TEST(NodeMessageTest, RejectsConflictingPayloadsButAllowsDualDestinations)
{
  NodeMessage conflict = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=FOO,string_val=bar,double_val=3.5");
  EXPECT_FALSE(conflict.valid());
  EXPECT_EQ(conflict.getSpec(), "");

  NodeMessage dual_dest = string2NodeMessage(
      "src_node=abe,dest_node=ben,dest_group=blue,var_name=FOO,double_val=3.5");
  ASSERT_TRUE(dual_dest.valid());
  EXPECT_EQ(dual_dest.getSpec(),
            "src_node=abe,dest_node=ben,dest_group=blue,var_name=FOO,"
            "double_val=3.5");
}

// Covers node message behavior: supports group destinations and payload length accounting.
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

// Covers node message behavior: pins parser quirks for quoted and malformed payloads.
TEST(NodeMessageTest, PinsParserQuirksForQuotedAndMalformedPayloads)
{
  // Pin current parser behavior: raw quoted strings stay quoted, malformed
  // numeric payloads coerce to zero, and payload-less messages can be valid.
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

// Covers node message behavior: pins duplicate fields and quoted payload round trips.
TEST(NodeMessageTest, PinsDuplicateFieldsAndQuotedPayloadRoundTrips)
{
  // Later duplicate fields win, while string_val_quoted controls whether outer
  // quotes are stripped or retained for a comma-bearing transmission value.
  NodeMessage duplicate = string2NodeMessage(
      "src_node=abe,src_node=ben,dest_node=cal,var_name=SAY,"
      "string_val=first,string_val=second");
  ASSERT_TRUE(duplicate.valid());
  EXPECT_EQ(duplicate.getSourceNode(), "ben");
  EXPECT_EQ(duplicate.getStringVal(), "second");

  NodeMessage stripped = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=SAY,"
      "string_val=\"hold\",string_val_quoted=true");
  ASSERT_TRUE(stripped.valid());
  EXPECT_EQ(stripped.getStringVal(), "hold");
  EXPECT_EQ(stripped.getSpec(),
            "src_node=abe,dest_node=ben,var_name=SAY,string_val=hold");

  NodeMessage requoted = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=SAY,"
      "string_val=\"hold,station\",string_val_quoted=true");
  ASSERT_TRUE(requoted.valid());
  EXPECT_EQ(requoted.getStringVal(), "\"hold,station\"");
  EXPECT_EQ(requoted.getStringValX(), "hold,station");
  EXPECT_EQ(requoted.getSpec(),
            "src_node=abe,dest_node=ben,var_name=SAY,"
            "string_val=\"hold,station\"");
}

// Covers node message behavior: ignores invalid colors and rejects conflicting payload types.
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

// Covers node message behavior: pins ack serialization independent of ack requested.
TEST(NodeMessageTest, PinsAckSerializationIndependentOfAckRequested)
{
  NodeMessage msg("abe", "ben", "FOO");
  msg.setStringVal("bar");
  msg.setMessageID("ack-only");

  // A message id is serialized even when ack=false; getSpec(false) is the only
  // path that suppresses ack metadata.
  EXPECT_TRUE(msg.valid());
  EXPECT_FALSE(msg.getAckRequested());
  EXPECT_EQ(msg.getSpec(),
            "ack_id=ack-only,src_node=abe,dest_node=ben,var_name=FOO,"
            "string_val=bar");
  EXPECT_EQ(msg.getSpec(false),
            "src_node=abe,dest_node=ben,var_name=FOO,string_val=bar");

  NodeMessage parsed = string2NodeMessage(
      "src_node=abe,dest_node=ben,var_name=FOO,string_val=bar,"
      "ack=false,ack_id=ack-only");
  EXPECT_TRUE(parsed.valid());
  EXPECT_FALSE(parsed.getAckRequested());
  EXPECT_EQ(parsed.getMessageID(), "ack-only");
}
