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
            raw_families="",
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

    def test_none_selects_no_harness_targets(self) -> None:
        selected = harness_targets.select_targets(
            self.targets,
            dispatch_mode="none",
            family="",
            raw_families="",
            raw_targets="",
        )

        self.assertEqual(selected, [])

    def test_full_selects_all_harness_targets(self) -> None:
        selected = harness_targets.select_targets(
            self.targets,
            dispatch_mode="full",
            family="",
            raw_families="",
            raw_targets="",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            [target["key"] for target in self.targets],
        )

    def test_specific_harnesses_preserves_requested_order(self) -> None:
        selected = harness_targets.select_targets(
            self.targets,
            dispatch_mode="specific_harnesses",
            family="",
            raw_families="",
            raw_targets="waypoint_h01,cmgr_h01",
        )

        self.assertEqual([target["key"] for target in selected], ["waypoint_h01", "cmgr_h01"])

    def test_batch_family_run_selects_requested_families(self) -> None:
        selected = harness_targets.select_targets(
            self.targets,
            dispatch_mode="batch_family_run",
            family="",
            raw_families="fixedturn_behavior,cutrange_behavior,loiter_behavior",
            raw_targets="",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            ["cutrange_h01", "fixedturn_h01", "loiter_h01"],
        )

    def test_subset_rejects_unknown_target(self) -> None:
        with self.assertRaisesRegex(ValueError, "Unknown target key"):
            harness_targets.select_targets(
                self.targets,
                dispatch_mode="specific_harnesses",
                family="",
                raw_families="",
                raw_targets="missing_target",
            )


if __name__ == "__main__":
    unittest.main()
