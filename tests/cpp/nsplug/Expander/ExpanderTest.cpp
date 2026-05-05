#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "Expander.h"
#include "TestFileUtils.h"

namespace {

std::string ReadFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  EXPECT_TRUE(in.is_open());
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string TrimTrailingBlankLines(std::string text)
{
  while(text.size() >= 2 && text.compare(text.size() - 2, 2, "\n\n") == 0)
    text.erase(text.size() - 1);
  return text;
}

bool ExpandToFile(Expander& expander)
{
  if(!expander.expand())
    return false;
  return expander.writeOutput();
}

bool ExpandText(const TempDir& temp,
                const std::string& input,
                std::string& output,
                Expander* configured_expander = nullptr)
{
  const std::string input_path = temp.writeFile("input.txt", input);
  const std::string output_path = temp.filePath("output.txt");
  Expander local_expander(input_path, output_path);
  Expander& expander = configured_expander == nullptr ? local_expander : *configured_expander;
  if(configured_expander != nullptr) {
    expander.setInFile(input_path);
    expander.setOutFile(output_path);
  }
  expander.setForce(true);
  const bool ok = ExpandToFile(expander);
  output = ok ? TrimTrailingBlankLines(ReadFile(output_path)) : "";
  return ok;
}

}  // namespace

// Source audit note: this suite covers nsplug Expander's stable macro,
// include, condition-stack, macro-file, output, path, and validation behavior
// against app_nsplug. CLI parsing and upstream paths that terminate the process
// instead of returning status are intentionally outside these component tests.

// Covers nsplug macro replacement: command-line style macros replace repeated
// $(KEY) occurrences while preserving surrounding text.
TEST(NsplugExpanderMacroTest, ReplacesRepeatedDollarMacros)
{
  TempDir temp("nsplug_macro_repeated");
  std::string output;
  Expander expander;
  expander.addMacro("VNAME", "abe");

  ASSERT_TRUE(ExpandText(temp,
                         "name=$(VNAME)\n"
                         "again=$(VNAME)\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "name=abe\n"
            "again=abe\n");
}

// Covers nsplug uppercase replacement: %(KEY) expands to the uppercase macro
// value while $(KEY) preserves the given value.
TEST(NsplugExpanderMacroTest, PercentMacrosUseUppercaseValue)
{
  TempDir temp("nsplug_macro_upper");
  std::string output;
  Expander expander;
  expander.addMacro("VNAME", "abe");

  ASSERT_TRUE(ExpandText(temp,
                         "lower=$(VNAME)\n"
                         "upper=%(VNAME)\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "lower=abe\n"
            "upper=ABE\n");
}

// Covers nsplug uppercase defaults: %(KEY:=default) uses an uppercase default
// when the macro is absent.
TEST(NsplugExpanderMacroTest, PercentMacroDefaultUsesUppercaseDefaultValue)
{
  TempDir temp("nsplug_macro_upper_default");
  std::string output;

  ASSERT_TRUE(ExpandText(temp,
                         "other=%(OTHER:=henry)\n",
                         output));

  EXPECT_EQ(output, "other=HENRY\n");
}

// Covers nsplug macro defaults: default expressions are used when a macro is
// absent and ignored when the macro is present.
TEST(NsplugExpanderMacroTest, MacroDefaultsApplyOnlyWhenUnset)
{
  TempDir temp("nsplug_macro_defaults");
  std::string output;
  Expander expander;
  expander.addMacro("ROLE", "survey");

  ASSERT_TRUE(ExpandText(temp,
                         "role=$(ROLE:=rescue)\n"
                         "color=$(COLOR:=purple)\n"
                         "name=$(NAME=abe)\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "role=survey\n"
            "color=purple\n"
            "name=abe\n");
}

// Covers nsplug empty macro defaults: KEY= treats an empty-but-present macro as
// set, while KEY:= uses the default when the present value is empty.
TEST(NsplugExpanderMacroTest, MacroDefaultsDistinguishEmptyPresentValues)
{
  TempDir temp("nsplug_macro_empty_defaults");
  std::string output;
  Expander expander;
  expander.addMacro("NAME", "");

  ASSERT_TRUE(ExpandText(temp,
                         "plain=$(NAME)\n"
                         "equals=$(NAME=fred)\n"
                         "colon_equals=$(NAME:=fred)\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "plain=\n"
            "equals=\n"
            "colon_equals=fred\n");
}

// Covers nsplug #define handling: both plain and $(KEY) define syntax are
// supported, and later definitions replace earlier values.
TEST(NsplugExpanderMacroTest, DefineSyntaxSupportsDollarWrappedNamesAndRedefinition)
{
  TempDir temp("nsplug_define");
  std::string output;

  ASSERT_TRUE(ExpandText(temp,
                         "#define FOO first\n"
                         "a=$(FOO)\n"
                         "#define $(FOO) second\n"
                         "b=$(FOO)\n",
                         output));

  EXPECT_EQ(output,
            "a=first\n"
            "b=second\n");
}

// Covers nsplug defined-only macros: a macro without a value is considered
// defined for conditionals and expands to an empty value.
TEST(NsplugExpanderMacroTest, DefinedOnlyMacroExpandsEmptyButPassesIfdef)
{
  TempDir temp("nsplug_defined_only");
  std::string output;

  ASSERT_TRUE(ExpandText(temp,
                         "#define FLAG\n"
                         "#ifdef FLAG\n"
                         "flag=$(FLAG)\n"
                         "#else\n"
                         "missing\n"
                         "#endif\n",
                         output));

  EXPECT_EQ(output, "flag=\n");
}

// Covers nsplug unresolved macro behavior: unresolved macros remain in output
// in the default non-strict mode.
TEST(NsplugExpanderMacroTest, UnresolvedMacrosRemainInNonStrictOutput)
{
  TempDir temp("nsplug_unresolved");
  std::string output;

  ASSERT_TRUE(ExpandText(temp, "value=$(MISSING)\n", output));

  EXPECT_EQ(output, "value=$(MISSING)\n");
}

// Covers nsplug strict mode: unresolved macros abort expansion by exiting with
// failure instead of producing partially expanded output.
TEST(NsplugExpanderMacroTest, StrictModeExitsOnUnresolvedMacro)
{
  TempDir temp("nsplug_strict_unresolved");
  const std::string input_path = temp.writeFile("input.txt", "value=$(MISSING)\n");
  const std::string output_path = temp.filePath("output.txt");

  EXPECT_EXIT(
      {
        Expander expander(input_path, output_path);
        expander.setForce(true);
        expander.setStrict(true);
        expander.expand();
        std::exit(0);
      },
      testing::ExitedWithCode(EXIT_FAILURE),
      "");
}

// Covers strict mode's comment exemption for unresolved macros.
TEST(NsplugExpanderMacroTest, StrictModeAllowsUnresolvedMacrosInComments)
{
  TempDir temp("nsplug_strict_comment_unresolved");
  std::string output;
  Expander expander;
  expander.setStrict(true);

  ASSERT_TRUE(ExpandText(temp,
                         "// comment $(MISSING)\n"
                         "value=ok\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "// comment $(MISSING)\n"
            "value=ok\n");
}

// Covers nsplug macro-file loading: comments are skipped, quotes are stripped,
// and values can reference macros already known to the expander.
TEST(NsplugExpanderMacroFileTest, MacroFileLoadsQuotedAndDerivedValues)
{
  TempDir temp("nsplug_macro_file");
  const std::string macros_path = temp.writeFile(
      "macros.txt",
      "# comment\n"
      "// comment\n"
      "COLOR=\"dark blue\"\n"
      "LABEL=$(VNAME)-lead\n");
  std::string output;
  Expander expander;
  expander.addMacro("VNAME", "abe");
  ASSERT_TRUE(expander.addMacroFile(macros_path));

  ASSERT_TRUE(ExpandText(temp,
                         "color=$(COLOR)\n"
                         "label=$(LABEL)\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "color=dark blue\n"
            "label=abe-lead\n");
}

// Covers nsplug macro-file validation: conflicting duplicate definitions are
// ignored and the first unambiguous value remains active.
TEST(NsplugExpanderMacroFileTest, AmbiguousMacroFileDuplicateKeepsFirstValue)
{
  TempDir temp("nsplug_macro_file_duplicate");
  const std::string macros_path = temp.writeFile(
      "macros.txt",
      "COLOR=red\n"
      "COLOR=blue\n");
  std::string output;
  Expander expander;
  ASSERT_TRUE(expander.addMacroFile(macros_path));

  ASSERT_TRUE(ExpandText(temp, "color=$(COLOR)\n", output, &expander));

  EXPECT_EQ(output, "color=red\n");
}

// Covers nsplug macro-file validation: missing macro files are rejected before
// expansion state is changed.
TEST(NsplugExpanderMacroFileTest, MissingMacroFileIsRejected)
{
  TempDir temp("nsplug_missing_macro_file");
  Expander expander;

  EXPECT_FALSE(expander.addMacroFile(temp.filePath("missing.txt")));
}

// Covers nsplug macro-file validation for empty readable files.
TEST(NsplugExpanderMacroFileTest, EmptyMacroFileIsAcceptedAsNoOp)
{
  TempDir temp("nsplug_empty_macro_file");
  const std::string macros_path = temp.writeFile("macros.txt", "");
  std::string output;
  Expander expander;

  EXPECT_TRUE(expander.addMacroFile(macros_path));
  ASSERT_TRUE(ExpandText(temp, "color=$(COLOR:=blue)\n", output, &expander));
  EXPECT_EQ(output, "color=blue\n");
}

// Covers nsplug #ifdef value matching and #else selection.
TEST(NsplugExpanderConditionalTest, IfdefValueAndElseSelectExpectedBranch)
{
  TempDir temp("nsplug_ifdef_value");
  std::string output;
  Expander expander;
  expander.addMacro("XMODE", "SIM");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef XMODE M300\n"
                         "m300\n"
                         "#else\n"
                         "sim\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "sim\n");
}

// Covers nsplug #elseifdef behavior: only the first matching branch emits.
TEST(NsplugExpanderConditionalTest, ElseIfDefSelectsFirstMatchingBranch)
{
  TempDir temp("nsplug_elseif");
  std::string output;
  Expander expander;
  expander.addMacro("ROLE", "lead");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef ROLE trail\n"
                         "trail\n"
                         "#elseifdef ROLE lead\n"
                         "lead\n"
                         "#else\n"
                         "other\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "lead\n");
}

// Covers nsplug #ifdef value matching: a macro can be compared with another
// macro's value, matching upstream nsplug fixture semantics.
TEST(NsplugExpanderConditionalTest, IfdefCanCompareAgainstAnotherMacroValue)
{
  TempDir temp("nsplug_ifdef_macro_value");
  std::string output;
  Expander expander;
  expander.addMacro("MODE", "SIM");
  expander.addMacro("EXPECTED", "SIM");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef MODE EXPECTED\n"
                         "matched\n"
                         "#else\n"
                         "missed\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "matched\n");
}

// Covers nsplug disjunction and conjunction expressions in #ifdef blocks.
TEST(NsplugExpanderConditionalTest, IfdefSupportsDisjunctionAndConjunction)
{
  TempDir temp("nsplug_logic");
  std::string output;
  Expander expander;
  expander.addMacro("A", "1");
  expander.addMacro("B", "2");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef A 9 || B 2\n"
                         "or-pass\n"
                         "#endif\n"
                         "#ifdef A 1 && B 2\n"
                         "and-pass\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "or-pass\n"
            "and-pass\n");
}

// Covers nsplug mixed logical operators: a mixed && and || #ifdef evaluates
// false in non-strict mode.
TEST(NsplugExpanderConditionalTest, MixedAndOrIfdefEvaluatesFalseInNonStrictMode)
{
  TempDir temp("nsplug_mixed_logic");
  std::string output;
  Expander expander;
  expander.addMacro("A", "1");
  expander.addMacro("B", "2");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef A 1 || B 2 && C 3\n"
                         "bad\n"
                         "#else\n"
                         "fallback\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "fallback\n");
}

// Covers nsplug #ifndef behavior for absent and present macros.
TEST(NsplugExpanderConditionalTest, IfndefSelectsOnlyWhenMacroIsAbsent)
{
  TempDir temp("nsplug_ifndef");
  std::string output;
  Expander expander;
  expander.addMacro("KNOWN", "yes");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifndef UNKNOWN\n"
                         "unknown-absent\n"
                         "#endif\n"
                         "#ifndef KNOWN\n"
                         "known-absent\n"
                         "#else\n"
                         "known-present\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "unknown-absent\n"
            "known-present\n");
}

// Covers nsplug #ifndef multi-clause behavior: every clause must be absent for
// the true branch to emit.
TEST(NsplugExpanderConditionalTest, IfndefRequiresAllClausesToBeAbsent)
{
  TempDir temp("nsplug_ifndef_multi");
  std::string output;
  Expander expander;
  expander.addMacro("KNOWN", "yes");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifndef MISSING ALSO_MISSING\n"
                         "both-absent\n"
                         "#endif\n"
                         "#ifndef MISSING KNOWN\n"
                         "bad\n"
                         "#else\n"
                         "known-present\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "both-absent\n"
            "known-present\n");
}

// Covers nsplug nested conditionals and skip-stack behavior.
TEST(NsplugExpanderConditionalTest, NestedConditionalsRespectOuterSkippedBranch)
{
  TempDir temp("nsplug_nested");
  std::string output;
  Expander expander;
  expander.addMacro("OUTER", "no");
  expander.addMacro("INNER", "yes");

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef OUTER yes\n"
                         "#ifdef INNER yes\n"
                         "bad-inner\n"
                         "#endif\n"
                         "#else\n"
                         "outer-fallback\n"
                         "#endif\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "outer-fallback\n");
}

// Covers skipped preprocessor branches: #define lines in skipped branches do
// not mutate later macro state.
TEST(NsplugExpanderConditionalTest, SkippedDefineDoesNotCreateMacro)
{
  TempDir temp("nsplug_skipped_define");
  std::string output;

  ASSERT_TRUE(ExpandText(temp,
                         "#ifdef ENABLED\n"
                         "#define COLOR red\n"
                         "#endif\n"
                         "color=$(COLOR:=blue)\n",
                         output));

  EXPECT_EQ(output, "color=blue\n");
}

// Covers nsplug conditional syntax errors: dangling #else fails expansion.
TEST(NsplugExpanderConditionalTest, DanglingElseFailsExpansion)
{
  TempDir temp("nsplug_dangling_else");
  std::string output;

  EXPECT_FALSE(ExpandText(temp,
                          "#else\n"
                          "bad\n",
                          output));
}

// Covers nsplug conditional syntax errors: dangling #elseifdef fails expansion.
TEST(NsplugExpanderConditionalTest, DanglingElseIfDefFailsExpansion)
{
  TempDir temp("nsplug_dangling_elseif");
  std::string output;

  EXPECT_FALSE(ExpandText(temp,
                          "#elseifdef MODE sim\n"
                          "bad\n",
                          output));
}

// Covers nsplug conditional syntax errors: dangling #endif fails expansion.
TEST(NsplugExpanderConditionalTest, DanglingEndifFailsExpansion)
{
  TempDir temp("nsplug_dangling_endif");
  std::string output;

  EXPECT_FALSE(ExpandText(temp,
                          "#endif\n"
                          "bad\n",
                          output));
}

// Covers nsplug includes: quoted include paths are resolved through explicit
// search paths and expanded into the parent output.
TEST(NsplugExpanderIncludeTest, QuotedIncludeUsesSearchPath)
{
  TempDir temp("nsplug_include_path");
  const std::string include_dir = temp.filePath("includes");
  ASSERT_EQ(mkdir(include_dir.c_str(), 0700), 0);
  {
    std::ofstream out((include_dir + "/plug.txt").c_str());
    ASSERT_TRUE(out.is_open());
    out << "from include $(VNAME)";
  }
  std::string output;
  Expander expander;
  expander.addPath(include_dir);
  expander.addMacro("VNAME", "abe");

  ASSERT_TRUE(ExpandText(temp,
                         "before\n"
                         "#include \"plug.txt\"\n"
                         "after\n",
                         output,
                         &expander));

  EXPECT_EQ(output,
            "before\n"
            "from include abe\n"
            "after\n");
}

// Covers nsplug path lists: colon-separated search paths are traversed in
// order, allowing includes to be found outside the first directory.
TEST(NsplugExpanderIncludeTest, ColonSeparatedIncludePathFindsSecondDirectory)
{
  TempDir temp("nsplug_include_colon_path");
  const std::string first_dir = temp.filePath("first");
  const std::string second_dir = temp.filePath("second");
  ASSERT_EQ(mkdir(first_dir.c_str(), 0700), 0);
  ASSERT_EQ(mkdir(second_dir.c_str(), 0700), 0);
  {
    std::ofstream out((second_dir + "/plug.txt").c_str());
    ASSERT_TRUE(out.is_open());
    out << "from second\n";
  }
  std::string output;
  Expander expander;
  expander.addPath(first_dir + ":" + second_dir);

  ASSERT_TRUE(ExpandText(temp,
                         "#include plug.txt\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "from second\n");
}

// Covers nsplug include filename expansion: macros are expanded before include
// lookup.
TEST(NsplugExpanderIncludeTest, IncludeFilenameCanContainMacro)
{
  TempDir temp("nsplug_include_macro");
  temp.writeFile("plug_abe.txt", "hello\n");
  std::string output;
  Expander expander;
  expander.addPath(temp.path());
  expander.addMacro("VNAME", "abe");

  ASSERT_TRUE(ExpandText(temp,
                         "#include plug_$(VNAME).txt\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "hello\n");
}

// Covers nsplug include tags: a tagged include only emits lines under the
// requested <tag> section.
TEST(NsplugExpanderIncludeTest, IncludeCanSelectTaggedSection)
{
  TempDir temp("nsplug_include_tag");
  temp.writeFile("plugs.txt",
                 "<tag> alpha\n"
                 "alpha-line\n"
                 "<tag> bravo\n"
                 "bravo-line\n");
  std::string output;
  Expander expander;
  expander.addPath(temp.path());

  ASSERT_TRUE(ExpandText(temp,
                         "#include plugs.txt <bravo>\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "bravo-line\n");
}

// Covers nsplug tagged includes: preprocessor-style lines inside a selected
// include tag are treated as tag comments and not as active directives.
TEST(NsplugExpanderIncludeTest, TaggedIncludeSkipsHashLines)
{
  TempDir temp("nsplug_include_tag_hash_lines");
  temp.writeFile("plugs.txt",
                 "<tag> wanted\n"
                 "#define LOCAL bad\n"
                 "line=$(LOCAL:=fallback)\n");
  std::string output;
  Expander expander;
  expander.addPath(temp.path());

  ASSERT_TRUE(ExpandText(temp,
                         "#include plugs.txt <wanted>\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "line=fallback\n");
}

// Covers nsplug top-level tag filtering through setIncTag.
TEST(NsplugExpanderIncludeTest, TopLevelIncTagFiltersInputFile)
{
  TempDir temp("nsplug_top_tag");
  std::string output;
  Expander expander;
  expander.setIncTag("wanted");

  ASSERT_TRUE(ExpandText(temp,
                         "<tag> skip\n"
                         "skip-line\n"
                         "<tag> wanted\n"
                         "wanted-line\n",
                         output,
                         &expander));

  EXPECT_EQ(output, "wanted-line\n");
}

// Recursive include loops are intentionally not registered as a normal passing
// CTest case here because the current upstream Expander implementation crashes
// on that path instead of returning a clean failure.

// Covers nsplug missing include validation.
TEST(NsplugExpanderIncludeTest, MissingIncludeFailsExpansion)
{
  TempDir temp("nsplug_missing_include");
  std::string output;

  EXPECT_FALSE(ExpandText(temp,
                          "#include missing.txt\n",
                          output));
}

// Covers nsplug xfile behavior: setXFile uses the top-level input file with an
// x suffix when present.
TEST(NsplugExpanderFileTest, XFileUsesTopLevelXVariantWhenPresent)
{
  TempDir temp("nsplug_xfile");
  const std::string input_path = temp.writeFile("meta.moos", "plain\n");
  temp.writeFile("meta.moosx", "xvariant\n");
  const std::string output_path = temp.filePath("target.moos");
  Expander expander(input_path, output_path);
  expander.setForce(true);
  expander.setXFile(true);

  ASSERT_TRUE(ExpandToFile(expander));

  EXPECT_EQ(TrimTrailingBlankLines(ReadFile(output_path)), "xvariant\n");
}

// Covers nsplug xfile behavior: when no x-suffixed input exists, expansion
// falls back to the requested top-level input file.
TEST(NsplugExpanderFileTest, XFileFallsBackToPlainInputWhenVariantMissing)
{
  TempDir temp("nsplug_xfile_fallback");
  const std::string input_path = temp.writeFile("meta.moos", "plain\n");
  const std::string output_path = temp.filePath("target.moos");
  Expander expander(input_path, output_path);
  expander.setForce(true);
  expander.setXFile(true);

  ASSERT_TRUE(ExpandToFile(expander));

  EXPECT_EQ(TrimTrailingBlankLines(ReadFile(output_path)), "plain\n");
}

// Covers nsplug write behavior: existing output files are overwritten when
// force mode is enabled.
TEST(NsplugExpanderFileTest, ForceAllowsOverwritingExistingOutput)
{
  TempDir temp("nsplug_force_output");
  const std::string input_path = temp.writeFile("input.txt", "fresh\n");
  const std::string output_path = temp.writeFile("output.txt", "stale\n");
  Expander expander(input_path, output_path);
  expander.setForce(true);

  ASSERT_TRUE(ExpandToFile(expander));

  EXPECT_EQ(TrimTrailingBlankLines(ReadFile(output_path)), "fresh\n");
}

// Covers nsplug empty-file behavior: an empty readable input expands
// successfully and writes the current single blank-line output.
TEST(NsplugExpanderFileTest, EmptyInputExpandsToSingleBlankLine)
{
  TempDir temp("nsplug_empty_input");
  const std::string input_path = temp.writeFile("input.txt", "");
  const std::string output_path = temp.filePath("output.txt");
  Expander expander(input_path, output_path);
  expander.setForce(true);

  ASSERT_TRUE(ExpandToFile(expander));

  EXPECT_EQ(ReadFile(output_path), "\n");
}

// Covers nsplug input validation.
TEST(NsplugExpanderFileTest, VerifyInfileRejectsMissingPath)
{
  TempDir temp("nsplug_verify");
  Expander expander(temp.filePath("missing.txt"), temp.filePath("out.txt"));

  EXPECT_FALSE(expander.verifyInfile());
}

// Covers nsplug #define validation.
TEST(NsplugExpanderFileTest, DefineWithoutNameFailsExpansion)
{
  TempDir temp("nsplug_define_without_name");
  std::string output;

  EXPECT_FALSE(ExpandText(temp,
                          "#define\n"
                          "bad\n",
                          output));
}
