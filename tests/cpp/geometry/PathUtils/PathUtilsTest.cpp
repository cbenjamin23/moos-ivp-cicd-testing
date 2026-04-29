#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "EdgeTag.h"
#include "EdgeTagSet.h"
#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "PathUtils.h"
#include "XYFormatUtilsSegl.h"
#include "XYSegList.h"

namespace {

XYSegList makeOutOfOrderWaypointLane()
{
  XYSegList lane;
  lane.set_label("survey_lane");
  lane.add_vertex(0, 20, 11, "north_gate");
  lane.add_vertex(10, 0, 12, "east_gate");
  lane.add_vertex(11, 0, 13, "east_gate_2");
  lane.add_vertex(0, 21, 14, "north_gate_2");
  return lane;
}

void expectVertex(const XYSegList& segl,
                  unsigned int ix,
                  double x,
                  double y,
                  const std::string& vprop)
{
  ASSERT_LT(ix, segl.size());
  EXPECT_NEAR(segl.get_vx(ix), x, kGeomTol);
  EXPECT_NEAR(segl.get_vy(ix), y, kGeomTol);
  EXPECT_EQ(segl.get_vprop(ix), vprop);
}

}  // namespace

TEST(PathUtilsGreedyPathTest, ReordersWaypointLaneByNearestNeighborProgression)
{
  XYSegList lane = makeOutOfOrderWaypointLane();

  XYSegList ordered = greedyPath(lane, 0, 0);

  ASSERT_EQ(ordered.size(), 4u);
  expectVertex(ordered, 0, 10, 0, "east_gate");
  expectVertex(ordered, 1, 11, 0, "east_gate_2");
  expectVertex(ordered, 2, 0, 20, "north_gate");
  expectVertex(ordered, 3, 0, 21, "north_gate_2");

  EXPECT_NEAR(ordered.length(), 1.0 + std::hypot(11.0, 20.0) + 1.0, kGeomTol);
}

TEST(PathUtilsGreedyPathTest, UsesCurrentChosenVertexAsNextSearchOrigin)
{
  XYSegList route;
  route.add_vertex(0, 8, 0, "nearest_to_start");
  route.add_vertex(100, 8, 0, "far_from_second");
  route.add_vertex(0, 9, 0, "nearest_after_first");

  XYSegList ordered = greedyPath(route, 0, 0);

  ASSERT_EQ(ordered.size(), 3u);
  expectVertex(ordered, 0, 0, 8, "nearest_to_start");
  expectVertex(ordered, 1, 0, 9, "nearest_after_first");
  expectVertex(ordered, 2, 100, 8, "far_from_second");
}

TEST(PathUtilsGreedyPathTest, PreservesDuplicateWaypointsAsDistinctVisits)
{
  XYSegList route;
  route.add_vertex(5, 0, 0, "entry");
  route.add_vertex(5, 0, 0, "loiter_start");
  route.add_vertex(10, 0, 0, "exit");

  XYSegList ordered = greedyPath(route, 0, 0);

  ASSERT_EQ(ordered.size(), 3u);
  expectVertex(ordered, 0, 5, 0, "entry");
  expectVertex(ordered, 1, 5, 0, "loiter_start");
  expectVertex(ordered, 2, 10, 0, "exit");
}

TEST(PathUtilsGreedyPathTest, ReordersBhvWaypointMissionPtsFromCurrentOwnshipPosition)
{
  XYSegList route = string2SegList("pts={60,-40:60,-160:150,-160:150,-40}");

  ASSERT_TRUE(route.valid());
  XYSegList ordered = greedyPath(route, 148, -42);

  ASSERT_EQ(ordered.size(), 4u);
  expectVertex(ordered, 0, 150, -40, "");
  expectVertex(ordered, 1, 60, -40, "");
  expectVertex(ordered, 2, 60, -160, "");
  expectVertex(ordered, 3, 150, -160, "");
  EXPECT_NEAR(ordered.length(), 90.0 + 120.0 + 90.0, kGeomTol);
}

TEST(PathUtilsGreedyPathTest, KeepsAlreadyOrderedColregsTransitRouteFromStartPoint)
{
  XYSegList route = string2SegList("pts={190,25:155,-20:45,-85:-40,-100}");

  ASSERT_TRUE(route.valid());
  XYSegList ordered = greedyPath(route, 190, 25);

  ASSERT_EQ(ordered.size(), 4u);
  expectVertex(ordered, 0, 190, 25, "");
  expectVertex(ordered, 1, 155, -20, "");
  expectVertex(ordered, 2, 45, -85, "");
  expectVertex(ordered, 3, -40, -100, "");
}

TEST(PathUtilsGreedyPathTest, BreaksEqualDistanceTiesByOriginalVertexOrder)
{
  XYSegList route;
  route.add_vertex(1, 0, 0, "east");
  route.add_vertex(-1, 0, 0, "west");
  route.add_vertex(0, 1, 0, "north");

  XYSegList ordered = greedyPath(route, 0, 0);

  ASSERT_EQ(ordered.size(), 3u);
  expectVertex(ordered, 0, 1, 0, "east");
  expectVertex(ordered, 1, 0, 1, "north");
  expectVertex(ordered, 2, -1, 0, "west");
}

TEST(PathUtilsGreedyPathTest, HandlesEmptyAndSingleVertexRoutes)
{
  XYSegList empty;
  XYSegList empty_ordered = greedyPath(empty, 10, 10);
  EXPECT_EQ(empty_ordered.size(), 0u);
  EXPECT_FALSE(empty_ordered.valid());

  XYSegList one;
  one.add_vertex(42, -7, 99, "only_waypoint");
  XYSegList one_ordered = greedyPath(one, 0, 0);

  ASSERT_EQ(one_ordered.size(), 1u);
  expectVertex(one_ordered, 0, 42, -7, "only_waypoint");
}

TEST(PathUtilsGreedyPathTest, CurrentBehaviorDropsRouteMetadataAndVertexZValues)
{
  XYSegList lane = makeOutOfOrderWaypointLane();
  EdgeTagSet tags;
  ASSERT_TRUE(tags.addEdgeTag(EdgeTag(0, 1, "survey_edge")));
  lane.set_edge_tags(tags);

  XYSegList ordered = greedyPath(lane, 0, 0);

  ASSERT_EQ(ordered.size(), 4u);
  EXPECT_EQ(ordered.get_label(), "");
  EXPECT_EQ(ordered.get_edge_tags().size(), 0u);
  for(unsigned int i = 0; i < ordered.size(); ++i)
    EXPECT_NEAR(ordered.get_vz(i), 0.0, kGeomTol);
}

TEST(EdgeTagTest, DefaultTagIsInvalidUntilAdjacentIndicesAndTagAreSet)
{
  EdgeTag tag;

  EXPECT_FALSE(tag.valid());
  EXPECT_FALSE(tag.setIndices(2, 4));
  EXPECT_FALSE(tag.valid());

  EXPECT_TRUE(tag.setIndices(2, 3));
  EXPECT_FALSE(tag.valid());

  tag.setTag("boundary");
  EXPECT_TRUE(tag.valid());
  EXPECT_EQ(tag.getSpec(), "2:3:boundary");
}

TEST(EdgeTagTest, ConstructsSerializesAndMatchesUndirectedAdjacentEdges)
{
  EdgeTag tag(7, 6, "no_cross");

  ASSERT_TRUE(tag.valid());
  EXPECT_EQ(tag.getIndex1(), 7u);
  EXPECT_EQ(tag.getIndex2(), 6u);
  EXPECT_EQ(tag.getTag(), "no_cross");
  EXPECT_EQ(tag.getSpec(), "7:6:no_cross");
  EXPECT_TRUE(tag.matches(7, 6));
  EXPECT_TRUE(tag.matches(6, 7));
  EXPECT_TRUE(tag.matches(6, 7, "no_cross"));
  EXPECT_FALSE(tag.matches(6, 7, "preferred"));
  EXPECT_FALSE(tag.matches(6, 8));
}

TEST(EdgeTagTest, RejectsNonAdjacentIndexUpdatesWithoutChangingValidTag)
{
  EdgeTag tag(4, 5, "lane");
  ASSERT_TRUE(tag.valid());

  EXPECT_FALSE(tag.setIndices(1, 3));
  EXPECT_TRUE(tag.valid());
  EXPECT_TRUE(tag.matches(4, 5, "lane"));
  EXPECT_FALSE(tag.matches(1, 3, "lane"));
}

TEST(EdgeTagTest, ParsesTrimmedSpecsAndRejectsMalformedInputs)
{
  EdgeTag tag;

  EXPECT_TRUE(tag.setOnSpec(" 12 : 13 : loiter_boundary "));
  EXPECT_TRUE(tag.valid());
  EXPECT_EQ(tag.getIndex1(), 12u);
  EXPECT_EQ(tag.getIndex2(), 13u);
  EXPECT_EQ(tag.getTag(), "loiter_boundary");

  EXPECT_FALSE(tag.setOnSpec("12:14:skip_edge"));
  EXPECT_FALSE(tag.setOnSpec("12:13:"));
  EXPECT_FALSE(tag.setOnSpec("a:13:bad"));
  EXPECT_FALSE(tag.setOnSpec("-1:0:bad"));
  EXPECT_FALSE(tag.setOnSpec("12:13"));
}

TEST(EdgeTagTest, CurrentParserIgnoresFieldsAfterTheThirdColonToken)
{
  EdgeTag tag;

  EXPECT_TRUE(tag.setOnSpec("1:2:gate:ignored"));
  EXPECT_TRUE(tag.valid());
  EXPECT_EQ(tag.getSpec(), "1:2:gate");
}

TEST(EdgeTagSetTest, AddsOnlyValidTagsAndSerializesInInsertionOrder)
{
  EdgeTagSet tags;
  EdgeTag invalid;
  invalid.setTag("not_adjacent");

  EXPECT_FALSE(tags.addEdgeTag(invalid));
  EXPECT_EQ(tags.size(), 0u);
  EXPECT_EQ(tags.getSpec(), "");

  EXPECT_TRUE(tags.addEdgeTag(EdgeTag(0, 1, "entry")));
  EXPECT_TRUE(tags.addEdgeTag(EdgeTag(3, 2, "exit")));

  EXPECT_EQ(tags.size(), 2u);
  EXPECT_EQ(tags.getSpec(), "tags={0:1:entry#3:2:exit}");
  EXPECT_TRUE(tags.matches(1, 0, "entry"));
  EXPECT_TRUE(tags.matches(2, 3, "exit"));
  EXPECT_FALSE(tags.matches(1, 0, "exit"));
}

TEST(EdgeTagSetTest, ParsesAllSupportedSpecWrappersWhileReturningFalseOnSuccess)
{
  EdgeTagSet tags;

  EXPECT_FALSE(tags.setOnSpec("tags={2:3:end#4:5:start}"));
  ASSERT_EQ(tags.size(), 2u);
  EXPECT_TRUE(tags.matches(3, 2, "end"));
  EXPECT_TRUE(tags.matches(5, 4, "start"));

  EXPECT_FALSE(tags.setOnSpec("{6:7:survey}"));
  EXPECT_EQ(tags.size(), 3u);
  EXPECT_TRUE(tags.matches(6, 7, "survey"));

  EXPECT_FALSE(tags.setOnSpec("8:9:gate"));
  EXPECT_EQ(tags.size(), 4u);
  EXPECT_TRUE(tags.matches(9, 8, "gate"));
}

TEST(EdgeTagSetTest, MalformedBatchDoesNotPartiallyMergeEarlierValidTags)
{
  EdgeTagSet tags;
  ASSERT_TRUE(tags.addEdgeTag(EdgeTag(0, 1, "existing")));

  EXPECT_FALSE(tags.setOnSpec("2:3:valid#4:6:not_adjacent"));

  EXPECT_EQ(tags.size(), 1u);
  EXPECT_TRUE(tags.matches(0, 1, "existing"));
  EXPECT_FALSE(tags.matches(2, 3, "valid"));
}

TEST(EdgeTagSetTest, AppendsParsedTagsInsteadOfReplacingExistingTags)
{
  EdgeTagSet tags;
  ASSERT_TRUE(tags.addEdgeTag(EdgeTag(0, 1, "entry")));

  EXPECT_FALSE(tags.setOnSpec("tags={1:2:middle#2:3:exit}"));

  EXPECT_EQ(tags.size(), 3u);
  EXPECT_EQ(tags.getSpec(), "tags={0:1:entry#1:2:middle#2:3:exit}");
}

TEST(EdgeTagSetTest, RoundTripsThroughXYSegListSpecsForTaggedRegionBoundaries)
{
  EdgeTagSet tags;
  ASSERT_TRUE(tags.addEdgeTag(EdgeTag(0, 1, "entry_lane")));
  ASSERT_TRUE(tags.addEdgeTag(EdgeTag(2, 3, "exit_lane")));

  XYSegList boundary;
  boundary.add_vertex(0, 0);
  boundary.add_vertex(20, 0);
  boundary.add_vertex(20, 10);
  boundary.add_vertex(0, 10);
  boundary.set_edge_tags(tags);

  EXPECT_TRUE(boundary.get_edge_tags().matches(1, 0, "entry_lane"));
  EXPECT_TRUE(boundary.get_edge_tags().matches(3, 2, "exit_lane"));

  std::string spec = boundary.get_spec();
  EXPECT_TRUE(stringContains(spec, "pts={0,0:20,0:20,10:0,10}"));
  EXPECT_TRUE(stringContains(spec, "tags={0:1:entry_lane#2:3:exit_lane}"));
}
