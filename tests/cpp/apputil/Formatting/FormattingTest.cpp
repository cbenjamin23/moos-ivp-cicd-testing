#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ACBlock.h"
#include "ACTable.h"

// Covers ac table behavior: builds MOOS style status tables with headers and margins.
TEST(ACTableTest, BuildsMoosStyleStatusTablesWithHeadersAndMargins)
{
  ACTable table(3, 2);
  table << "App" << "Node" << "Warnings";
  table.addHeaderLines();
  table << "pHelmIvP" << "abe" << 2;
  table << "pMarinePIDV22" << "ben" << 0;
  table.setLeftMargin("  ");

  std::vector<std::string> rows = table.getTableOutput();
  ASSERT_EQ(rows.size(), 4u);
  EXPECT_EQ(rows[0], "  App            Node  Warnings");
  EXPECT_EQ(rows[1], "  -------------  ----  --------");
  EXPECT_EQ(rows[2], "  pHelmIvP       abe   2       ");
  EXPECT_EQ(rows[3], "  pMarinePIDV22  ben   0       ");

  EXPECT_EQ(table.getFormattedString(),
            "  App            Node  Warnings\n"
            "  -------------  ----  --------\n"
            "  pHelmIvP       abe   2\n"
            "  pMarinePIDV22  ben   0");
}

// Covers ac table behavior: splits pipe delimited cells and honors ignored cells.
TEST(ACTableTest, SplitsPipeDelimitedCellsAndHonorsIgnoredCells)
{
  ACTable table(2, 1);
  table << "Var|Value";
  table.addHeaderLines(3);
  table << "DEPLOY" << "true";
  table << "_ignore_" << "unused";

  std::vector<std::string> rows = table.getTableOutput();
  ASSERT_EQ(rows.size(), 3u);
  EXPECT_EQ(rows[0], "Var    Value");
  EXPECT_EQ(rows[1], "--- ---");
  EXPECT_EQ(rows[2], "DEPLOY true ");
}

// Covers ac table behavior: applies width limits justification and custom padding.
TEST(ACTableTest, AppliesWidthLimitsJustificationAndCustomPadding)
{
  ACTable table(3, 1);
  table.setColumnJustify(1, "right");
  table.setColumnJustify(2, "center");
  table.setColumnMaxWidth(0, 8);
  table.setColumnPadStr(0, " | ");
  table.setColumnNoPad(2);
  table << "Contact" << "Range" << "Mode";
  table.addHeaderLines();
  table << "very_long_vehicle_name" << 17.25 << "track";

  std::vector<std::string> rows = table.getTableOutput();
  ASSERT_EQ(rows.size(), 3u);
  EXPECT_EQ(rows[0], "Contact  | Range Mode");
  EXPECT_EQ(rows[1], "-------- | ----- -----");
  EXPECT_EQ(rows[2], "ver..ame | 17.25 track");
}

// Covers ac table behavior: supports colored cells without corrupting column widths.
TEST(ACTableTest, SupportsColoredCellsWithoutCorruptingColumnWidths)
{
  ACTable table(2, 1);
  table << "Source" << "State";
  table.addHeaderLines();
  table.addCell("pProcessWatch");
  table.addCell("OK", "green");

  std::string rendered = table.getFormattedString(false);
  EXPECT_NE(rendered.find("pProcessWatch"), std::string::npos);
  EXPECT_NE(rendered.find("OK"), std::string::npos);
  EXPECT_NE(rendered.find("\033["), std::string::npos);
}

// Covers ac table behavior: drops partial final rows.
TEST(ACTableTest, DropsPartialFinalRows)
{
  ACTable table(3, 1);
  table << "Var" << "Value" << "Source";
  table << "DEPLOY" << "true";
  table << "RETURN" << "false" << "pHelmIvP";

  std::vector<std::string> rows = table.getTableOutput();
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_EQ(rows[0], "Var    Value    Source");
  EXPECT_EQ(rows[1], "DEPLOY true     RETURN");
}

// Covers ac table behavior: ignores out of range configuration and supports zero columns.
TEST(ACTableTest, IgnoresOutOfRangeConfigurationAndSupportsZeroColumns)
{
  ACTable table(2, 2);
  table.setColumnJustify(99, "right");
  table.setColumnMaxWidth(99, 1);
  table.setColumnPadStr(99, " | ");
  table.setColumnNoPad(99);
  table << "App" << "Status" << "pHelmIvP" << "OK";

  EXPECT_EQ(table.getFormattedString(),
            "App       Status\n"
            "pHelmIvP  OK");

  ACTable empty(0, 2);
  empty << "ignored";
  EXPECT_TRUE(empty.getTableOutput().empty());
  EXPECT_EQ(empty.getFormattedString(), "");
}

// Covers ac block behavior: formats mission info payloads across wrapped lines.
TEST(ACBlockTest, FormatsMissionInfoPayloadsAcrossWrappedLines)
{
  ACBlock block("PHI_HOST_INFO: ",
                "community=abe,hostip=localhost,port_db=9001,port_udp=9201,timewarp=2",
                42);

  std::vector<std::string> lines = block.getFormattedLines();
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "PHI_HOST_INFO: community=abe,hostip=localhost,");
  EXPECT_EQ(lines[1], "               port_db=9001,port_udp=9201,timewarp=2");
  EXPECT_EQ(block.getFormattedString(),
            "PHI_HOST_INFO: community=abe,hostip=localhost,\n"
            "               port_db=9001,port_udp=9201,timewarp=2\n");
}

// Covers ac block behavior: handles empty messages separators and color validation.
TEST(ACBlockTest, HandlesEmptyMessagesSeparatorsAndColorValidation)
{
  ACBlock empty("Warnings: ", "", 20);
  EXPECT_EQ(empty.getFormattedLines(), (std::vector<std::string>{"Warnings: "}));
  EXPECT_EQ(empty.getFormattedString(), "Warnings: ");

  ACBlock block("Events: ", "one|two|three", 10, '|');
  EXPECT_EQ(block.getFormattedLines(),
            (std::vector<std::string>{"Events: one|two,",
                                      "        three"}));

  EXPECT_TRUE(block.setColor("red"));
  EXPECT_EQ(block.getColor(), "red");
  EXPECT_FALSE(block.setColor("not_a_term_color"));
  EXPECT_EQ(block.getColor(), "red");
}

// Covers ac block behavior: wraps quoted commas as single fields.
TEST(ACBlockTest, WrapsQuotedCommasAsSingleFields)
{
  ACBlock block("NODE_REPORT: ",
                "NAME=abe,NOTE=\"hold,station\",MODE=ACTIVE:SURVEYING,SPD=2.1",
                34);

  EXPECT_EQ(block.getFormattedLines(),
            (std::vector<std::string>{
                "NODE_REPORT: NAME=abe,NOTE=\"hold,station\",",
                "             MODE=ACTIVE:SURVEYING,SPD=2.1"}));
}
