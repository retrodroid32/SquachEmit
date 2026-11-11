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
            ("471699cb08ad7e458b4d56a11e1f5ef265d54ed5", "CYD 3.2", 49),
            ("2774856ec2c0f852eb87b1ec99dfcb910aadca7e", "CYD 3.2", 49),
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

    def test_current_172_case_lab_and_main_notes_are_specific(self):
        lab_sha = "ffc08d1b65bf261ec4dad7bb6b44da05650ad98e"
        lab = site.describe_build(lab_sha, "lab/cyd32-uav-vendor-oui-tests-172-20261110",
                                  "cyd32-st7798", self.notes)
        self.assertTrue(lab['notes_verified_for_sha'])
        self.assertIn('172', lab['title'])
        self.assertIn('TX', lab['summary'])
        self.assertIn('3–30', lab['summary'])
        self.assertNotIn('Uncatalogued', lab['title'])
        main_sha = "09b4b0519eb411391a51a0ea6dc5283428d3f62f"
        main = site.describe_build(main_sha, 'main', 'cyd32-st7798', self.notes)
        self.assertIn('172', main['title'])
        self.assertIn('SHA-256', main['summary'])
        self.assertNotIn('v1.0.0', main['version_label'])

    def test_current_mainline_profile_is_board_specific(self):
        profile = json.loads((ROOT/'tools/active_firmware_profile.json').read_text())
        profile['source_commit_sha'] = 'a' * 40
        versions = set()
        titles = set()
        for board in site.BOARDS:
            with self.subTest(board=board):
                desc = site.describe_source_profile(profile, board, 'a' * 40)
                self.assertTrue(desc['notes_verified_for_sha'])
                self.assertIn('172', desc['title'])
                self.assertIn('MAINLINE', desc['version_label'])
                self.assertIn('aaaaaaaa', desc['version_label'])
                self.assertIn('172', desc['summary'])
                self.assertIn('SHA-256', desc['summary'])
                titles.add(desc['title'])
                versions.add(desc['version_label'])
        self.assertEqual(len(titles), 3)
        self.assertEqual(len(versions), 1)
        with self.assertRaisesRegex(ValueError, 'ref differs'):
            site.describe_source_profile(profile, 'cyd32-st7798', 'b' * 40)

    def test_current_firmware_source_fingerprint_requires_updated_notes(self):
        import subprocess
        profile = json.loads((ROOT/'tools/active_firmware_profile.json').read_text())
        cases = json.loads((ROOT/'tests/cases.json').read_text())
        self.assertEqual(profile['scenario_count'], len(cases))
        self.assertEqual(len(cases), 172)
        for path, expected_sha in profile['firmware_sources'].items():
            with self.subTest(path=path):
                actual_sha = subprocess.check_output(
                    ['git', 'rev-parse', f'HEAD:{path}'], cwd=ROOT, text=True).strip()
                self.assertEqual(actual_sha, expected_sha,
                    'Firmware changed: update version, title and What Changed metadata')

    def test_installer_does_not_alias_current_main_to_tagged_95_case_v1(self):
        builder = (ROOT/'tools/build_installer_site.py').read_text()
        self.assertNotIn('First Public Release', builder)
        self.assertNotIn('95 Test Cases', builder)
        self.assertIn('describe_source_profile(', builder)
        publisher = (ROOT/'.github/workflows/publish-installer.yml').read_text()
        self.assertIn('mainline_profile(sha)', publisher)
        self.assertIn('active_firmware_profile.json?ref={sha}', publisher)
        app = (ROOT/'web-installer/app.js').read_text()
        self.assertIn('MAINLINE CHANNEL', app)
        self.assertNotIn('Uncatalogued firmware', app)

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
