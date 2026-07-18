from __future__ import annotations

from pathlib import Path
import unittest

from scripts.generate_context_graph import is_generated_runtime_artifact


class ContextGraphPathTests(unittest.TestCase):
    def test_modern_run_root_is_runtime_artifact(self) -> None:
        path = Path("harnesses/family/H01-case/.harness_runs/run_1/case_000/mission/zlaunch.sh")
        self.assertTrue(is_generated_runtime_artifact(path))

    def test_legacy_parallel_root_is_runtime_artifact(self) -> None:
        path = Path("harnesses/family/H01-case/.parallel_case_ABC123/mission/zlaunch.sh")
        self.assertTrue(is_generated_runtime_artifact(path))

    def test_moos_log_directory_is_runtime_artifact(self) -> None:
        path = Path("missions/family/stem/LOG_ABE_timestamp/LOG_ABE_timestamp.alog")
        self.assertTrue(is_generated_runtime_artifact(path))

    def test_last_opened_log_marker_is_runtime_artifact(self) -> None:
        path = Path("missions/family/stem/.LastOpenedMOOSLogDirectory")
        self.assertTrue(is_generated_runtime_artifact(path))

    def test_configured_harness_launcher_is_not_runtime_artifact(self) -> None:
        path = Path("harnesses/family/H01-case/zlaunch.sh")
        self.assertFalse(is_generated_runtime_artifact(path))


if __name__ == "__main__":
    unittest.main()
