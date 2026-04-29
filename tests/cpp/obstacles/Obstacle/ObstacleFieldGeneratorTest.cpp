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

  unsigned int polyVertices() const { return m_poly_vertices; }
  double minPolySize() const { return m_min_poly_size; }
  double maxPolySize() const { return m_max_poly_size; }
};

XYPolygon region()
{
  XYPolygon poly = string2Poly("pts={0,0:250,0:250,250:0,250},label=field");
  EXPECT_TRUE(poly.is_convex());
  return poly;
}

}  // namespace

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

TEST(ObstacleFieldGeneratorTest, PinsCurrentStringPolyVerticesQuirk)
{
  TestableObstacleFieldGenerator generator;
  EXPECT_EQ(generator.polyVertices(), 8u);

  EXPECT_TRUE(generator.setPolyVertices("5"));
  EXPECT_EQ(generator.polyVertices(), 8u);
  EXPECT_DOUBLE_EQ(generator.maxPolySize(), 5);

  EXPECT_TRUE(generator.setPolyVertices(5));
  EXPECT_EQ(generator.polyVertices(), 5u);
}

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
