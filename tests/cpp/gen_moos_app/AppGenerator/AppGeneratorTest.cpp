#include <gtest/gtest.h>

#include <string>

#include "AppGenerator.h"

namespace {

class TestableAppGenerator : public AppGenerator {
 public:
  bool appcasting() const { return m_appcasting; }
  const std::string& prefix() const { return m_prefix; }
  const std::string& body() const { return m_body; }
  const std::string& name() const { return m_name; }
  const std::string& date() const { return m_date; }
  const std::string& org() const { return m_org; }
};

}  // namespace

// Source audit note: AppGenerator is currently an upstream stub in
// app_gen_moos_app: setters are inline no-ops and generate() always returns
// true. These tests pin that intentionally small direct surface so future real
// generator behavior will require corresponding coverage instead of silently
// inheriting smoke-test assumptions.

// Covers constructor defaults and generate() return value.
TEST(AppGeneratorTest, GenerateReturnsTrueWithDefaultState)
{
  TestableAppGenerator generator;

  EXPECT_FALSE(generator.appcasting());
  EXPECT_TRUE(generator.generate());
}

// Covers current no-op setter behavior.
TEST(AppGeneratorTest, SettersDoNotMutateStubState)
{
  TestableAppGenerator generator;
  generator.setPrefix("p");
  generator.setBody("body");
  generator.setName("name");
  generator.setDate("date");
  generator.setOrg("org");

  EXPECT_EQ(generator.prefix(), "");
  EXPECT_EQ(generator.body(), "");
  EXPECT_EQ(generator.name(), "");
  EXPECT_EQ(generator.date(), "");
  EXPECT_EQ(generator.org(), "");
}
