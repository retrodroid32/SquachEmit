"""Prevent stale on-device boot case counts when the RF catalog expands."""
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class BootCatalogCountTest(unittest.TestCase):
    def test_catalog_query_duration_validation_and_post_stop_truth(self):
        src = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        self.assertIn('cmd.equalsIgnoreCase("CATALOG")', src)
        self.assertIn('catalogHash()', src)
        self.assertIn('for(size_t i=0;i<seconds.length();++i)', src)
        self.assertIn("seconds[i]<'0'||seconds[i]>'9'", src)
        stop_section = src.split('static void finish(const char* status = "ok")', 1)[1].split('static void cancelAll()', 1)[0]
        self.assertLess(stop_section.index('stopTx();'), stop_section.index('truth("END"'),
                        "TRUTH END must not precede radio shutdown")

    def test_all_board_boot_banners_use_catalog_count(self):
        source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        catalog = json.loads((ROOT / "tests/cases.json").read_text(encoding="utf-8"))
        profile = json.loads((ROOT / "tools/active_firmware_profile.json").read_text(encoding="utf-8"))
        self.assertEqual(len(catalog), profile["scenario_count"])
        for board in ("ESP32-C5-Waveshare-2.8", "ESP32-2432S032R-ST7798", "ESP32-S3"):
            with self.subTest(board=board):
                self.assertRegex(source, re.escape(board) + r',%u-CASES')
        self.assertGreaterEqual(source.count('(unsigned)kScenarioCount);'), 4)
        self.assertNotRegex(source, r'BOOT,[^\\n]*,172-CASES')


if __name__ == "__main__":
    unittest.main()
