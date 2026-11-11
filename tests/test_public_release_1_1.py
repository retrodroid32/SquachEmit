"""Guard public v1.1.0 metadata and CI-backed firmware release workflow."""
import json
import re
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
VERSION='v1.1.0'
TAGS=('CYD32-ST7798','WAVESHARE-C5-28','ESP32-S3')


class PublicReleaseTests(unittest.TestCase):
    def test_public_version_is_visible_on_every_board(self):
        source=(ROOT/'src/main.cpp').read_text(encoding='utf-8')
        for signature in ('BOOT,SquachEmit,v1.1.0,ESP32-C5-Waveshare-2.8',
                          'BOOT,SquachEmit,v1.1.0,ESP32-2432S032R-ST7798',
                          'BOOT,SquachEmit,v1.1.0,ESP32-S3'):
            self.assertIn(signature,source)
        self.assertEqual(source.count('BOOT,SquachEmit,v1.1.0'),3)

    def test_profile_versions_match_release_name_and_test_count(self):
        p=json.loads((ROOT/'tools/active_firmware_profile.json').read_text())
        self.assertEqual(p['scenario_count'],172)
        self.assertEqual(len(json.loads((ROOT/'tests/cases.json').read_text())),172)
        self.assertEqual(set(p['builds']),{'cyd32-st7798','c5-waveshare-28','s3-devkit'})
        for board,record in p['builds'].items():
            with self.subTest(board=board):
                self.assertIn(VERSION,record['title'])
                self.assertIn(VERSION,record['version'])
                self.assertIn('172',record['summary'])
                self.assertIn('SHA-256',record['summary'])
                self.assertGreaterEqual(len(record['highlights']),4)

    def test_release_ci_gate_and_board_archives(self):
        f=(ROOT/'.github/workflows/release-v1.1.yml').read_text()
        self.assertIn('Publish SquachEmit v1.1.0',f)
        self.assertIn("github.event.workflow_run.conclusion == 'success'",f)
        self.assertIn("github.event.workflow_run.event == 'push'",f)
        self.assertIn("github.event.workflow_run.head_branch == 'main'",f)
        self.assertIn('head_repository.full_name == github.repository',f)
        self.assertIn('test "$SOURCE_SHA" = "$(gh api repos/$REPO/commits/main --jq .sha)"',f)
        self.assertIn('--target "$SOURCE_SHA"',f)
        self.assertIn('gh release create v1.1.0',f)
        self.assertIn('--latest',f)
        self.assertIn('--notes-file docs/RELEASE_V1.1.0.md',f)
        self.assertIn('SquachEmit-v1.1.0-SHA256SUMS.txt',f)
        for tag in TAGS:
            self.assertIn(tag,f)
        for artifact in ('squachemit-cyd32-st7798-firmware-unverified',
                         'squachemit-waveshare-c5-28-firmware-unverified',
                         'squachemit-esp32s3-firmware-unverified'):
            self.assertIn(artifact,f)

    def test_release_notes_and_readme_are_current(self):
        notes=(ROOT/'docs/RELEASE_V1.1.0.md').read_text()
        readme=(ROOT/'README.md').read_text()
        self.assertIn('v1.1.0',notes)
        self.assertIn('172',notes)
        self.assertIn('What Changed',notes)
        self.assertIn('SHA256SUMS',notes)
        self.assertIn('3–30',notes)
        self.assertIn('v1.1.0 Public Release',readme)
        self.assertIn('**VERSION** and **WHAT CHANGED**',readme)

if __name__=='__main__':
    unittest.main()
