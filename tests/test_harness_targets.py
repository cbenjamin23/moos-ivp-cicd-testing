from __future__ import annotations

import unittest

from scripts import harness_targets


class HarnessTargetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.targets = harness_targets.load_targets()
        harness_targets.validate_targets(cls.targets)

    def test_family_run_selects_all_family_targets(self) -> None:
        selected = harness_targets.select_targets(
            self.targets,
            dispatch_mode="family_run",
            family="colregs",
            raw_targets="",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            [
                "colregs_h01",
                "colregs_h02",
                "colregs_h03",
                "colregs_h04",
                "p02_colregs",
                "p03_colregs",
            ],
        )

    def test_subset_preserves_requested_order(self) -> None:
        selected = harness_targets.select_targets(
            self.targets,
            dispatch_mode="just_make_subset",
            family="",
            raw_targets="waypoint_h01,cmgr_h01",
        )

        self.assertEqual([target["key"] for target in selected], ["waypoint_h01", "cmgr_h01"])

    def test_subset_rejects_unknown_target(self) -> None:
        with self.assertRaisesRegex(ValueError, "Unknown target key"):
            harness_targets.select_targets(
                self.targets,
                dispatch_mode="correctness_subset",
                family="",
                raw_targets="missing_target",
            )


if __name__ == "__main__":
    unittest.main()
