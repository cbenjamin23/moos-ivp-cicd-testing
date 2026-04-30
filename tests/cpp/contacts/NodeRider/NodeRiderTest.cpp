#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "NodeRider.h"
#include "NodeRiderSet.h"

namespace {

class ExposedNodeRider : public NodeRider {
 public:
  using NodeRider::setFrequency;
  using NodeRider::setPolicy;
};

}  // namespace

// Covers node rider behavior: validates reserved vars policies precision and specs.
TEST(NodeRiderTest, ValidatesReservedVarsPoliciesPrecisionAndSpecs)
{
  NodeRider rider;
  EXPECT_FALSE(rider.valid());
  EXPECT_EQ(rider.getSpec(), "");
  EXPECT_FALSE(rider.setVar("NAME"));
  EXPECT_TRUE(rider.setVar("DB_UPTIME"));
  EXPECT_FALSE(rider.setRiderFld("bad_field"));
  EXPECT_TRUE(rider.setRiderFld("uptime"));
  EXPECT_FALSE(rider.setPolicyConfig("updated+-1"));
  EXPECT_TRUE(rider.setPolicyConfig("updated+10"));
  EXPECT_TRUE(rider.setPrecision(12));
  EXPECT_EQ(rider.getPrecision(), 10u);
  EXPECT_TRUE(rider.valid());
  EXPECT_EQ(rider.getSpec(), "var=DB_UPTIME, policy=updated+10, rfld=uptime");

  NodeRider frequency;
  EXPECT_TRUE(frequency.setVar("NAV_DEPTH"));
  EXPECT_TRUE(frequency.setPolicyConfig("2.5"));
  EXPECT_EQ(frequency.getPolicy(), "freq");
  EXPECT_DOUBLE_EQ(frequency.getFrequency(), 2.5);
  EXPECT_EQ(frequency.getSpec(), "var=NAV_DEPTH, policy=2.5");
}

// Covers node rider behavior: rejects all reserved NODE_REPORT fields as MOOS vars.
TEST(NodeRiderTest, RejectsAllReservedNodeReportFieldsAsMoosVars)
{
  const std::vector<std::string> reserved = {
      "NAME", "X", "Y", "SPD", "HDG", "DEP", "LAT", "LON", "TYPE",
      "COLOR", "MODE", "ALLSTOP", "INDEX", "YAW", "TIME", "LENGTH"};

  for(const std::string& var : reserved) {
    NodeRider rider;
    EXPECT_FALSE(rider.setVar(var)) << var;
    EXPECT_FALSE(rider.valid()) << var;
  }

  NodeRider case_sensitive;
  EXPECT_TRUE(case_sensitive.setVar("name"));
  EXPECT_TRUE(case_sensitive.valid());
}

// Covers node rider behavior: rejected reserved var does not replace prior var but empty var does.
TEST(NodeRiderTest, RejectedReservedVarDoesNotReplacePriorVarButEmptyVarDoes)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));
  EXPECT_TRUE(rider.valid());

  EXPECT_FALSE(rider.setVar("NAME"));
  EXPECT_EQ(rider.getVar(), "CONTACT_RANGE");
  EXPECT_TRUE(rider.valid());

  EXPECT_TRUE(rider.setVar(""));
  EXPECT_EQ(rider.getVar(), "");
  EXPECT_FALSE(rider.valid());
  EXPECT_EQ(rider.getSpec(), "");
}

// Covers node rider behavior: direct set var accepts whitespace but set parser rejects it.
TEST(NodeRiderTest, DirectSetVarAcceptsWhitespaceButSetParserRejectsIt)
{
  NodeRider rider;
  EXPECT_TRUE(rider.setVar("BAD VAR"));
  EXPECT_TRUE(rider.valid());
  EXPECT_EQ(rider.getSpec(), "var=BAD VAR, policy=always");

  NodeRiderSet set;
  EXPECT_FALSE(set.addNodeRider("var=BAD VAR,policy=always"));
  EXPECT_EQ(set.size(), 0u);
}

// Covers node rider behavior: policy config rejects malformed values and keeps prior config.
TEST(NodeRiderTest, PolicyConfigRejectsMalformedValuesAndKeepsPriorConfig)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));
  ASSERT_TRUE(rider.setPolicyConfig("updated+10"));
  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 10);

  EXPECT_FALSE(rider.setPolicyConfig("updated+-1"));
  EXPECT_FALSE(rider.setPolicyConfig("updated+"));
  EXPECT_FALSE(rider.setPolicyConfig("updated+fast"));
  EXPECT_FALSE(rider.setPolicyConfig("-1"));
  EXPECT_FALSE(rider.setPolicyConfig("freq"));
  EXPECT_FALSE(rider.setPolicyConfig("ALWAYS"));

  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 10);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=updated+10");
}

// Covers node rider behavior: policy transitions use latest accepted policy.
TEST(NodeRiderTest, PolicyTransitionsUseLatestAcceptedPolicy)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));

  EXPECT_TRUE(rider.setPolicyConfig("3"));
  EXPECT_EQ(rider.getPolicy(), "freq");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 3);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=3");

  EXPECT_TRUE(rider.setPolicyConfig("updated"));
  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), -1);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=updated");

  EXPECT_TRUE(rider.setPolicyConfig("always"));
  EXPECT_EQ(rider.getPolicy(), "always");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), -1);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=always");
}

// Covers node rider behavior: policy config accepts positive signs and compacts spec formatting.
TEST(NodeRiderTest, PolicyConfigAcceptsPositiveSignsAndCompactsSpecFormatting)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));

  EXPECT_TRUE(rider.setPolicyConfig("+2.50"));
  EXPECT_EQ(rider.getPolicy(), "freq");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 2.5);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=2.5");

  EXPECT_TRUE(rider.setPolicyConfig("updated+02.50"));
  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 2.5);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=updated+2.5");
}

// Covers node rider behavior: always policy does not reset prior frequency value.
TEST(NodeRiderTest, AlwaysPolicyDoesNotResetPriorFrequencyValue)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));
  ASSERT_TRUE(rider.setPolicyConfig("updated+10"));
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 10);

  EXPECT_TRUE(rider.setPolicyConfig("always"));
  EXPECT_EQ(rider.getPolicy(), "always");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 10);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=always");

  EXPECT_TRUE(rider.setPolicyConfig("updated"));
  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), -1);
}

// Covers node rider behavior: protected policy and frequency helpers validate direct inputs.
TEST(NodeRiderTest, ProtectedPolicyAndFrequencyHelpersValidateDirectInputs)
{
  ExposedNodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));

  EXPECT_TRUE(rider.setPolicy("updated"));
  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_TRUE(rider.setPolicy("freq"));
  EXPECT_EQ(rider.getPolicy(), "freq");
  EXPECT_TRUE(rider.setPolicy("always"));
  EXPECT_EQ(rider.getPolicy(), "always");
  EXPECT_FALSE(rider.setPolicy("sometimes"));
  EXPECT_EQ(rider.getPolicy(), "always");

  EXPECT_TRUE(rider.setFrequency(2.5));
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 2.5);
  EXPECT_FALSE(rider.setFrequency(-0.1));
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 2.5);
}

// Covers node rider behavior: updated zero policy has no frequency fallback.
TEST(NodeRiderTest, UpdatedZeroPolicyHasNoFrequencyFallback)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));
  ASSERT_TRUE(rider.setPolicyConfig("updated+0"));
  EXPECT_EQ(rider.getPolicy(), "updated");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 0);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=updated");
}

// Covers node rider behavior: rider field must be alphanumeric and precision clamps high only.
TEST(NodeRiderTest, RiderFieldMustBeAlphanumericAndPrecisionClampsHighOnly)
{
  NodeRider rider;
  EXPECT_TRUE(rider.setRiderFld("range1"));
  EXPECT_EQ(rider.getRiderFld(), "range1");
  EXPECT_FALSE(rider.setRiderFld("range_1"));
  EXPECT_FALSE(rider.setRiderFld("range-1"));
  EXPECT_FALSE(rider.setRiderFld(""));
  EXPECT_EQ(rider.getRiderFld(), "range1");

  EXPECT_TRUE(rider.setPrecision(0));
  EXPECT_EQ(rider.getPrecision(), 0u);
  EXPECT_TRUE(rider.setPrecision(99));
  EXPECT_EQ(rider.getPrecision(), 10u);
}

// Covers node rider behavior: update value marks fresh and tracks timestamp.
TEST(NodeRiderTest, UpdateValueMarksFreshAndTracksTimestamp)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_SUMMARY"));
  EXPECT_FALSE(rider.isFresh());
  EXPECT_EQ(rider.getCurrVal(), "");
  EXPECT_DOUBLE_EQ(rider.getLastUTC(), 0);

  EXPECT_TRUE(rider.updateValue("abe,ben", 12.5));
  EXPECT_TRUE(rider.isFresh());
  EXPECT_EQ(rider.getCurrVal(), "abe,ben");
  EXPECT_DOUBLE_EQ(rider.getLastUTC(), 12.5);

  rider.setFresh(false);
  rider.setLastUTC(20);
  EXPECT_FALSE(rider.isFresh());
  EXPECT_DOUBLE_EQ(rider.getLastUTC(), 20);
}

// Covers node rider set behavior: parses NODE_REPORTer rider config and reports always values.
TEST(NodeRiderSetTest, ParsesNodeReporterRiderConfigAndReportsAlwaysValues)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=DB_UPTIME,policy=always,rfld=uptime,prec=3"));
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_SUMMARY,policy=always,rfld=summary"));
  EXPECT_FALSE(set.addNodeRider("var=DB_UPTIME,policy=always"));
  EXPECT_EQ(set.size(), 2u);

  std::vector<std::string> vars = set.getVars();
  ASSERT_EQ(vars.size(), 2u);
  EXPECT_EQ(vars[0], "DB_UPTIME");
  EXPECT_EQ(vars[1], "CONTACT_SUMMARY");

  std::vector<std::string> specs = set.getRiderSpecs();
  ASSERT_EQ(specs.size(), 2u);
  EXPECT_EQ(specs[0], "var=DB_UPTIME, policy=always, rfld=uptime");
  EXPECT_EQ(specs[1], "var=CONTACT_SUMMARY, policy=always, rfld=summary");

  EXPECT_TRUE(set.updateRider("DB_UPTIME", "", 12.34567, 100));
  EXPECT_TRUE(set.updateRider("CONTACT_SUMMARY", "abe,ben", 100));
  EXPECT_EQ(set.getRiderReports(100), "uptime=12.346,summary={abe,ben}");
  EXPECT_EQ(set.getRiderReports(101), "uptime=12.346,summary={abe,ben}");
}

// Covers node rider set behavior: parses config fields in any order and formats precision.
TEST(NodeRiderSetTest, ParsesConfigFieldsInAnyOrderAndFormatsPrecision)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("policy=always,prec=0,rfld=range,var=CONTACT_RANGE"));
  ASSERT_TRUE(set.addNodeRider("rfld=bearing,var=CONTACT_BEARING,prec=2,policy=always"));

  std::vector<std::string> specs = set.getRiderSpecs();
  ASSERT_EQ(specs.size(), 2u);
  EXPECT_EQ(specs[0], "var=CONTACT_RANGE, policy=always, rfld=range");
  EXPECT_EQ(specs[1], "var=CONTACT_BEARING, policy=always, rfld=bearing");

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.9, 100));
  EXPECT_TRUE(set.updateRider("CONTACT_BEARING", "", 271.234, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=43,bearing=271.23");
}

// Covers node rider set behavior: config parser trims whitespace but is case sensitive.
TEST(NodeRiderSetTest, ConfigParserTrimsWhitespaceButIsCaseSensitive)
{
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider(" var = CONTACT_RANGE , policy = always , rfld = range "));
  EXPECT_EQ(set.size(), 1u);
  EXPECT_EQ(set.getRiderSpecs()[0], "var=CONTACT_RANGE, policy=always, rfld=range");

  NodeRiderSet mixed_case;
  EXPECT_FALSE(mixed_case.addNodeRider("VAR=CONTACT_RANGE,policy=always"));
  EXPECT_FALSE(mixed_case.addNodeRider("var=CONTACT_RANGE,POLICY=always"));
  EXPECT_EQ(mixed_case.size(), 0u);
}

// Covers node rider set behavior: duplicate config tokens use last value and loose precision uses atoi.
TEST(NodeRiderSetTest, DuplicateConfigTokensUseLastValueAndLoosePrecisionUsesAtoi)
{
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider(
      "var=CONTACT_RANGE,var=CONTACT_BEARING,policy=always,policy=updated,"
      "rfld=range,rfld=bearing,prec=3.9"));
  EXPECT_EQ(set.size(), 1u);

  std::vector<std::string> vars = set.getVars();
  ASSERT_EQ(vars.size(), 1u);
  EXPECT_EQ(vars[0], "CONTACT_BEARING");
  ASSERT_EQ(set.getRiderSpecs().size(), 1u);
  EXPECT_EQ(set.getRiderSpecs()[0],
            "var=CONTACT_BEARING, policy=updated, rfld=bearing");

  EXPECT_TRUE(set.updateRider("CONTACT_BEARING", "", 12.34567, 100));
  EXPECT_EQ(set.getRiderReports(100), "bearing=12.346");
  EXPECT_EQ(set.getRiderReports(101), "");

  NodeRiderSet non_numeric_prec;
  EXPECT_TRUE(non_numeric_prec.addNodeRider(
      "var=CONTACT_RANGE,policy=always,rfld=range,prec=abc"));
  EXPECT_TRUE(non_numeric_prec.updateRider("CONTACT_RANGE", "", 12.9, 100));
  EXPECT_EQ(non_numeric_prec.getRiderReports(100), "range=13");
}

// Covers node rider set behavior: duplicate always policy keeps earlier frequency on stored rider.
TEST(NodeRiderSetTest, DuplicateAlwaysPolicyKeepsEarlierFrequencyOnStoredRider)
{
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider(
      "var=CONTACT_RANGE,policy=updated+10,policy=always,rfld=range"));
  ASSERT_EQ(set.size(), 1u);

  NodeRider rider = set.getNodeRider(0);
  EXPECT_EQ(rider.getPolicy(), "always");
  EXPECT_DOUBLE_EQ(rider.getFrequency(), 10);
  EXPECT_EQ(rider.getSpec(), "var=CONTACT_RANGE, policy=always, rfld=range");

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.0, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=42.0000");
  EXPECT_EQ(set.getRiderReports(105), "range=42.0000");
}

// Covers node rider set behavior: var only config uses default always policy.
TEST(NodeRiderSetTest, VarOnlyConfigUsesDefaultAlwaysPolicy)
{
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider("var=CONTACT_RANGE"));

  std::vector<std::string> specs = set.getRiderSpecs();
  ASSERT_EQ(specs.size(), 1u);
  EXPECT_EQ(specs[0], "var=CONTACT_RANGE, policy=always");

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.0, 100));
  EXPECT_EQ(set.getRiderReports(100), "CONTACT_RANGE=42.0000");
}

// Covers node rider set behavior: policy only config stores invalid empty var rider.
TEST(NodeRiderSetTest, PolicyOnlyConfigStoresInvalidEmptyVarRider)
{
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider("policy=always"));
  EXPECT_EQ(set.size(), 1u);
  ASSERT_EQ(set.getVars().size(), 1u);
  EXPECT_EQ(set.getVars()[0], "");
  EXPECT_TRUE(set.getRiderSpecs().empty());

  EXPECT_TRUE(set.updateRider("", "value", 100));
  EXPECT_EQ(set.getRiderReports(100), "=value");
}

// Covers node rider set behavior: duplicate empty var riders are rejected.
TEST(NodeRiderSetTest, DuplicateEmptyVarRidersAreRejected)
{
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider("policy=always"));
  EXPECT_FALSE(set.addNodeRider("policy=updated"));
  EXPECT_EQ(set.size(), 1u);
}

// Covers node rider set behavior: rejects malformed NODE_REPORTer rider config.
TEST(NodeRiderSetTest, RejectsMalformedNodeReporterRiderConfig)
{
  const std::vector<std::string> malformed = {
      "var=NAME,policy=always",
      "var=CONTACT_RANGE,policy=updated+-1",
      "var=CONTACT_RANGE,policy=always,rfld=range_value",
      "var=CONTACT_RANGE,policy=always,prec=-1",
      "var=CONTACT RANGE,policy=always",
      "var=CONTACT_RANGE,policy=always,extra=value"};

  for(const std::string& config : malformed) {
    NodeRiderSet set;
    EXPECT_FALSE(set.addNodeRider(config)) << config;
  }
}

// Covers node rider set behavior: rejected string configs do not mutate set.
TEST(NodeRiderSetTest, RejectedStringConfigsDoNotMutateSet)
{
  NodeRiderSet bad_policy;
  EXPECT_FALSE(bad_policy.addNodeRider("var=CONTACT_RANGE,policy=updated+-1"));
  EXPECT_EQ(bad_policy.size(), 0u);
  EXPECT_TRUE(bad_policy.getRiderSpecs().empty());
  EXPECT_FALSE(bad_policy.updateRider("CONTACT_RANGE", "", 42.0, 100));

  NodeRiderSet bad_field;
  EXPECT_FALSE(bad_field.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=bad_field"));
  EXPECT_EQ(bad_field.size(), 0u);
  EXPECT_TRUE(bad_field.getRiderSpecs().empty());

  NodeRiderSet extra_token;
  EXPECT_FALSE(extra_token.addNodeRider("var=CONTACT_RANGE,policy=always,extra=value"));
  EXPECT_EQ(extra_token.size(), 0u);
  EXPECT_TRUE(extra_token.getRiderSpecs().empty());
}

// Covers node rider set behavior: degenerate string configs either reject or store invalid empty var riders.
TEST(NodeRiderSetTest, DegenerateStringConfigsEitherRejectOrStoreInvalidEmptyVarRiders)
{
  const std::vector<std::string> rejected_configs = {
      "CONTACT_RANGE",
      "policy"};

  for(const std::string& config : rejected_configs) {
    NodeRiderSet set;
    EXPECT_FALSE(set.addNodeRider(config)) << config;
    EXPECT_EQ(set.size(), 0u) << config;
    EXPECT_TRUE(set.getVars().empty()) << config;
    EXPECT_TRUE(set.getRiderSpecs().empty()) << config;
  }

  const std::vector<std::string> invalid_empty_var_configs = {
      "",
      "var",
      "var="};

  for(const std::string& config : invalid_empty_var_configs) {
    NodeRiderSet set;
    EXPECT_TRUE(set.addNodeRider(config)) << config;
    EXPECT_EQ(set.size(), 1u) << config;
    ASSERT_EQ(set.getVars().size(), 1u) << config;
    EXPECT_EQ(set.getVars()[0], "") << config;
    EXPECT_TRUE(set.getRiderSpecs().empty()) << config;
    EXPECT_TRUE(set.updateRider("", "value", 100)) << config;
    EXPECT_EQ(set.getRiderReports(100), "=value") << config;
  }

  NodeRiderSet field_only;
  EXPECT_TRUE(field_only.addNodeRider("rfld=range"));
  EXPECT_EQ(field_only.size(), 1u);
  EXPECT_TRUE(field_only.getRiderSpecs().empty());
  EXPECT_TRUE(field_only.updateRider("", "value", 100));
  EXPECT_EQ(field_only.getRiderReports(100), "range=value");
}

// Covers node rider set behavior: duplicate rejected even when first rider was added as object.
TEST(NodeRiderSetTest, DuplicateRejectedEvenWhenFirstRiderWasAddedAsObject)
{
  NodeRider rider;
  ASSERT_TRUE(rider.setVar("CONTACT_RANGE"));
  ASSERT_TRUE(rider.setRiderFld("range"));
  ASSERT_TRUE(rider.setPolicyConfig("always"));

  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider(rider));
  EXPECT_FALSE(set.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range2"));
  EXPECT_EQ(set.size(), 1u);
}

// Covers node rider set behavior: invalid object riders can be stored and updated by empty var.
TEST(NodeRiderSetTest, InvalidObjectRidersCanBeStoredAndUpdatedByEmptyVar)
{
  NodeRider invalid;
  NodeRiderSet set;
  EXPECT_TRUE(set.addNodeRider(invalid));
  EXPECT_EQ(set.size(), 1u);
  EXPECT_TRUE(set.getVars().empty() || set.getVars()[0] == "");
  EXPECT_TRUE(set.getRiderSpecs().empty());
  EXPECT_TRUE(set.updateRider("", "value", 100));
  EXPECT_EQ(set.getRiderReports(100), "=value");
}

// Covers node rider set behavior: out of range lookup returns invalid empty rider.
TEST(NodeRiderSetTest, OutOfRangeLookupReturnsInvalidEmptyRider)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range"));

  NodeRider rider = set.getNodeRider(0);
  EXPECT_TRUE(rider.valid());
  EXPECT_EQ(rider.getVar(), "CONTACT_RANGE");

  NodeRider missing = set.getNodeRider(1);
  EXPECT_FALSE(missing.valid());
  EXPECT_EQ(missing.getVar(), "");
}

// Covers node rider set behavior: get node rider returns copy not mutable reference.
TEST(NodeRiderSetTest, GetNodeRiderReturnsCopyNotMutableReference)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range"));
  ASSERT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.0, 100));

  NodeRider rider = set.getNodeRider(0);
  ASSERT_TRUE(rider.valid());
  EXPECT_TRUE(rider.setRiderFld("changed"));
  EXPECT_TRUE(rider.updateValue("99", 200));

  EXPECT_EQ(set.getNodeRider(0).getRiderFld(), "range");
  EXPECT_EQ(set.getRiderReports(100), "range=42.0000");
}

// Covers node rider set behavior: copy and assignment preserve independent rider state.
TEST(NodeRiderSetTest, CopyAndAssignmentPreserveIndependentRiderState)
{
  NodeRiderSet original;
  ASSERT_TRUE(original.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range"));
  ASSERT_TRUE(original.addNodeRider("var=CONTACT_STATUS,policy=updated,rfld=status"));
  ASSERT_TRUE(original.updateRider("CONTACT_RANGE", "", 42.0, 100));
  ASSERT_TRUE(original.updateRider("CONTACT_STATUS", "ready", 100));

  NodeRiderSet copy(original);
  NodeRiderSet assigned;
  assigned = original;

  EXPECT_EQ(copy.getRiderReports(100), "range=42.0000,status=ready");
  EXPECT_EQ(assigned.getRiderReports(100), "range=42.0000,status=ready");

  ASSERT_TRUE(original.updateRider("CONTACT_RANGE", "", 50.0, 101));
  ASSERT_TRUE(original.updateRider("CONTACT_STATUS", "changed", 101));
  EXPECT_EQ(original.getRiderReports(101), "range=50.0000,status=changed");
  EXPECT_EQ(copy.getRiderReports(101), "range=42.0000,");
  EXPECT_EQ(assigned.getRiderReports(101), "range=42.0000,");
}

// Covers node rider set behavior: reports updated policy only when fresh.
TEST(NodeRiderSetTest, ReportsUpdatedPolicyOnlyWhenFresh)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACTS_LIST,policy=updated,rfld=contacts"));

  EXPECT_EQ(set.getRiderReports(100), "");
  EXPECT_TRUE(set.updateRider("CONTACTS_LIST", "abe,ben", 100));
  EXPECT_EQ(set.getRiderReports(100), "contacts={abe,ben}");
  EXPECT_EQ(set.getRiderReports(101), "");

  EXPECT_TRUE(set.updateRider("CONTACTS_LIST", "abe,ben,cal", 102));
  EXPECT_EQ(set.getRiderReports(102), "contacts={abe,ben,cal}");
  EXPECT_EQ(set.getRiderReports(103), "");
}

// Covers node rider set behavior: empty updated values are accepted but not reported.
TEST(NodeRiderSetTest, EmptyUpdatedValuesAreAcceptedButNotReported)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_STATUS,policy=updated,rfld=status"));

  EXPECT_TRUE(set.updateRider("CONTACT_STATUS", "", 100));
  EXPECT_EQ(set.getRiderReports(100), "");
  EXPECT_EQ(set.getRiderReports(101), "");

  EXPECT_TRUE(set.updateRider("CONTACT_STATUS", "ready", 102));
  EXPECT_EQ(set.getRiderReports(102), "status=ready");
}

// Covers node rider set behavior: updated policy replaces prior value and refreshes timestamp.
TEST(NodeRiderSetTest, UpdatedPolicyReplacesPriorValueAndRefreshesTimestamp)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=updated+10,rfld=range,prec=1"));

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.01, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=42.0");
  EXPECT_EQ(set.getRiderReports(109.9), "");

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 45.55, 108));
  EXPECT_EQ(set.getRiderReports(108), "range=45.5");
  EXPECT_EQ(set.getRiderReports(117.9), "");
  EXPECT_EQ(set.getRiderReports(118), "CONTACT_RANGE=45.5");
}

// Covers node rider set behavior: updated zero reports only fresh updates.
TEST(NodeRiderSetTest, UpdatedZeroReportsOnlyFreshUpdates)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=updated+0,rfld=range"));

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.1234, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=42.1234");
  EXPECT_EQ(set.getRiderReports(100), "");
  EXPECT_EQ(set.getRiderReports(1000), "");
}

// Covers node rider set behavior: reports updated plus frequency fallback with MOOS var field.
TEST(NodeRiderSetTest, ReportsUpdatedPlusFrequencyFallbackWithMoosVarField)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=updated+10,rfld=range"));

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.1234, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=42.1234");
  EXPECT_EQ(set.getRiderReports(109.9), "");
  EXPECT_EQ(set.getRiderReports(110), "CONTACT_RANGE=42.1234");
}

// Covers node rider set behavior: reports frequency policy only after elapsed interval.
TEST(NodeRiderSetTest, ReportsFrequencyPolicyOnlyAfterElapsedInterval)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=NAV_DEPTH,policy=5,rfld=depth,prec=1"));

  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 12.34, 100));
  EXPECT_EQ(set.getRiderReports(104.9), "");
  EXPECT_EQ(set.getRiderReports(105), "depth=12.3");
  EXPECT_EQ(set.getRiderReports(109.9), "");
  EXPECT_EQ(set.getRiderReports(110), "depth=12.3");
}

// Covers node rider set behavior: frequency policy replaces value before next interval.
TEST(NodeRiderSetTest, FrequencyPolicyReplacesValueBeforeNextInterval)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=NAV_DEPTH,policy=5,rfld=depth,prec=2"));

  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 12.344, 100));
  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 13.555, 102));
  EXPECT_EQ(set.getRiderReports(106.9), "");
  EXPECT_EQ(set.getRiderReports(107), "depth=13.55");

  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 14.001, 108));
  EXPECT_EQ(set.getRiderReports(112.9), "");
  EXPECT_EQ(set.getRiderReports(113), "depth=14.00");
}

// Covers node rider set behavior: double update uses non empty string value instead of formatting double.
TEST(NodeRiderSetTest, DoubleUpdateUsesNonEmptyStringValueInsteadOfFormattingDouble)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range,prec=1"));

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "forty-two", 42.0, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=forty-two");

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.0, 101));
  EXPECT_EQ(set.getRiderReports(101), "range=42.0");
}

// Covers node rider set behavior: frequency zero reports every time after first value.
TEST(NodeRiderSetTest, FrequencyZeroReportsEveryTimeAfterFirstValue)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=NAV_DEPTH,policy=0,rfld=depth,prec=2"));

  EXPECT_EQ(set.getRiderReports(0), "");
  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 12.345, 100));
  EXPECT_EQ(set.getRiderReports(100), "depth=12.35");
  EXPECT_EQ(set.getRiderReports(100), "depth=12.35");
  EXPECT_EQ(set.getRiderReports(99), "");
  EXPECT_EQ(set.getRiderReports(101), "depth=12.35");
}

// Covers node rider set behavior: update unknown var returns false and leaves reports unchanged.
TEST(NodeRiderSetTest, UpdateUnknownVarReturnsFalseAndLeavesReportsUnchanged)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range"));

  EXPECT_FALSE(set.updateRider("CONTACT_BEARING", "", 20.5, 100));
  EXPECT_FALSE(set.updateRider("CONTACT_BEARING", "east", 100));
  EXPECT_EQ(set.getRiderReports(100), "");

  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.0, 100));
  EXPECT_EQ(set.getRiderReports(100), "range=42.0000");
}

// Covers node rider set behavior: string values with commas are brace wrapped but numeric strings are not.
TEST(NodeRiderSetTest, StringValuesWithCommasAreBraceWrappedButNumericStringsAreNot)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_LIST,policy=always,rfld=contacts"));
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=always,rfld=range,prec=1"));

  EXPECT_TRUE(set.updateRider("CONTACT_LIST", "abe,ben,cal", 100));
  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "42.25", 99.99, 100));
  EXPECT_EQ(set.getRiderReports(100), "contacts={abe,ben,cal},range=42.25");
}

// Covers node rider set behavior: existing braced comma values are wrapped again.
TEST(NodeRiderSetTest, ExistingBracedCommaValuesAreWrappedAgain)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_LIST,policy=always,rfld=contacts"));

  EXPECT_TRUE(set.updateRider("CONTACT_LIST", "{abe,ben}", 100));
  EXPECT_EQ(set.getRiderReports(100), "contacts={{abe,ben}}");
}

// Covers node rider set behavior: missing rider field falls back to MOOS var for all policies.
TEST(NodeRiderSetTest, MissingRiderFieldFallsBackToMoosVarForAllPolicies)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_LIST,policy=always"));
  ASSERT_TRUE(set.addNodeRider("var=CONTACT_RANGE,policy=updated"));
  ASSERT_TRUE(set.addNodeRider("var=NAV_DEPTH,policy=5"));

  EXPECT_TRUE(set.updateRider("CONTACT_LIST", "abe", 100));
  EXPECT_TRUE(set.updateRider("CONTACT_RANGE", "", 42.0, 100));
  EXPECT_TRUE(set.updateRider("NAV_DEPTH", "", 7.0, 100));

  EXPECT_EQ(set.getRiderReports(100), "CONTACT_LIST=abe,CONTACT_RANGE=42.0000,");
  EXPECT_EQ(set.getRiderReports(105), "CONTACT_LIST=abe,,NAV_DEPTH=7.0000");
}

// Covers node rider set behavior: mixed policies report in configured order with empty slots.
TEST(NodeRiderSetTest, MixedPoliciesReportInConfiguredOrderWithEmptySlots)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=ALWAYS_VAL,policy=always,rfld=always"));
  ASSERT_TRUE(set.addNodeRider("var=UPDATED_VAL,policy=updated,rfld=updated"));
  ASSERT_TRUE(set.addNodeRider("var=FREQ_VAL,policy=5,rfld=freq"));

  EXPECT_TRUE(set.updateRider("ALWAYS_VAL", "a", 100));
  EXPECT_TRUE(set.updateRider("UPDATED_VAL", "u", 100));
  EXPECT_TRUE(set.updateRider("FREQ_VAL", "f", 100));

  EXPECT_EQ(set.getRiderReports(100), "always=a,updated=u,");
  EXPECT_EQ(set.getRiderReports(104.9), "always=a,,");
  EXPECT_EQ(set.getRiderReports(105), "always=a,,freq=f");
}

// Covers node rider set behavior: backwards time suppresses frequency but not always reports.
TEST(NodeRiderSetTest, BackwardsTimeSuppressesFrequencyButNotAlwaysReports)
{
  NodeRiderSet set;
  ASSERT_TRUE(set.addNodeRider("var=ALWAYS_VAL,policy=always,rfld=always"));
  ASSERT_TRUE(set.addNodeRider("var=FREQ_VAL,policy=5,rfld=freq"));

  EXPECT_TRUE(set.updateRider("ALWAYS_VAL", "a", 100));
  EXPECT_TRUE(set.updateRider("FREQ_VAL", "f", 100));

  EXPECT_EQ(set.getRiderReports(99), "always=a,");
  EXPECT_EQ(set.getRiderReports(105), "always=a,freq=f");
}
