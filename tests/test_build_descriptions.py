import importlib.util
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("squachemit_site", ROOT / "tools" / "build_installer_site.py")
site = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(site)


class BuildDescriptionTests(unittest.TestCase):
    def setUp(self):
        self.notes = site.load_build_notes()

    def test_known_firmware_builds_have_distinct_descriptive_labels(self):
        builds = [
            ("fb57b628ce6a2539cb0940127763744e47be19bf", "Drone Remote ID", 49),
            ("6c08d0de80c0206a08ba99eaab4c324400a35bff", "S3 Packaging", 31),
            ("e5cf01bf40a796f50750b37b1777da0273da92f1", "RF Test Matrix", 31),
            ("2d6e9f3f29b51b30d1b5731bc7b220e7d020312f", "Touchscreen Baseline", 22),
        ]
        for sha, expected_text, expected_case_count in builds:
            with self.subTest(sha=sha):
                description = site.describe_build(sha, "lab/unknown", "c5-waveshare-28", self.notes)
                self.assertIn(expected_text, description["title"])
                self.assertIn(str(expected_case_count), description["title"])
                self.assertTrue(description["notes_verified_for_sha"])
                self.assertTrue(description["summary"])
                self.assertTrue(description["highlights"])

    def test_future_commit_on_same_branch_is_not_mislabeled_as_known_build(self):
        future_sha = "0" * 40
        desc = site.describe_build(future_sha, "lab/drone-rid-profiles-20261109",
                                  "c5-waveshare-28", self.notes)
        self.assertFalse(desc["notes_verified_for_sha"])
        self.assertIn("review exact changes", desc["title"])
        self.assertNotIn("49", desc["title"])

    def test_main_s3_baseline_is_identified(self):
        sha = "ee7295ac58d3e159036fae76b964644a202d9838"
        desc = site.describe_build(sha, "main", "s3-devkit", self.notes)
        self.assertEqual(desc["version_label"], "v0.1.0")
        self.assertEqual(desc["title"], "ESP32-S3 Baseline (22 tests)")

    def test_catalog_data_is_valid_json(self):
        payload = json.loads((ROOT / "tools" / "firmware_build_notes.json").read_text())
        self.assertEqual(len(payload["builds"]), len(self.notes))
        self.assertGreaterEqual(len(self.notes), 5)


if __name__ == "__main__":
    unittest.main()
