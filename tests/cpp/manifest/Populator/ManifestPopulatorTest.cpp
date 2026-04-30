#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "Populator_ManifestSet.h"
#include "TestFileUtils.h"

namespace {

std::string writeFixture(const std::string& name, const std::string& content)
{
  static std::vector<std::unique_ptr<TempFile>> files;
  files.emplace_back(new TempFile(name, content));
  return files.back()->path();
}

}  // namespace

// Covers populator manifest set behavior: populates multi module MOOS manifest files.
TEST(PopulatorManifestSetTest, PopulatesMultiModuleMoosManifestFiles)
{
  const std::string manifest_path = writeFixture(
      "moos_ivp_manifest.mfs",
      "// comments are ignored\n"
      "module = lib_geometry\n"
      "type = library\n"
      "thumb = geometry.png\n"
      "contact = moos-ivp@mit.edu\n"
      "author = michael benjamin, henrik schmidt\n"
      "org = mit\n"
      "group = core, geometry\n"
      "synopsis = Geometry primitives and algorithms\n"
      "  used by behaviors and viewers.\n"
      "distro = moos-ivp\n"
      "borndate = 120101\n"
      "doc_url = https://oceanai.mit.edu/ivpman/pmwiki/pmwiki.php?n=IvPTools.Cover\n"
      "license = GPL\n"
      "\n"
      "module = pMarineViewer\n"
      "type = app\n"
      "thumb = viewer.png\n"
      "contact = moos-ivp@mit.edu\n"
      "author = michael benjamin\n"
      "org = mit\n"
      "group = viewer\n"
      "synopsis = Primary shoreside viewer for vehicles and VIEW_* posts.\n"
      "depends = lib_geometry, lib_marineview\n"
      "distro = moos-ivp\n"
      "borndate = 130101\n"
      "doc_url = https://oceanai.mit.edu/ivpman/apps/pMarineViewer\n");

  Populator_ManifestSet populator;
  populator.addManifestFile(manifest_path);
  ASSERT_TRUE(populator.populate());

  ManifestSet set = populator.getManifestSet();
  EXPECT_EQ(set.size(), 2u);
  EXPECT_EQ(set.getManifestByModule("lib_geometry").getType(), "library");
  EXPECT_EQ(set.getManifestByModule("pMarineViewer").getLibDepends(),
            (std::vector<std::string>{"lib_geometry", "lib_marineview"}));
  EXPECT_TRUE(set.getManifestByModule("lib_geometry").hasAuthor("Henrik"));
  EXPECT_NE(set.getManifestByModule("lib_geometry").getSynopsisStr().find("used by behaviors"),
            std::string::npos);
  EXPECT_EQ(set.getManifestSetByDependency("lib_geometry").size(), 1u);
  EXPECT_EQ(set.getManifestByModule("lib_geometry").getAppDepends(),
            (std::vector<std::string>{"pMarineViewer"}));
}

// Covers populator manifest set behavior: applies loc files including app prefix aliases.
TEST(PopulatorManifestSetTest, AppliesLocFilesIncludingAppPrefixAliases)
{
  const std::string manifest_path = writeFixture(
      "loc_manifest.mfs",
      "module = lib_mbutil\n"
      "type = library\n"
      "thumb = mbutil.png\n"
      "contact = moos-ivp@mit.edu\n"
      "author = michael benjamin\n"
      "org = mit\n"
      "synopsis = Common string and utility helpers.\n"
      "distro = moos-ivp\n"
      "borndate = 120101\n"
      "doc_url = https://oceanai.mit.edu\n"
      "\n"
      "module = pHelmIvP\n"
      "type = app\n"
      "thumb = helm.png\n"
      "contact = moos-ivp@mit.edu\n"
      "author = michael benjamin\n"
      "org = mit\n"
      "synopsis = IvP helm application.\n"
      "depends = lib_mbutil\n"
      "distro = moos-ivp\n"
      "borndate = 120201\n"
      "doc_url = https://oceanai.mit.edu\n");

  const std::string loc_path = writeFixture(
      "loc_manifest.loc",
      "module=lib_mbutil,loc=500,foc=5,wkyrs=0.5\n"
      "module=app_pHelmIvP,loc=1200,foc=12,wkyrs=1.2\n");

  Populator_ManifestSet populator;
  populator.addManifestFile(manifest_path);
  populator.addLOCFile(loc_path);
  ASSERT_TRUE(populator.populate());

  ManifestSet set = populator.getManifestSet();
  Manifest helm = set.getManifestByModule("pHelmIvP");
  EXPECT_EQ(helm.getLinesOfCode(), 1200u);
  EXPECT_EQ(helm.getFilesOfCode(), 12u);
  EXPECT_DOUBLE_EQ(helm.getWorkYears(), 1.2);
  EXPECT_EQ(helm.getLinesOfCodeTot(), 1700u);
  EXPECT_EQ(helm.getFilesOfCodeTot(), 17u);
  EXPECT_DOUBLE_EQ(helm.getWorkYearsTot(), 1.7);
  EXPECT_EQ(set.getAllLinesOfCode(), 1700u);
}

// Covers populator manifest set behavior: supports comments continuation and optional fields.
TEST(PopulatorManifestSetTest, SupportsCommentsContinuationAndOptionalFields)
{
  const std::string manifest_path = writeFixture(
      "alias_manifest.mfs",
      "// pRealm and appcast metadata use continuation lines in generated manifests\n"
      "module = pRealm\n"
      "type = app\n"
      "thumb = realm.png\n"
      "contact = moos-ivp@mit.edu\n"
      "author = michael benjamin, jane doe\n"
      "org = mit, oceanai\n"
      "group = realm, appcast\n"
      "synopsis = Realmcast distribution with literal DEPLOY=true text\n"
      "  and continuation text containing MODE=SURVEY without becoming a key.\n"
      "depends = lib_mbutil, lib_logic\n"
      "distro = moos-ivp\n"
      "download = https://oceanai.mit.edu/moos-ivp\n"
      "borndate = 240101\n"
      "mod_date = 240201\n"
      "mod_date = 240301\n"
      "doc_url = https://oceanai.mit.edu/ivpman/apps/pRealm\n"
      "license = GPL\n");

  Populator_ManifestSet populator;
  populator.addManifestFile(manifest_path);
  ASSERT_TRUE(populator.populate());

  ManifestSet set = populator.getManifestSet();
  ASSERT_EQ(set.size(), 1u);
  Manifest realm = set.getManifestByModule("pRealm");
  EXPECT_TRUE(realm.valid());
  EXPECT_TRUE(realm.hasAuthor("Jane"));
  EXPECT_TRUE(realm.hasOrg("oceanai"));
  EXPECT_TRUE(realm.hasGroup("appcast"));
  EXPECT_TRUE(realm.hasDependency("lib_logic"));
  EXPECT_EQ(realm.getDownload(), "https://oceanai.mit.edu/moos-ivp");
  EXPECT_EQ(realm.getModDates(),
            (std::vector<std::string>{"240201", "240301"}));
  EXPECT_NE(realm.getSynopsisStr().find("DEPLOY=true"), std::string::npos);
  EXPECT_NE(realm.getSynopsisStr().find("MODE=SURVEY"), std::string::npos);
}

// Covers populator manifest set behavior: rejects plural aliases before second pass handling.
TEST(PopulatorManifestSetTest, RejectsPluralAliasesBeforeSecondPassHandling)
{
  const std::string manifest_path = writeFixture(
      "plural_alias_manifest.mfs",
      "module = pRealm\n"
      "type = app\n"
      "thumb = realm.png\n"
      "contact = moos-ivp@mit.edu\n"
      "authors = michael benjamin, jane doe\n"
      "groups = realm, appcast\n");

  Populator_ManifestSet populator;
  populator.addManifestFile(manifest_path);
  EXPECT_FALSE(populator.populate());
}

// Covers populator manifest set behavior: rejects invalid line keys and missing files.
TEST(PopulatorManifestSetTest, RejectsInvalidLineKeysAndMissingFiles)
{
  const std::string invalid_manifest_path = writeFixture(
      "invalid_manifest.mfs",
      "module = pHostInfo\n"
      "type = app\n"
      "bogus_key = value\n");

  Populator_ManifestSet invalid_manifest;
  invalid_manifest.addManifestFile(invalid_manifest_path);
  EXPECT_FALSE(invalid_manifest.populate());

  const std::string manifest_path = writeFixture(
      "good_manifest.mfs",
      "module = pHostInfo\n"
      "type = app\n"
      "thumb = host.png\n"
      "contact = moos-ivp@mit.edu\n"
      "author = michael benjamin\n"
      "org = mit\n"
      "synopsis = Host information app.\n"
      "depends = lib_mbutil\n"
      "distro = moos-ivp\n"
      "borndate = 120101\n"
      "doc_url = https://oceanai.mit.edu\n");

  const std::string invalid_loc_path = writeFixture(
      "invalid_loc.loc",
      "module=pHostInfo,loc=100,bad=field\n");

  Populator_ManifestSet invalid_loc;
  invalid_loc.addManifestFile(manifest_path);
  invalid_loc.addLOCFile(invalid_loc_path);
  EXPECT_FALSE(invalid_loc.populate());
}

// Covers populator manifest set behavior: allows duplicate modules and comment only loc files.
TEST(PopulatorManifestSetTest, AllowsDuplicateModulesAndCommentOnlyLocFiles)
{
  const std::string duplicate_manifest_path = writeFixture(
      "duplicate_manifest.mfs",
      "module = pHelmIvP\n"
      "type = app\n"
      "thumb = helm.png\n"
      "contact = moos-ivp@mit.edu\n"
      "\n"
      "module = pHelmIvP\n"
      "type = app\n"
      "thumb = helm2.png\n"
      "contact = moos-ivp@mit.edu\n");

  Populator_ManifestSet duplicate_manifest;
  duplicate_manifest.addManifestFile(duplicate_manifest_path);
  ASSERT_TRUE(duplicate_manifest.populate());
  ManifestSet duplicate_set = duplicate_manifest.getManifestSet();
  EXPECT_EQ(duplicate_set.size(), 2u);
  EXPECT_EQ(duplicate_set.getManifestByIndex(0).getThumb(), "helm.png");
  EXPECT_EQ(duplicate_set.getManifestByIndex(1).getThumb(), "helm2.png");
  EXPECT_EQ(duplicate_set.getManifestByModule("pHelmIvP").getThumb(),
            "helm.png");

  const std::string empty_loc_manifest_path = writeFixture(
      "empty_loc_manifest.mfs",
      "module = pHostInfo\n"
      "type = app\n"
      "thumb = host.png\n"
      "contact = moos-ivp@mit.edu\n"
      "depends = lib_mbutil\n");
  const std::string empty_loc_path = writeFixture(
      "empty.loc",
      "// only comments and blanks\n"
      "\n");

  Populator_ManifestSet empty_loc;
  empty_loc.addManifestFile(empty_loc_manifest_path);
  empty_loc.addLOCFile(empty_loc_path);
  EXPECT_TRUE(empty_loc.populate());
  EXPECT_EQ(empty_loc.getManifestSet().size(), 1u);
}

// Covers populator manifest set behavior: populates across multiple manifest files and loc aliases.
TEST(PopulatorManifestSetTest, PopulatesAcrossMultipleManifestFilesAndLocAliases)
{
  const std::string libs_path = writeFixture(
      "libs_manifest.mfs",
      "module = lib_geometry\n"
      "type = library\n"
      "thumb = geometry.png\n"
      "contact = moos-ivp@mit.edu\n"
      "depends = lib_mbutil\n"
      "distro = moos-ivp\n");
  const std::string apps_path = writeFixture(
      "apps_manifest.mfs",
      "module = uXMS\n"
      "type = app\n"
      "thumb = uxms.png\n"
      "contact = moos-ivp@mit.edu\n"
      "depends = lib_geometry\n"
      "distro = moos-ivp\n");
  const std::string loc_path = writeFixture(
      "multi.loc",
      "module=lib_geometry,loc=1000,foc=10,wkyrs=1.0\n"
      "module=app_uXMS,loc=150,foc=2,wkyrs=0.2\n");

  Populator_ManifestSet populator;
  populator.addManifestFile(libs_path);
  populator.addManifestFile(apps_path);
  populator.addLOCFile(loc_path);
  ASSERT_TRUE(populator.populate());

  ManifestSet set = populator.getManifestSet();
  EXPECT_EQ(set.size(), 2u);
  EXPECT_EQ(set.getManifestByModule("lib_geometry").getAppDepends(),
            (std::vector<std::string>{"uXMS"}));
  EXPECT_EQ(set.getManifestByModule("uXMS").getLinesOfCode(), 150u);
  EXPECT_EQ(set.getManifestByModule("uXMS").getLinesOfCodeTot(), 1150u);
}

// Covers populator manifest set behavior: pins line key case sensitivity and continuation rules.
TEST(PopulatorManifestSetTest, PinsLineKeyCaseSensitivityAndContinuationRules)
{
  const std::string bad_case_path = writeFixture(
      "bad_case_manifest.mfs",
      "Module = pHostInfo\n"
      "type = app\n");

  Populator_ManifestSet bad_case;
  bad_case.addManifestFile(bad_case_path);
  EXPECT_FALSE(bad_case.populate());

  const std::string continuation_path = writeFixture(
      "continuation_manifest.mfs",
      "module = pContactMgrV20\n"
      "type = app\n"
      "thumb = contact.png\n"
      "contact = moos-ivp@mit.edu\n"
      "synopsis = Posts CONTACT_INFO summaries\n"
      "  RANGE=42 text remains part of the synopsis because it is indented.\n"
      "depends = lib_mbutil\n");

  Populator_ManifestSet continuation;
  continuation.addManifestFile(continuation_path);
  ASSERT_TRUE(continuation.populate());
  EXPECT_NE(continuation.getManifestSet()
                .getManifestByModule("pContactMgrV20")
                .getSynopsisStr()
                .find("RANGE=42 text"),
            std::string::npos);
}

// Covers populator manifest set behavior: pins loc parser partial and missing module behavior.
TEST(PopulatorManifestSetTest, PinsLocParserPartialAndMissingModuleBehavior)
{
  const std::string manifest_path = writeFixture(
      "partial_loc_manifest.mfs",
      "module = pHostInfo\n"
      "type = app\n"
      "thumb = host.png\n"
      "contact = moos-ivp@mit.edu\n");
  const std::string loc_path = writeFixture(
      "partial.loc",
      "module=pHostInfo,loc=100\n"
      "loc=999,foc=9,wkyrs=0.9\n");

  Populator_ManifestSet populator;
  populator.addManifestFile(manifest_path);
  populator.addLOCFile(loc_path);
  ASSERT_TRUE(populator.populate());

  Manifest host = populator.getManifestSet().getManifestByModule("pHostInfo");
  EXPECT_EQ(host.getLinesOfCode(), 100u);
  EXPECT_EQ(host.getFilesOfCode(), 0u);
  EXPECT_DOUBLE_EQ(host.getWorkYears(), 0);
}

// Covers populator manifest set behavior: invalid born date exits even when exit on failure disabled.
TEST(PopulatorManifestSetTest, InvalidBornDateExitsEvenWhenExitOnFailureDisabled)
{
  const std::string manifest_path = writeFixture(
      "bad_borndate_manifest.mfs",
      "module = pHostInfo\n"
      "type = app\n"
      "thumb = host.png\n"
      "contact = moos-ivp@mit.edu\n"
      "borndate = 20240101\n");

  EXPECT_EXIT(
      {
        Populator_ManifestSet populator;
        populator.setExitOnFailure(false);
        populator.addManifestFile(manifest_path);
        populator.populate();
      },
      testing::ExitedWithCode(1),
      "");
}
