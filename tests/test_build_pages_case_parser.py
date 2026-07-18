from __future__ import annotations

import unittest

from docs.tools.build_pages import zlaunch_all_cases


class BuildPagesCaseParserTests(unittest.TestCase):
    def test_modern_cases_array_is_supported(self) -> None:
        self.assertEqual(
            zlaunch_all_cases(
                """
CASES=(
  alpha_pass
  beta_fail
)
"""
            ),
            ["alpha_pass", "beta_fail"],
        )

    def test_all_cases_remains_preferred_when_both_exist(self) -> None:
        self.assertEqual(
            zlaunch_all_cases(
                """
CASES=(modern_pass)
ALL_CASES=(legacy_pass)
"""
            ),
            ["legacy_pass"],
        )

    def test_composed_case_arrays_expand_in_order(self) -> None:
        self.assertEqual(
            zlaunch_all_cases(
                """
GROUP_ONE_CASES=(first_pass second_fail)
GROUP_TWO_CASES=(third_absent)
CASES=(
  "${GROUP_ONE_CASES[@]}"
  "${GROUP_TWO_CASES[@]}"
)
"""
            ),
            ["first_pass", "second_fail", "third_absent"],
        )


if __name__ == "__main__":
    unittest.main()
