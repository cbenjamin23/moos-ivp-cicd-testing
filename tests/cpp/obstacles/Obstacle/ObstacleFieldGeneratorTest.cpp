#include <gtest/gtest.h>

#include <cstdlib>
#include <vector>

#include "ObstacleFieldGenerator.h"
#include "XYFormatUtilsPoly.h"
#include "XYPolygon.h"

namespace {

class TestableObstacleFieldGenerator : public ObstacleFieldGenerator {
 public:
  using ObstacleFieldGenerator::generateObstacle;

  unsigned int amount() const { return m_amount; }
  unsigned int beginID() const { return m_begin_id; }
  unsigned int genTries() const { return m_gen_tries; }
  unsigned int polyVertices() const { return m_poly_vertices; }
  double minPolySize() const { return m_min_poly_size; }
  double maxPolySize() const { return m_max_poly_size; }
  double minRange() const { return m_min_range; }
  double precision() const { return m_precision; }
};

XYPolygon region()
{
  XYPolygon poly = string2Poly("pts={0,0:250,0:250,250:0,250},label=field");
  EXPECT_TRUE(poly.is_convex());
  return poly;
}

}  // namespace

// Covers obstacle field generator behavior: validates configuration setters.
TEST(ObstacleFieldGeneratorTest, ValidatesConfigurationSetters)
{
  TestableObstacleFieldGenerator generator;
  EXPECT_FALSE(generator.setAmount("0"));
  EXPECT_FALSE(generator.setAmount("-1"));
  EXPECT_FALSE(generator.setAmount("many"));
  EXPECT_TRUE(generator.setAmount("2"));

  EXPECT_FALSE(generator.setMinRange("-1"));
  EXPECT_TRUE(generator.setMinRange("12.5"));
  EXPECT_FALSE(generator.setObstacleMinSize("0"));
  EXPECT_FALSE(generator.setObstacleMaxSize("0"));
  EXPECT_FALSE(generator.setPrecision(0));
  EXPECT_FALSE(generator.setPrecision(1000));
  EXPECT_TRUE(generator.setPrecision(0.5));
  EXPECT_FALSE(generator.setPolyVertices(2));
  EXPECT_TRUE(generator.setPolyVertices(6));
  EXPECT_FALSE(generator.setGenTries("-1"));
  EXPECT_TRUE(generator.setGenTries("20"));
}

// Covers obstacle field generator behavior: adjusts min and max obstacle size bounds.
TEST(ObstacleFieldGeneratorTest, AdjustsMinAndMaxObstacleSizeBounds)
{
  TestableObstacleFieldGenerator generator;
  EXPECT_TRUE(generator.setObstacleMinSize(12));
  EXPECT_DOUBLE_EQ(generator.minPolySize(), 12);
  EXPECT_DOUBLE_EQ(generator.maxPolySize(), 12);

  EXPECT_TRUE(generator.setObstacleMaxSize(8));
  EXPECT_DOUBLE_EQ(generator.minPolySize(), 8);
  EXPECT_DOUBLE_EQ(generator.maxPolySize(), 8);

  EXPECT_TRUE(generator.setObstacleMaxSize(14));
  EXPECT_DOUBLE_EQ(generator.minPolySize(), 8);
  EXPECT_DOUBLE_EQ(generator.maxPolySize(), 14);
}

// Covers obstacle field generator behavior: pins current string poly vertices quirk.
TEST(ObstacleFieldGeneratorTest, PinsCurrentStringPolyVerticesQuirk)
{
  TestableObstacleFieldGenerator generator;
  EXPECT_EQ(generator.polyVertices(), 8u);

  // The string overload currently routes through the max-size setter, while the
  // numeric overload updates the polygon vertex count.
  EXPECT_TRUE(generator.setPolyVertices("5"));
  EXPECT_EQ(generator.polyVertices(), 8u);
  EXPECT_DOUBLE_EQ(generator.maxPolySize(), 5);

  EXPECT_TRUE(generator.setPolyVertices(5));
  EXPECT_EQ(generator.polyVertices(), 5u);
}

// Covers obstacle field generator behavior: generates contained labeled obstacle polygons.
TEST(ObstacleFieldGeneratorTest, GeneratesContainedLabeledObstaclePolygons)
{
  std::srand(1);
  TestableObstacleFieldGenerator generator;
  generator.setVerbose(false);
  generator.setBeginID(7);
  ASSERT_TRUE(generator.setPolygon(region()));
  ASSERT_TRUE(generator.setObstacleMinSize(10));
  ASSERT_TRUE(generator.setObstacleMaxSize(10));
  ASSERT_TRUE(generator.setMinRange(20));
  ASSERT_TRUE(generator.setPrecision(1));
  ASSERT_TRUE(generator.setPolyVertices(6));

  ASSERT_TRUE(generator.generateObstacle(5000));
  ASSERT_TRUE(generator.generateObstacle(5000));
  std::vector<XYPolygon> obstacles = generator.getObstacles();
  ASSERT_EQ(obstacles.size(), 2u);

  EXPECT_EQ(obstacles[0].get_label(), "ob_7");
  EXPECT_EQ(obstacles[1].get_label(), "ob_8");
  EXPECT_EQ(obstacles[0].size(), 6u);
  EXPECT_EQ(obstacles[1].size(), 6u);
  EXPECT_TRUE(region().contains(obstacles[0]));
  EXPECT_TRUE(region().contains(obstacles[1]));
  EXPECT_FALSE(obstacles[0].intersects(obstacles[1]));
  EXPECT_GE(obstacles[0].dist_to_poly(obstacles[1]), 20);
}

// Covers obstacle field generator behavior: fails generation without convex region.
TEST(ObstacleFieldGeneratorTest, FailsGenerationWithoutConvexRegion)
{
  TestableObstacleFieldGenerator generator;
  generator.setVerbose(false);
  EXPECT_FALSE(generator.generate());

  XYPolygon concave;
  concave.add_vertex(0, 0, false);
  concave.add_vertex(10, 0, false);
  concave.add_vertex(5, 5, false);
  concave.add_vertex(10, 10, false);
  concave.add_vertex(0, 10, false);
  concave.determine_convexity();
  ASSERT_FALSE(concave.is_convex());
  EXPECT_FALSE(generator.setPolygon(concave));
}

// Covers obstacle field generator behavior: pins direct setters and zero amount generation.
TEST(ObstacleFieldGeneratorTest, PinsDirectSettersAndZeroAmountGeneration)
{
  TestableObstacleFieldGenerator generator;
  generator.setVerbose(false);
  // Direct setters allow zero values that string configuration rejects, and
  // zero requested obstacles is a successful empty generation.
  generator.setAmount(0);
  generator.setBeginID(42);
  EXPECT_EQ(generator.amount(), 0u);
  EXPECT_EQ(generator.beginID(), 42u);
  EXPECT_TRUE(generator.setMinRange(0));
  EXPECT_DOUBLE_EQ(generator.minRange(), 0);
  EXPECT_TRUE(generator.setGenTries(0));
  EXPECT_EQ(generator.genTries(), 0u);
  EXPECT_TRUE(generator.setPrecision(0.25));
  EXPECT_DOUBLE_EQ(generator.precision(), 0.25);

  ASSERT_TRUE(generator.setPolygon(region()));
  EXPECT_TRUE(generator.generate());
  EXPECT_TRUE(generator.getObstacles().empty());
}

// Covers obstacle field generator behavior: parses polygon specs and fails when too constrained.
TEST(ObstacleFieldGeneratorTest, ParsesPolygonSpecsAndFailsWhenTooConstrained)
{
  std::srand(3);
  TestableObstacleFieldGenerator generator;
  generator.setVerbose(false);
  ASSERT_TRUE(generator.setPolygon("pts={0,0:20,0:20,20:0,20},label=tiny"));
  ASSERT_TRUE(generator.setObstacleMinSize(30));
  ASSERT_TRUE(generator.setObstacleMaxSize(30));
  ASSERT_TRUE(generator.setMinRange(0));
  ASSERT_TRUE(generator.setPolyVertices(8));

  EXPECT_FALSE(generator.generateObstacle(2));
  EXPECT_TRUE(generator.getObstacles().empty());
}
