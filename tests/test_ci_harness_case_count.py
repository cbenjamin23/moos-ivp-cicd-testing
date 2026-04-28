from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from scripts.ci_harness_case_count import derive_case_count, derive_case_order


class HarnessCaseCountTests(unittest.TestCase):
    def write_zlaunch(self, text: str) -> Path:
        tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(tmpdir.cleanup)
        path = Path(tmpdir.name) / "zlaunch.sh"
        path.write_text(text, encoding="utf-8")
        return path

    def test_all_cases_array(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    "ALL_CASES=(",
                    "    alpha_pass",
                    "    beta_pass",
                    "    gamma_fail",
                    ")",
                ]
            )
        )

        self.assertEqual(derive_case_count(path), 3)
        self.assertEqual(derive_case_order(path), ["alpha_pass", "beta_pass", "gamma_fail"])

    def test_composed_group_arrays(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    "GROUP_HEADON_CASES=(",
                    "    head_on_pass",
                    "    head_on_fail",
                    ")",
                    "GROUP_CROSSING_CASES=(",
                    "    crossing_port_pass",
                    "    crossing_starboard_pass",
                    ")",
                    "ALL_CASES=(",
                    '    "${GROUP_HEADON_CASES[@]}"',
                    '    "${GROUP_CROSSING_CASES[@]}"',
                    ")",
                ]
            )
        )

        self.assertEqual(derive_case_count(path), 4)
        self.assertEqual(
            derive_case_order(path),
            [
                "head_on_pass",
                "head_on_fail",
                "crossing_port_pass",
                "crossing_starboard_pass",
            ],
        )

    def test_run_cases_subset_is_ignored_for_full_count(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    "ALL_CASES=(",
                    "    one_pass",
                    "    two_pass",
                    "    three_fail",
                    ")",
                    'RUN_CASES=("${ALL_CASES[@]}")',
                    'RUN_CASES=("one_pass")',
                ]
            )
        )

        self.assertEqual(derive_case_count(path), 3)

    def test_requires_all_cases_array(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    "RUN_CASES=(",
                    "    one_pass",
                    ")",
                ]
            )
        )

        with self.assertRaises(ValueError):
            derive_case_count(path)


if __name__ == "__main__":
    unittest.main()
