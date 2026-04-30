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

// Covers string tree behavior: builds printable mode tree from helm mode set.
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

// Covers string tree behavior: builds tree from helm scope mode set message.
TEST(StringTreeTest, BuildsTreeFromHelmScopeModeSetMessage)
{
  const std::string modeset =
      "MODE#---,ACTIVE#ACTIVE,SURVEYING#SURVEYING,STATION_KEEP#"
      "ACTIVE,RETURNING#RETURNING,DOCKING#---,INACTIVE";

  StringTree tree;
  std::vector<std::string> entries = {
      "MODE", "---,ACTIVE", "ACTIVE,SURVEYING", "SURVEYING,STATION_KEEP",
      "ACTIVE,RETURNING", "RETURNING,DOCKING", "---,INACTIVE"};
  for(unsigned int i = 0; i < entries.size(); ++i) {
    std::string entry = entries[i];
    std::string parent = entry.substr(0, entry.find(','));
    std::string child = (entry.find(',') == std::string::npos)
                            ? ""
                            : entry.substr(entry.find(',') + 1);
    if(child.empty())
      tree.setKey(parent);
    else
      EXPECT_TRUE(tree.addParChild(parent, child)) << modeset;
  }

  EXPECT_TRUE(tree.allHandled());
  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "    SURVEYING",
                  "        STATION_KEEP", "    RETURNING", "        DOCKING",
                  "INACTIVE"});
}

// Covers string tree behavior: pins waiting child behavior for out of order mode sets.
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

// Covers string tree behavior: can defer waiter processing for batch construction.
TEST(StringTreeTest, CanDeferWaiterProcessingForBatchConstruction)
{
  StringTree tree;
  tree.setKey("MODE");

  EXPECT_FALSE(tree.addParChild("SURVEYING", "LOITERING", false));
  EXPECT_TRUE(tree.addParChild("---", "ACTIVE", false));
  EXPECT_TRUE(tree.addParChild("ACTIVE", "SURVEYING", false));
  EXPECT_FALSE(tree.allHandled());
  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "    SURVEYING"});

  EXPECT_TRUE(tree.addParChild("ACTIVE", "RETURNING"));
  EXPECT_TRUE(tree.allHandled());
  expectVectorEq(tree.getPrintableSet(),
                 {"Mode-Variable=MODE", "ACTIVE", "    SURVEYING",
                  "        LOITERING", "    RETURNING"});
}

// Covers string tree behavior: builds multi level out of order helm mode tree.
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

// Covers string tree behavior: pins duplicate roots and unresolved parents.
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

// Covers string tree behavior: emits graphviz with sanitized mode names.
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
