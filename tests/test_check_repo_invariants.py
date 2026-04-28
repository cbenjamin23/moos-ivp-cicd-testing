from __future__ import annotations

import unittest

from scripts import check_repo_invariants


WORKFLOW_SNIPPET = """
on:
  workflow_dispatch:
    inputs:
      dispatch_mode:
        description: "Mode"
        required: true
        type: choice
        default: full
        options:
          - full
          - family_run
      family:
        description: "Family"
        required: false
        type: choice
        default: colregs
        options:
          - cmgr
          - colregs
      targets:
        description: "Targets"
        required: false
        default: "colregs_h01,cmgr_h01"
"""


class CheckRepoInvariantParsingTests(unittest.TestCase):
    def test_workflow_choice_block_reads_named_input_options(self) -> None:
        self.assertEqual(
            check_repo_invariants.workflow_choice_block("dispatch_mode", WORKFLOW_SNIPPET),
            ["full", "family_run"],
        )
        self.assertEqual(
            check_repo_invariants.workflow_choice_block("family", WORKFLOW_SNIPPET),
            ["cmgr", "colregs"],
        )

    def test_workflow_default_reads_named_input_default(self) -> None:
        self.assertEqual(
            check_repo_invariants.workflow_default("dispatch_mode", WORKFLOW_SNIPPET),
            "full",
        )
        self.assertEqual(
            check_repo_invariants.workflow_default("targets", WORKFLOW_SNIPPET),
            "colregs_h01,cmgr_h01",
        )

    def test_comma_list_ignores_empty_items_and_whitespace(self) -> None:
        self.assertEqual(
            check_repo_invariants.comma_list(" cmgr_h01, , colregs_h01 "),
            ["cmgr_h01", "colregs_h01"],
        )


if __name__ == "__main__":
    unittest.main()
