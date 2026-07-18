from __future__ import annotations

import contextlib
import io
import unittest

from scripts import moos_ivp_build_matrix


class MoosIvpBuildMatrixTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.targets = moos_ivp_build_matrix.load_targets()
        moos_ivp_build_matrix.validate_targets(cls.targets)

    def test_modern_selects_all_initial_targets(self) -> None:
        selected = moos_ivp_build_matrix.select_targets(
            self.targets,
            target_set="modern",
            raw_targets="",
            build_profile="headless",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            [
                "ubuntu_2604_gcc",
                "ubuntu_2604_clang",
                "ubuntu_2404_gcc",
                "ubuntu_2404_clang",
                "debian_13_gcc",
                "debian_12_gcc",
                "rocky_10_gcc",
                "rocky_9_gcc",
            ],
        )

    def test_modern_full_selects_gui_capable_targets(self) -> None:
        selected = moos_ivp_build_matrix.select_targets(
            self.targets,
            target_set="modern",
            raw_targets="",
            build_profile="full",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            [
                "ubuntu_2604_gcc",
                "ubuntu_2604_clang",
                "ubuntu_2404_gcc",
                "ubuntu_2404_clang",
                "debian_13_gcc",
                "debian_12_gcc",
                "rocky_10_gcc",
            ],
        )

    def test_compiler_family_selection(self) -> None:
        selected = moos_ivp_build_matrix.select_targets(
            self.targets,
            target_set="clang",
            raw_targets="",
            build_profile="full",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            ["ubuntu_2604_clang", "ubuntu_2404_clang"],
        )

    def test_current_selects_newest_generation_targets(self) -> None:
        selected = moos_ivp_build_matrix.select_targets(
            self.targets,
            target_set="current",
            raw_targets="",
            build_profile="full",
        )

        self.assertEqual(
            [target["key"] for target in selected],
            [
                "ubuntu_2604_gcc",
                "ubuntu_2604_clang",
                "debian_13_gcc",
                "rocky_10_gcc",
            ],
        )

    def test_specific_targets_preserves_requested_order(self) -> None:
        selected = moos_ivp_build_matrix.select_targets(
            self.targets,
            target_set="specific_targets",
            raw_targets="rocky_9_gcc,ubuntu_2404_gcc",
            build_profile="headless",
        )

        self.assertEqual([target["key"] for target in selected], ["rocky_9_gcc", "ubuntu_2404_gcc"])

    def test_specific_targets_rejects_unknown_target(self) -> None:
        with self.assertRaisesRegex(ValueError, "Unknown build target key"):
            moos_ivp_build_matrix.select_targets(
                self.targets,
                target_set="specific_targets",
                raw_targets="missing_target",
                build_profile="full",
            )

    def test_specific_targets_rejects_unsupported_profile(self) -> None:
        with self.assertRaisesRegex(ValueError, "not supported"):
            moos_ivp_build_matrix.select_targets(
                self.targets,
                target_set="specific_targets",
                raw_targets="rocky_9_gcc",
                build_profile="full",
            )

    def test_target_rows_are_tab_separated_for_local_wrapper(self) -> None:
        selected = moos_ivp_build_matrix.select_targets(
            self.targets,
            target_set="specific_targets",
            raw_targets="ubuntu_2404_gcc",
            build_profile="full",
        )

        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            moos_ivp_build_matrix.print_target_rows(selected)

        self.assertEqual(output.getvalue(), "ubuntu_2404_gcc\tubuntu:24.04\tgcc\n")


if __name__ == "__main__":
    unittest.main()
