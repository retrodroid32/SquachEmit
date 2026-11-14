"""Regression for all ten DJI MA-L allocations in *offline-only* fixtures.

MAC vendor attribution is not evidence of flight, Remote ID, or a drone model.
No radio identity or scenario is changed by this reference fixture matrix.
"""
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "04:A8:5A", "0C:9A:E6", "34:D2:62", "48:1C:B9", "4C:43:F6",
    "58:B8:58", "60:60:1F", "88:29:85", "8C:58:23", "E4:7A:2C",
}

class DjiOuiRegistryTests(unittest.TestCase):
    def setUp(self):
        self.registry = json.loads((ROOT / "tests/dji_oui_registry.json").read_text())
        self.vectors = json.loads((ROOT / "tests/offline_vectors.json").read_text())

    def test_exact_vendor_registry_set(self):
        self.assertEqual(self.registry["registrant"], "SZ DJI TECHNOLOGY CO.,LTD")
        self.assertEqual(self.registry["registry_type"], "IEEE MA-L /24")
        records = self.registry["allocations"]
        self.assertEqual(len(records), 10)
        self.assertEqual({entry["prefix"] for entry in records}, EXPECTED)
        for entry in records:
            self.assertRegex(entry["registered"], r"^20[0-9]{2}-[0-9]{2}-[0-9]{2}$")
        self.assertTrue(self.registry["source"].startswith("https://maclookup.app/"))

    def test_all_ten_beacons_are_offline_and_keep_vendor_identity(self):
        by_id = {v["id"]: v for v in self.vectors}
        self.assertEqual(len(by_id), len(self.vectors))
        entries = [v for v in self.vectors if v["id"].startswith("OUI-DJI-")]
        self.assertEqual(len(entries), 10)
        for item in self.registry["allocations"]:
            mac3 = bytes.fromhex(item["prefix"].replace(":", ""))
            id_ = "OUI-DJI-" + mac3.hex().upper()
            self.assertIn(id_, by_id)
            fixture = by_id[id_]
            self.assertEqual((fixture["class"], fixture["expected"]), ("WIFI_OUI", "DRONE"))
            self.assertFalse(fixture["rf_transmitted"])
            self.assertIn("not aircraft/Remote ID", fixture["notes"])
            self.assertIn(item["registered"], fixture["notes"])
            frame = bytes.fromhex(fixture["frame_hex"])
            self.assertEqual(frame[0:2], bytes([0x80, 0x00]))  # beacon
            self.assertEqual(frame[10:13], mac3) # transmitter
            self.assertEqual(frame[16:19], mac3) # BSSID read by native checker
            self.assertEqual(len(frame), 45)

    def test_dji_on_air_prefixes_are_explicit_synthetic_wifi_tests(self):
        identity = (ROOT / "include/identity.h").read_text()
        main = (ROOT / "src/main.cpp").read_text()
        self.assertIn("? 0xC2 : 0x02", identity)
        self.assertIn("SquachDjiOuiLab::apply(s->id,out)", main)
        self.assertIn("SquachUavVendorLab::apply(s->id,out)", main)
        self.assertIn("TEST,OUI_ONLY", main)
        onair = json.loads((ROOT / "tests/cases.json").read_text())
        dji_cases = [s for s in onair if s["id"].startswith("OUI-DJI-")]
        self.assertEqual(len(dji_cases), 10)
        self.assertEqual({s["id"] for s in dji_cases},
                         {"OUI-DJI-" + p.replace(":", "") for p in EXPECTED})
        self.assertTrue(all(s["kind"] == "WifiAp" and
                            s["text"].startswith("SQE-LAB-DJI-") and
                            "NO RID" in s["description"] for s in dji_cases))

if __name__ == "__main__":
    unittest.main()
