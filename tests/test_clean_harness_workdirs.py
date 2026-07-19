from __future__ import annotations

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT = REPO_ROOT / "scripts" / "clean_harness_workdirs.sh"


class CleanHarnessWorkdirsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.repo = Path(self.temp_dir.name)
        (self.repo / "scripts").mkdir()
        (self.repo / "config").mkdir()
        (self.repo / "harnesses" / "alpha_harnesses" / "H01-alpha").mkdir(parents=True)
        (self.repo / "harnesses" / "beta_harnesses" / "H01-beta").mkdir(parents=True)
        shutil.copy2(SCRIPT, self.repo / "scripts" / SCRIPT.name)
        (self.repo / "scripts" / "moos_scoped_teardown.sh").write_text(
            "moos_scoped_teardown_pids_for_root_checked() { return 0; }\n"
            "moos_scoped_teardown_stop_root() { return 0; }\n",
            encoding="utf-8",
        )
        targets = [
            {
                "key": "alpha_h01",
                "path": "harnesses/alpha_harnesses/H01-alpha",
                "harness": "H01-alpha",
                "families": ["alpha"],
            },
            {
                "key": "beta_h01",
                "path": "harnesses/beta_harnesses/H01-beta",
                "harness": "H01-beta",
                "families": ["beta"],
            },
        ]
        (self.repo / "config" / "harness_targets.json").write_text(
            json.dumps(targets), encoding="utf-8"
        )
        for harness in (self.alpha, self.beta):
            (harness / "zlaunch.sh").write_text("#!/bin/bash\n", encoding="utf-8")

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    @property
    def alpha(self) -> Path:
        return self.repo / "harnesses" / "alpha_harnesses" / "H01-alpha"

    @property
    def beta(self) -> Path:
        return self.repo / "harnesses" / "beta_harnesses" / "H01-beta"

    def run_cleaner(self, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(self.repo / "scripts" / SCRIPT.name), *args],
            check=False,
            text=True,
            capture_output=True,
        )

    def make_run(self, harness: Path, container: str, name: str = "run_001") -> Path:
        run_root = harness / container / name
        run_root.mkdir(parents=True)
        (run_root / "evidence.txt").write_text("retained\n", encoding="utf-8")
        return run_root

    def test_default_reports_without_deleting(self) -> None:
        run_root = self.make_run(self.alpha, ".harness_runs")

        result = self.run_cleaner()

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"path={run_root.resolve()}", result.stdout)
        self.assertIn("SUMMARY mode=report retained=1", result.stdout)
        self.assertTrue(run_root.is_dir())

    def test_delete_all_handles_modern_performance_and_legacy_roots(self) -> None:
        modern = self.make_run(self.alpha, ".harness_runs", "run_modern")
        scratch = self.make_run(self.alpha, ".harness_runs", "scratch_probe.ABC123")
        readiness = self.make_run(
            self.alpha, ".harness_runs", "readiness_scratch_20260718"
        )
        performance = self.make_run(self.alpha, "workdirs", "run_performance")
        legacy = self.alpha / ".parallel_alpha_ABC123"
        legacy.mkdir()
        (legacy / "evidence.txt").write_text("retained\n", encoding="utf-8")

        result = self.run_cleaner("--delete", "--all", "--target=alpha_h01")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("SUMMARY mode=delete retained=5", result.stdout)
        self.assertFalse(modern.exists())
        self.assertFalse(scratch.exists())
        self.assertFalse(readiness.exists())
        self.assertFalse(performance.exists())
        self.assertFalse(legacy.exists())
        self.assertFalse((self.alpha / ".harness_runs").exists())
        self.assertFalse((self.alpha / "workdirs").exists())

    def test_excluded_target_is_preserved(self) -> None:
        alpha_run = self.make_run(self.alpha, ".harness_runs")
        beta_run = self.make_run(self.beta, ".harness_runs")

        result = self.run_cleaner("--delete", "--all", "--exclude=beta_h01")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(alpha_run.exists())
        self.assertTrue(beta_run.is_dir())
        self.assertNotIn(str(beta_run), result.stdout)

    def test_older_than_filters_recent_roots(self) -> None:
        old_root = self.make_run(self.alpha, ".harness_runs", "run_old")
        recent_root = self.make_run(self.alpha, ".harness_runs", "run_recent")
        old_time = time.time() - 10 * 86400
        os.utime(old_root, (old_time, old_time))

        result = self.run_cleaner("--delete", "--older-than=7", "--target=alpha_h01")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(old_root.exists())
        self.assertTrue(recent_root.is_dir())

    def test_delete_requires_explicit_age_selection(self) -> None:
        result = self.run_cleaner("--delete")

        self.assertEqual(result.returncode, 2)
        self.assertIn("--delete requires --all or --older-than", result.stderr)


if __name__ == "__main__":
    unittest.main()
