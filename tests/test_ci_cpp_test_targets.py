from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from scripts import ci_cpp_test_targets


class CppTestTargetSummaryTests(unittest.TestCase):
    def write_report(self, text: str) -> Path:
        tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(tmpdir.cleanup)
        path = Path(tmpdir.name) / "report.xml"
        path.write_text(text, encoding="utf-8")
        return path

    def test_selected_targets_accepts_one_or_comma_list(self) -> None:
        self.assertEqual(ci_cpp_test_targets.selected_targets("geometry"), ["geometry"])
        self.assertEqual(
            ci_cpp_test_targets.selected_targets(" geometry, ivpbuild ,, mbutil "),
            ["geometry", "ivpbuild", "mbutil"],
        )

    def test_selected_targets_defaults_empty_selection_to_all(self) -> None:
        self.assertEqual(ci_cpp_test_targets.selected_targets(" , "), ["all"])

    def test_select_targets_for_mode_matches_workflow_modes(self) -> None:
        self.assertEqual(
            ci_cpp_test_targets.select_targets_for_mode("none", "", ""),
            [],
        )
        self.assertEqual(
            ci_cpp_test_targets.select_targets_for_mode("all", "", ""),
            ["all"],
        )
        self.assertEqual(
            ci_cpp_test_targets.select_targets_for_mode("family_run", "geometry", ""),
            ["geometry"],
        )
        self.assertEqual(
            ci_cpp_test_targets.select_targets_for_mode(
                "batch_family_run",
                "",
                "geometry,ivpbuild",
            ),
            ["geometry", "ivpbuild"],
        )

    def test_select_targets_for_mode_rejects_unknown_family(self) -> None:
        with self.assertRaisesRegex(ValueError, "Unknown C\\+\\+ test family"):
            ci_cpp_test_targets.select_targets_for_mode("family_run", "missing", "")

    def test_safe_filename_replaces_regex_characters(self) -> None:
        self.assertEqual(ci_cpp_test_targets.safe_filename("^behaviors$"), "behaviors")
        self.assertEqual(ci_cpp_test_targets.safe_filename("BHV_Waypoint"), "BHV_Waypoint")

    def test_matrix_for_targets_builds_github_include_matrix(self) -> None:
        self.assertEqual(
            ci_cpp_test_targets.matrix_for_targets("geometry,ivpbuild"),
            {
                "include": [
                    {
                        "target": "geometry",
                        "artifact": "cpp-unit-test-report-geometry",
                    },
                    {
                        "target": "ivpbuild",
                        "artifact": "cpp-unit-test-report-ivpbuild",
                    },
                ]
            },
        )

    def test_matrix_for_selected_targets_allows_empty_none_matrix(self) -> None:
        self.assertEqual(
            ci_cpp_test_targets.matrix_for_selected_targets([]),
            {"include": []},
        )

    def test_all_ctest_target_runs_without_label_filter(self) -> None:
        command = ci_cpp_test_targets.ctest_command(
            Path("build"),
            "all",
            Path("reports/all.xml"),
        )

        self.assertNotIn("-L", command)

    def test_parse_junit_report_counts_failures_errors_and_skips(self) -> None:
        report = self.write_report(
            """<?xml version="1.0"?>
<testsuite tests="4" failures="1" errors="1" skipped="1">
  <testcase classname="suite" name="passes"/>
  <testcase classname="suite" name="fails"><failure message="bad value"/></testcase>
  <testcase classname="suite" name="errors"><error>crashed</error></testcase>
  <testcase classname="suite" name="skips"><skipped/></testcase>
</testsuite>
"""
        )

        tests, failures, errors, skipped, issues, report_issue = (
            ci_cpp_test_targets.parse_junit_report(report)
        )

        self.assertEqual((tests, failures, errors, skipped), (4, 1, 1, 1))
        self.assertEqual(report_issue, "")
        self.assertEqual([issue.name for issue in issues], ["suite.fails", "suite.errors"])
        self.assertEqual([issue.status for issue in issues], ["failure", "error"])

    def test_build_summary_includes_failing_case_table(self) -> None:
        result = ci_cpp_test_targets.TargetResult(
            target="geometry",
            report_path=Path("build/cpp-unit-test-reports/geometry.xml"),
            command=["ctest", "--test-dir", "build", "-L", "geometry"],
            returncode=8,
            tests=2,
            failures=1,
            errors=0,
            skipped=0,
            issues=[
                ci_cpp_test_targets.CaseIssue(
                    name="suite.fails",
                    status="failure",
                    message="bad value",
                )
            ],
        )

        summary = ci_cpp_test_targets.build_summary([result])

        self.assertIn("| `geometry` | `fail` | 2 | 1 | 0 | 0 |", summary)
        self.assertIn("#### Failing Cases", summary)
        self.assertIn("`suite.fails`", summary)


if __name__ == "__main__":
    unittest.main()
