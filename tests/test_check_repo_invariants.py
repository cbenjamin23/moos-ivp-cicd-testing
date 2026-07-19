from __future__ import annotations

import unittest

from scripts import check_repo_invariants


class CheckRepoInvariantParsingTests(unittest.TestCase):
    def test_gui_contract_requires_explicit_forwarding_and_case_guard(self) -> None:
        old_launcher = """
        --gui) DISPLAY_ARGS=() ;;
        """
        self.assertEqual(
            check_repo_invariants.harness_gui_contract_issues(old_launcher),
            [
                "--gui must forward an explicit --gui argument to the stem mission",
                "--gui must require one explicit --case",
            ],
        )

    def test_gui_contract_accepts_explicit_forwarding_and_case_guard(self) -> None:
        fixed_launcher = """
        --gui) DISPLAY_ARGS=(--gui) ;;
        if [ "${DISPLAY_ARGS[0]}" = --gui ] && [ "$total" -ne 1 ]; then
        """
        self.assertEqual(
            check_repo_invariants.harness_gui_contract_issues(fixed_launcher),
            [],
        )

    def test_gui_contract_ignores_headless_only_harnesses(self) -> None:
        self.assertEqual(
            check_repo_invariants.harness_gui_contract_issues("--nogui|-ng) DISPLAY_ARGS=(--nogui) ;;"),
            [],
        )

    def test_mission_launch_args_contract_rejects_direct_empty_array_expansion(self) -> None:
        broken_launcher = '''
        set -u
        launch_args=(--max_time="$MAX_TIME" "${DISPLAY_ARGS[@]}" "${FLOW_ARGS[@]}" "$TIME_WARP")
        '''
        self.assertEqual(
            check_repo_invariants.mission_launch_args_contract_issues(broken_launcher),
            [
                "launch_args must append optional arrays only after a nonempty length guard "
                "for Bash 3.2 set -u compatibility"
            ],
        )

    def test_mission_launch_args_contract_accepts_guarded_array_appends(self) -> None:
        fixed_launcher = '''
        set -u
        launch_args=(--max_time="$MAX_TIME")
        if [ "${#DISPLAY_ARGS[@]}" -gt 0 ]; then
            launch_args+=("${DISPLAY_ARGS[@]}")
        fi
        '''
        self.assertEqual(
            check_repo_invariants.mission_launch_args_contract_issues(fixed_launcher),
            [],
        )

    def test_mission_launch_args_contract_checks_multiline_initializers(self) -> None:
        broken_launcher = '''
        set -u
        launch_args=(
            --max_time="$MAX_TIME"
            "${FLOW_ARGS[@]}"
            "$TIME_WARP"
        )
        '''
        self.assertEqual(
            len(check_repo_invariants.mission_launch_args_contract_issues(broken_launcher)),
            1,
        )


if __name__ == "__main__":
    unittest.main()
