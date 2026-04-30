from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from scripts import check_cpp_tests


class CheckCppTestsParsingTests(unittest.TestCase):
    def test_parse_add_moos_cpp_test_sections_and_suite_labels(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            cmake_path = Path(tmpdir) / "CMakeLists.txt"
            cmake_path.write_text(
                """
add_moos_cpp_test(test_example_target
  SOURCES
    ExampleTest.cpp
  LIBS
    example
  LABELS
    example
    pExample
  SUITE_LABELS
    ExampleTest=Example
    OtherTest=Other,Shared
)
""",
                encoding="utf-8",
            )

            targets = check_cpp_tests.parse_add_moos_cpp_tests(cmake_path)

        self.assertEqual(len(targets), 1)
        self.assertEqual(targets[0].target, "test_example_target")
        self.assertEqual(targets[0].values("SOURCES"), ["ExampleTest.cpp"])
        self.assertEqual(targets[0].values("LABELS"), ["example", "pExample"])
        self.assertEqual(
            targets[0].values("SUITE_LABELS"),
            ["ExampleTest=Example", "OtherTest=Other,Shared"],
        )

    def test_normalize_source_lib_dir_matches_test_bucket_names(self) -> None:
        self.assertEqual(check_cpp_tests.normalize_source_lib_dir("lib_behaviors-marine"), "behaviors_marine")
        self.assertEqual(check_cpp_tests.normalize_source_lib_dir("lib_ivpbuild"), "ivpbuild")

    def test_canonical_area_label_matches_ctest_bucket_label(self) -> None:
        self.assertEqual(check_cpp_tests.canonical_area_label("behaviors_marine"), "behaviors-marine")
        self.assertEqual(check_cpp_tests.canonical_area_label("behaviors_colregs"), "behaviors-colregs")
        self.assertEqual(check_cpp_tests.canonical_area_label("ivpbuild"), "ivpbuild")

    def test_labels_for_ctest_accepts_list_and_scalar_values(self) -> None:
        self.assertEqual(
            check_cpp_tests.labels_for_ctest(
                {"properties": [{"name": "LABELS", "value": ["geometry", "XYArc"]}]}
            ),
            ["geometry", "XYArc"],
        )
        self.assertEqual(
            check_cpp_tests.labels_for_ctest({"properties": [{"name": "LABELS", "value": "geometry"}]}),
            ["geometry"],
        )

    def test_def_source_for_ctest_strips_line_number(self) -> None:
        self.assertEqual(
            check_cpp_tests.def_source_for_ctest(
                {"properties": [{"name": "DEF_SOURCE_LINE", "value": "/tmp/ExampleTest.cpp:42"}]}
            ),
            Path("/tmp/ExampleTest.cpp"),
        )


if __name__ == "__main__":
    unittest.main()
