import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
HARNESS_ROOT = REPO_ROOT / "harnesses" / "mission_utility_harnesses"
RUNNER = HARNESS_ROOT / "mission_utility_runner.sh"
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


if __name__ == "__main__":
    unittest.main()
