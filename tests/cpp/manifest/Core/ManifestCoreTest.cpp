#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Manifest.h"
#include "ManifestSet.h"

namespace {

Manifest makeManifest(const std::string& module,
                      const std::string& type,
                      const std::string& date)
{
  Manifest manifest;
  manifest.setModuleName(module);
  manifest.setType(type);
  manifest.setThumb(module + ".png");
  manifest.setContactEmail("moos-ivp@mit.edu");
  manifest.setDistro("moos-ivp");
  EXPECT_TRUE(manifest.setDate(date));
  manifest.addSynopsisLine(module + " synopsis");
  manifest.addAuthor("michael benjamin");
  manifest.addOrg("mit");
  return manifest;
}

}  // namespace

// Covers manifest behavior: stores MOOS module metadata and comma separated lists.
TEST(ManifestTest, StoresMoosModuleMetadataAndCommaSeparatedLists)
{
  Manifest manifest = makeManifest("pHelmIvP", "app", "240701");
  manifest.setDocURL("https://oceanai.mit.edu/ivpman/apps/pHelmIvP");
  manifest.setDownload("https://oceanai.mit.edu");
  manifest.setLicense("GPL");
  manifest.setLinesOfCode(1200);
  manifest.setFilesOfCode(12);
  manifest.setWorkYears(1.25);
  manifest.setLinesOfCodeTot(1500);
  manifest.setFilesOfCodeTot(18);
  manifest.setWorkYearsTot(1.75);
  manifest.addLibDependency("lib_helmivp, lib_ivpbuild, lib_mbutil");
  manifest.addLibDependency("lib_mbutil");
  manifest.addAppDepender("pMarineViewer, uMACView");
  manifest.addAuthor("henrik schmidt, michael benjamin");
  manifest.addOrg("mit, csail");
  manifest.addGroup("helm, autonomy");
  manifest.addModDate("250101");

  EXPECT_EQ(manifest.getModuleName(), "pHelmIvP");
  EXPECT_EQ(manifest.getDocURL(), "https://oceanai.mit.edu/ivpman/apps/pHelmIvP");
  EXPECT_EQ(manifest.getLicense(), "GPL");
  EXPECT_EQ(manifest.getLinesOfCode(), 1200u);
  EXPECT_EQ(manifest.getFilesOfCode(), 12u);
  EXPECT_DOUBLE_EQ(manifest.getWorkYears(), 1.25);
  EXPECT_EQ(manifest.getLinesOfCodeTot(), 1500u);
  EXPECT_EQ(manifest.getFilesOfCodeTot(), 18u);
  EXPECT_DOUBLE_EQ(manifest.getWorkYearsTot(), 1.75);
  EXPECT_EQ(manifest.getLibDepends(),
            (std::vector<std::string>{"lib_helmivp", "lib_ivpbuild", "lib_mbutil"}));
  EXPECT_EQ(manifest.getAppDepends(),
            (std::vector<std::string>{"pMarineViewer", "uMACView"}));
  EXPECT_EQ(manifest.getGroups(),
            (std::vector<std::string>{"helm", "autonomy"}));
  EXPECT_EQ(manifest.getModDates(), (std::vector<std::string>{"250101"}));
  EXPECT_NE(manifest.getSynopsisStr().find("pHelmIvP synopsis"), std::string::npos);
}

// Covers manifest behavior: validates yymmdd born dates with current permissive bounds.
TEST(ManifestTest, ValidatesYYMMDDBornDatesWithCurrentPermissiveBounds)
{
  Manifest manifest;
  EXPECT_TRUE(manifest.setDate("240229"));
  EXPECT_EQ(manifest.getDate(), "240229");
  EXPECT_EQ(manifest.getNumDate(), 240229u);

  EXPECT_FALSE(manifest.setDate("20240229"));
  EXPECT_FALSE(manifest.setDate("24A229"));
  EXPECT_FALSE(manifest.setDate("241301"));
  EXPECT_FALSE(manifest.setDate("240132"));
  EXPECT_TRUE(manifest.setDate("240000"));
  EXPECT_EQ(manifest.getDate(), "240000");
}

// Covers manifest behavior: reports missing mandatory and optional fields by module type.
TEST(ManifestTest, ReportsMissingMandatoryAndOptionalFieldsByModuleType)
{
  Manifest app;
  EXPECT_FALSE(app.valid());
  EXPECT_EQ(app.missingMandatory(),
            (std::vector<std::string>{"module_name", "thumb", "contact"}));
  EXPECT_NE(app.missingOptionalCnt(), 0u);

  app = makeManifest("pMarineViewer", "app", "220101");
  app.addLibDependency("lib_marineview");
  app.setDocURL("https://oceanai.mit.edu/ivpman/apps/pMarineViewer");
  EXPECT_TRUE(app.valid());
  EXPECT_EQ(app.missingMandatoryCnt(), 0u);
  EXPECT_EQ(app.missingOptionalCnt(), 0u);

  Manifest group;
  group.setModuleName("Utilities");
  group.setType("group");
  group.setContactEmail("moos-ivp@mit.edu");
  group.addSynopsisLine("Common utility applications.");
  EXPECT_TRUE(group.valid());
  EXPECT_EQ(group.missingMandatoryCnt(), 0u);
  EXPECT_EQ(group.missingOptional(), (std::vector<std::string>{"distro"}));
}

// Covers manifest behavior: searches authors dependencies organizations and groups.
TEST(ManifestTest, SearchesAuthorsDependenciesOrganizationsAndGroups)
{
  Manifest manifest = makeManifest("BHV_Waypoint", "behavior", "230101");
  manifest.addAuthor("michael benjamin, henrik schmidt");
  manifest.addLibDependency("lib_behaviors, lib_geometry");
  manifest.addOrg("mit");
  manifest.addGroup("behaviors, navigation");

  EXPECT_TRUE(manifest.hasAuthor("Michael"));
  EXPECT_TRUE(manifest.hasAuthor("Benjamin"));
  EXPECT_TRUE(manifest.hasAuthor("Henrik Schmidt"));
  EXPECT_FALSE(manifest.hasAuthor("Ada"));
  EXPECT_TRUE(manifest.hasDependency("LIB_GEOMETRY"));
  EXPECT_FALSE(manifest.hasDependency("lib_contacts"));
  EXPECT_TRUE(manifest.hasOrg("mit"));
  EXPECT_FALSE(manifest.hasOrg("caltech"));
  EXPECT_TRUE(manifest.hasGroup("NAVIGATION"));
}

// Covers manifest behavior: pins case whitespace and dedup semantics.
TEST(ManifestTest, PinsCaseWhitespaceAndDedupSemantics)
{
  Manifest manifest;
  manifest.addAuthor("Michael Benjamin, michael benjamin, Michael Benjamin");
  manifest.addOrg("MIT, mit");
  manifest.addGroup("Helm, helm, Helm");
  manifest.addLibDependency("lib_mbutil, lib_mbutil, lib_MBUTIL, ");
  manifest.addAppDepender("pHelmIvP, pHelmIvP, ");

  EXPECT_EQ(manifest.getAuthors(),
            (std::vector<std::string>{"Michael Benjamin", "michael benjamin"}));
  EXPECT_TRUE(manifest.hasAuthor("michael benjamin"));
  EXPECT_TRUE(manifest.hasAuthor("benjamin"));
  EXPECT_FALSE(manifest.hasAuthor("Ada"));

  EXPECT_EQ(manifest.getOrgs(), (std::vector<std::string>{"MIT", "mit"}));
  EXPECT_TRUE(manifest.hasOrg("mit"));
  EXPECT_TRUE(manifest.hasOrg("MIT"));
  EXPECT_FALSE(manifest.hasOrg("oceanai"));

  EXPECT_EQ(manifest.getGroups(), (std::vector<std::string>{"Helm", "helm"}));
  EXPECT_TRUE(manifest.hasGroup("HELM"));
  EXPECT_EQ(manifest.getLibDepends(),
            (std::vector<std::string>{"lib_mbutil", "lib_MBUTIL", ""}));
  EXPECT_TRUE(manifest.hasDependency("LIB_MBUTIL"));
  EXPECT_EQ(manifest.getAppDepends(),
            (std::vector<std::string>{"pHelmIvP", ""}));
}

// Covers manifest behavior: print formats manifest records for manifest tools.
TEST(ManifestTest, PrintFormatsManifestRecordsForManifestTools)
{
  Manifest manifest = makeManifest("uXMS", "app", "221231");
  manifest.setDocURL("https://oceanai.mit.edu/ivpman/apps/uXMS");
  manifest.setLicense("GPL");
  manifest.addLibDependency("lib_mbutil");
  manifest.addModDate("230101");

  testing::internal::CaptureStdout();
  manifest.print();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Module   = uXMS"), std::string::npos);
  EXPECT_NE(output.find("doc_url  = https://oceanai.mit.edu/ivpman/apps/uXMS"),
            std::string::npos);
  EXPECT_NE(output.find("date     = 221231"), std::string::npos);
  EXPECT_NE(output.find("depends  = lib_mbutil"), std::string::npos);
  EXPECT_NE(output.find("synopsis = uXMS synopsis"), std::string::npos);

  testing::internal::CaptureStdout();
  manifest.printTerse();
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "* uXMS (moos-ivp) uXMS synopsis\n");
}

// Covers manifest set behavior: filters and aggregates manifest collections.
TEST(ManifestSetTest, FiltersAndAggregatesManifestCollections)
{
  Manifest helm = makeManifest("pHelmIvP", "app", "240701");
  helm.addLibDependency("lib_helmivp, lib_ivpbuild");
  helm.addGroup("helm");
  helm.addAuthor("michael benjamin");

  Manifest lib = makeManifest("lib_helmivp", "library", "180101");
  lib.addGroup("helm");
  lib.addAuthor("michael benjamin, henrik schmidt");

  Manifest view = makeManifest("pMarineViewer", "app", "220101");
  view.addLibDependency("lib_marineview, lib_geometry");
  view.addGroup("viewer");
  view.addAuthor("jane doe");

  Manifest group = makeManifest("Helm Apps", "group", "170101");
  group.setThumb("");
  group.setDocURL("https://oceanai.mit.edu/ivpman/helm");
  group.setDownload("https://oceanai.mit.edu/moos-ivp");
  group.addGroup("helm");
  group.addSynopsisLine("Applications around IvP helm operation.");

  ManifestSet set;
  EXPECT_TRUE(set.addManifest(helm));
  EXPECT_TRUE(set.addManifest(lib));
  EXPECT_TRUE(set.addManifest(view));
  EXPECT_TRUE(set.addManifest(group));

  EXPECT_EQ(set.size(), 3u);
  EXPECT_EQ(set.getManifestByModule("pHelmIvP").getType(), "app");
  EXPECT_EQ(set.getManifestByIndex(99).getModuleName(), "");
  EXPECT_EQ(set.getManifestSetByAuthor("Benjamin").size(), 3u);
  EXPECT_EQ(set.getManifestSetByGroup("helm").size(), 2u);
  EXPECT_EQ(set.getManifestSetByDistro("MOOS-IVP").size(), 3u);
  EXPECT_EQ(set.getManifestSetByDependency("lib_geometry").size(), 1u);
  EXPECT_EQ(set.getManifestSetByType("library").size(), 1u);
  EXPECT_EQ(set.getAllAuthors(),
            (std::vector<std::string>{"henrik schmidt", "jane doe", "michael benjamin"}));
  EXPECT_EQ(set.getAllAuthorsX(),
            (std::vector<std::string>{
                "henrik schmidt (1)", "jane doe (1)", "michael benjamin (4)"}));
  EXPECT_EQ(set.getAllGroups(), (std::vector<std::string>{"helm", "viewer"}));
  EXPECT_EQ(set.getAllDistros(), (std::vector<std::string>{"moos-ivp"}));
  EXPECT_EQ(set.getAllDependencies(),
            (std::vector<std::string>{
                "lib_geometry", "lib_helmivp", "lib_ivpbuild", "lib_marineview"}));
  EXPECT_EQ(set.getAllTypes(), (std::vector<std::string>{"app", "group", "library"}));
  EXPECT_EQ(set.getGroupDocURL("helm"), "https://oceanai.mit.edu/ivpman/helm");
  EXPECT_EQ(set.getGroupDistro("helm"), "moos-ivp");
  EXPECT_EQ(set.getGroupDownload("helm"), "https://oceanai.mit.edu/moos-ivp");
  EXPECT_EQ(set.getGroupSynopsis("helm").size(), 2u);
  EXPECT_TRUE(set.containsLibrary("libhelmivp"));
  EXPECT_TRUE(set.containsLibrary("lib_helmivp"));
  EXPECT_FALSE(set.containsLibrary("pHelmIvP"));
}

// Covers manifest set behavior: pins sorting type filtering and group last match behavior.
TEST(ManifestSetTest, PinsSortingTypeFilteringAndGroupLastMatchBehavior)
{
  Manifest older = makeManifest("uXMS", "app", "120101");
  older.addGroup("scope");
  Manifest newer = makeManifest("uMAC", "app", "240101");
  newer.setType("App");
  newer.addGroup("scope");
  Manifest group_one = makeManifest("Scope Tools", "group", "110101");
  group_one.setThumb("");
  group_one.addGroup("scope");
  group_one.setDocURL("https://oceanai.mit.edu/scope-old");
  group_one.setDownload("old-download");
  Manifest group_two = makeManifest("Scope Tools New", "GROUP", "250101");
  group_two.setThumb("");
  group_two.addGroup("scope");
  group_two.setDocURL("https://oceanai.mit.edu/scope-new");
  group_two.setDownload("new-download");

  ManifestSet set;
  ASSERT_TRUE(set.addManifest(newer));
  ASSERT_TRUE(set.addManifest(group_one));
  ASSERT_TRUE(set.addManifest(older));
  ASSERT_TRUE(set.addManifest(group_two));

  EXPECT_EQ(set.size(), 2u);
  EXPECT_EQ(set.getManifestSetByType("app").size(), 1u);
  EXPECT_EQ(set.getManifestSetByType("App").size(), 1u);
  EXPECT_EQ(set.getGroupDocURL("scope"), "https://oceanai.mit.edu/scope-new");
  EXPECT_EQ(set.getGroupDownload("SCOPE"), "new-download");

  set.orderNewToOld();
  EXPECT_EQ(set.getManifestByIndex(0).getModuleName(), "Scope Tools");
  EXPECT_EQ(set.getManifestByIndex(3).getModuleName(), "Scope Tools New");

  set.orderOldToNew();
  EXPECT_EQ(set.getManifestByIndex(0).getModuleName(), "Scope Tools");
}

// Covers manifest set behavior: associates library dependers and lines of code totals.
TEST(ManifestSetTest, AssociatesLibraryDependersAndLinesOfCodeTotals)
{
  Manifest lib = makeManifest("lib_geometry", "library", "180101");
  Manifest app = makeManifest("pMarineViewer", "app", "220101");
  app.addLibDependency("lib_geometry, lib_mbutil");

  ManifestSet set;
  ASSERT_TRUE(set.addManifest(lib));
  ASSERT_TRUE(set.addManifest(app));
  set.setLinesOfCode("lib_geometry", 1000);
  set.setFilesOfCode("lib_geometry", 10);
  set.setWorkYears("lib_geometry", 1.0);
  set.setLinesOfCode("lib_mbutil", 500);
  set.setFilesOfCode("lib_mbutil", 5);
  set.setWorkYears("lib_mbutil", 0.5);
  set.setLinesOfCode("pMarineViewer", 200);
  set.setFilesOfCode("pMarineViewer", 2);
  set.setWorkYears("pMarineViewer", 0.2);

  set.associateDependers();
  set.associateLinesOfCode();

  Manifest updated_lib = set.getManifestByModule("lib_geometry");
  Manifest updated_app = set.getManifestByModule("pMarineViewer");
  EXPECT_EQ(updated_lib.getAppDepends(), (std::vector<std::string>{"pMarineViewer"}));
  EXPECT_EQ(updated_lib.getLinesOfCode(), 1000u);
  EXPECT_EQ(updated_app.getLinesOfCode(), 200u);
  EXPECT_EQ(updated_app.getLinesOfCodeTot(), 1700u);
  EXPECT_EQ(updated_app.getFilesOfCodeTot(), 17u);
  EXPECT_DOUBLE_EQ(updated_app.getWorkYearsTot(), 1.7);
  EXPECT_EQ(set.getAllLinesOfCode(), 1200u);
  EXPECT_EQ(set.getAllFilesOfCode(), 12u);
  EXPECT_DOUBLE_EQ(set.getAllWorkYears(), 1.2);
}
