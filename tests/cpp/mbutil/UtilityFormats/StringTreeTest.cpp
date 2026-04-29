#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "StringTree.h"

namespace {

void expectVectorEq(const std::vector<std::string>& actual,
                    const std::vector<std::string>& expected)
{
  ASSERT_EQ(actual.size(), expected.size());
  for(std::size_t i = 0; i < expected.size(); ++i)
    EXPECT_EQ(actual[i], expected[i]) << "at index " << i;
}

}  // namespace

TEST(StringTreeTest, BuildsPrintableModeTreeFromHelmModeSet)
{
  const std::string modeset =
      "---,ACTIVE#---,INACTIVE#ACTIVE,SURVEYING#ACTIVE,RETURNING";

  StringTree tree;
  tree.setKey("MODE");
  tree.addParChild("---", "ACTIVE");
  tree.addParChild("---", "INACTIVE");
  tree.addParChild("ACTIVE", "SURVEYING");
  tree.addParChild("ACTIVE", "RETURNING");

  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "    SURVEYING",
                  "    RETURNING", "INACTIVE"});
  EXPECT_TRUE(tree.allHandled()) << modeset;
}

TEST(StringTreeTest, PinsWaitingChildBehaviorForOutOfOrderModeSets)
{
  StringTree tree;
  tree.setKey("MODE");

  EXPECT_FALSE(tree.addParChild("ACTIVE", "SURVEYING"));
  EXPECT_FALSE(tree.allHandled());

  EXPECT_TRUE(tree.addParChild("---", "ACTIVE"));
  EXPECT_FALSE(tree.allHandled());
  expectVectorEq(tree.getPrintableSet(), {"Mode-Variable=MODE", "ACTIVE"});

  EXPECT_TRUE(tree.addParChild("ACTIVE", "RETURNING"));
  EXPECT_TRUE(tree.allHandled());
  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "    RETURNING",
                  "    SURVEYING"});
}

TEST(StringTreeTest, BuildsMultiLevelOutOfOrderHelmModeTree)
{
  StringTree tree;
  tree.setKey("MODE");

  EXPECT_FALSE(tree.addParChild("SURVEYING", "LOITERING"));
  EXPECT_FALSE(tree.addParChild("ACTIVE", "SURVEYING"));
  EXPECT_FALSE(tree.addParChild("RETURNING", "DOCKING"));
  EXPECT_TRUE(tree.addParChild("---", "ACTIVE"));
  EXPECT_TRUE(tree.addParChild("ACTIVE", "RETURNING"));

  EXPECT_TRUE(tree.allHandled());
  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "    RETURNING",
                  "        DOCKING", "    SURVEYING", "        LOITERING",
                  "        LOITERING"});
}

TEST(StringTreeTest, PinsDuplicateRootsAndUnresolvedParents)
{
  StringTree tree;
  tree.setKey("MODE");

  EXPECT_TRUE(tree.addParChild("---", "ACTIVE"));
  EXPECT_TRUE(tree.addParChild("", "ACTIVE"));
  EXPECT_FALSE(tree.addParChild("ACTIVE", "ACTIVE"));
  EXPECT_FALSE(tree.addParChild("MISSING", "ORPHAN"));
  EXPECT_FALSE(tree.allHandled());

  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "ACTIVE"});
}

TEST(StringTreeTest, EmitsGraphvizWithSanitizedModeNames)
{
  StringTree tree;
  tree.setKey("MODE");
  tree.addParChild("---", "TRANSIT-MODE");
  tree.addParChild("TRANSIT-MODE", "STATION-KEEP");

  std::ostringstream os;
  tree.writeGraphviz(os, "MODE [shape=box];");

  EXPECT_EQ(os.str(),
            "graph stringtree {\n  MODE [shape=box];\n"
            "root--TRANSIT_MODE--STATION_KEEP\n  }\n");
}
