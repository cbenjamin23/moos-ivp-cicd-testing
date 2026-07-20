import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
HARNESS_ROOT = REPO_ROOT / "harnesses" / "mission_utility_harnesses"
RUNNER = HARNESS_ROOT / "mission_utility_runner.sh"
HASH_RESET_PATCH = HARNESS_ROOT / "patches" / "hash-reset-changes-shoreside.xmoos"
LAUNCHERS = (
    HARNESS_ROOT / "H01-pmissioneval_unit" / "zlaunch.sh",
    HARNESS_ROOT / "H02-pmissionhash_unit" / "zlaunch.sh",
    HARNESS_ROOT / "H03-umayfinish_unit" / "zlaunch.sh",
)


class MissionUtilityRunnerContractTests(unittest.TestCase):
    def test_sourcing_runner_is_inert_and_defines_main(self) -> None:
        probe = r'''
set -u
source "$1"
declare -F mission_utility_main >/dev/null
if declare -F usage >/dev/null; then
    exit 9
fi
'''
        completed = subprocess.run(
            ["bash", "-c", probe, "runner-probe", str(RUNNER)],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertEqual(completed.stdout, "")
        self.assertEqual(completed.stderr, "")

    def test_launchers_explicitly_call_shared_runner(self) -> None:
        for launcher in LAUNCHERS:
            with self.subTest(launcher=launcher):
                text = launcher.read_text(encoding="utf-8")
                source_pos = text.index("mission_utility_runner.sh")
                call_pos = text.index('mission_utility_main "$@"')
                self.assertLess(source_pos, call_pos)
                self.assertNotIn("mission_utility_common.sh", text)

    def test_runner_does_not_invoke_itself(self) -> None:
        text = RUNNER.read_text(encoding="utf-8")
        self.assertIn("mission_utility_main() {", text)
        self.assertNotIn('mission_utility_main "$@"', text)

    def test_hash_reset_timing_is_owned_by_the_case_patch(self) -> None:
        runner_text = RUNNER.read_text(encoding="utf-8")
        case_block = runner_text.split("hash_reset_changes_pass)", 1)[1].split(";;", 1)[0]
        self.assertIn(
            'POKE_ARGS="PASS_INPUT=true CASE_VALUE:=hashreset CASE_MAIL:=hashreset"',
            case_block,
        )
        self.assertNotIn("RESET_MHASH", case_block)
        self.assertNotIn("TEST_EVAL_READY", case_block)
        self.assertNotIn("__SLEEP__", case_block)

        patch_text = HASH_RESET_PATCH.read_text(encoding="utf-8")
        self.assertIn("event = var=RESET_MHASH, val=true, time=5", patch_text)
        self.assertIn("event = var=TEST_EVAL_READY, val=true, time=45", patch_text)


if __name__ == "__main__":
    unittest.main()
