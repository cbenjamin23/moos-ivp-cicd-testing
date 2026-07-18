from __future__ import annotations

from pathlib import Path
import shlex
import subprocess
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "scripts" / "moos_scoped_teardown.sh"


class MoosScopedTeardownTests(unittest.TestCase):
    def run_bash(self, body: str, *, cwd: Path) -> subprocess.CompletedProcess[str]:
        script = textwrap.dedent(
            f"""
            set -u
            source {shlex.quote(str(HELPER))}
            {body}
            """
        )
        return subprocess.run(
            ["bash", "-c", script],
            cwd=cwd,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_combined_discovery_fails_when_both_backends_are_unusable(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.run_bash(
                """
                moos_scoped_teardown_pids_for_root_procfs() { return 1; }
                moos_scoped_teardown_pids_for_root_lsof() { return 1; }
                if moos_scoped_teardown_pids_for_root_checked "$PWD"; then
                    exit 9
                fi
                """,
                cwd=Path(tmp),
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("unable to inspect scoped processes", result.stderr)

    def test_wait_and_stop_propagate_discovery_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.run_bash(
                """
                moos_scoped_teardown_pids_for_root() { return 1; }
                if moos_scoped_teardown_wait_clear "$PWD" 1; then
                    exit 8
                fi
                if moos_scoped_teardown_stop_root "$PWD"; then
                    exit 9
                fi
                """,
                cwd=Path(tmp),
            )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_lsof_backend_accepts_a_successful_empty_listing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.run_bash(
                """
                lsof() {
                    return 0
                }
                output=$(moos_scoped_teardown_pids_for_root_lsof "$PWD") || exit 8
                [ -z "$output" ] || exit 9
                """,
                cwd=Path(tmp),
            )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_lsof_backend_finds_known_app_by_scoped_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "case" / "mission").mkdir(parents=True)
            result = self.run_bash(
                """
                lsof() {
                    printf 'p%s\ncbash\nfcwd\nn%s\n' "$$" "$PWD"
                    printf 'p4242\ncpMissionEval\nfcwd\nn%s/case/mission\n' "$PWD"
                }
                output=$(moos_scoped_teardown_pids_for_root_lsof "$PWD") || exit 8
                [ "$output" = 4242 ] || exit 9
                """,
                cwd=root,
            )
        self.assertEqual(result.returncode, 0, result.stderr)

if __name__ == "__main__":
    unittest.main()
