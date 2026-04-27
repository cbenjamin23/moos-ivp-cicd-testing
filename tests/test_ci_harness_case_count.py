from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from scripts.ci_harness_case_count import derive_case_count


class HarnessCaseCountTests(unittest.TestCase):
    def write_zlaunch(self, text: str) -> Path:
        tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(tmpdir.cleanup)
        path = Path(tmpdir.name) / "zlaunch.sh"
        path.write_text(text, encoding="utf-8")
        return path

    def test_plain_cases_assignment(self) -> None:
        path = self.write_zlaunch('CASES="alpha_pass beta_pass gamma_fail"\n')

        self.assertEqual(derive_case_count(path), 3)

    def test_composed_case_variables(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    'HEADON_CASES="head_on_pass head_on_fail"',
                    'CROSSING_CASES="crossing_port_pass crossing_starboard_pass"',
                    'CASES="$HEADON_CASES $CROSSING_CASES"',
                ]
            )
        )

        self.assertEqual(derive_case_count(path), 4)

    def test_uses_largest_default_cases_assignment(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    'if [ "$CASE" != "" ]; then',
                    '    CASES="$CASE"',
                    "else",
                    '    CASES="one two three"',
                    "fi",
                ]
            )
        )

        self.assertEqual(derive_case_count(path), 3)

    def test_all_cases_array_fallback(self) -> None:
        path = self.write_zlaunch(
            '\n'.join(
                [
                    "ALL_CASES=(",
                    "    startup_no_warning_pass",
                    "    static_shadow_pass",
                    "    bad_decay_fail",
                    ")",
                    'RUN_CASES=("${ALL_CASES[@]}")',
                ]
            )
        )

        self.assertEqual(derive_case_count(path), 3)


if __name__ == "__main__":
    unittest.main()
