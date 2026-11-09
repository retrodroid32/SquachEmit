import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from generate_drone_vectors import build_vectors, ble4, ble5, basic, location, beacon, nan, elapsed_since_hour_seconds
from protocol_reference import ble_remote_adv, beacon_remote_id, wifi_nan_remote_id


class DroneProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.v = {x["id"]: x for x in build_vectors()}

    def frame(self, key):
        return bytes.fromhex(self.v[key]["frame_hex"])

    def test_fixtures_remain_offline_and_transport_scoped(self):
        self.assertGreaterEqual(len(self.v), 13)
        self.assertTrue(all(not x["rf_transmitted"] for x in self.v.values()))
        self.assertEqual(self.v["RID-BLE5-EXT-PACK"]["phy"], "LE_CODED_TEST_ONLY")
        self.assertEqual(len(self.frame("RID-BLE5-EXT-PACK")), 109)

    def test_bluetooth_legacy_positive_and_negative(self):
        self.assertIsNotNone(ble_remote_adv(self.frame("RID-BLE4-BASIC")))
        self.assertIsNone(ble_remote_adv(self.frame("RID-BLE-NEG-APP")))
        self.assertIsNone(ble_remote_adv(self.frame("RID-BLE-NEG-SHORT")))

    def test_wifi_beacon_and_parrot_vendor_ie(self):
        for key in ("RID-WIFI-BEACON-LAB", "RID-WIFI-PARROT-LAB"):
            self.assertIsNotNone(beacon_remote_id(self.frame(key)))
        self.assertIsNone(beacon_remote_id(self.frame("RID-WIFI-NEG-PACK")))

    def test_nan_hash_positive_negative(self):
        self.assertTrue(wifi_nan_remote_id(self.frame("RID-WIFI-NAN-LAB")))
        self.assertFalse(wifi_nan_remote_id(self.frame("RID-WIFI-NAN-NEG")))

    def test_stale_location_and_missing_operator_not_falsely_none(self):
        self.assertEqual(self.v["RID-NO-OPERATOR"]["expected"], "DRONE")
        self.assertTrue(self.v["RID-NO-OPERATOR"]["missing_operator"])
        self.assertEqual(self.v["RID-STALE-LOCATION"]["expected"], "DRONE")
        self.assertGreater(self.v["RID-STALE-LOCATION"]["age_seconds"], 120)
        self.assertIsNone(elapsed_since_hour_seconds(120, 0xFFFF))

    def test_catalog_profiles_and_model_descriptions(self):
        source = (ROOT / "src" / "scenarios.cpp").read_text()
        for suffix in ("DJI-MINI","DJI-AIR","DJI-MAVIC","DJI-PHANTOM",
                       "DJI-MATRICE","AUTEL-EVO2","AUTEL-EVOMAX",
                       "AUTEL-EVOLITE","PARROT-ANAFI","PARROT-ANAFIUSA"):
            self.assertIn('"RID-'+suffix+'"', source)
        for name in ("RID-WIFI-BEACON","RID-WIFI-PARROT","RID-NEG-APP",
                     "RID-NEG-SHORT","RID-NO-OPERATOR","RID-STALE-LOC"):
            self.assertIn('"'+name+'"', source)
        main = (ROOT / "src" / "main.cpp").read_text()
        self.assertIn("esp_wifi_set_vendor_ie", main)
        self.assertIn("WIFI_VND_IE_TYPE_BEACON", main)
        self.assertIn("RID-NO-OPERATOR", main)
        self.assertIn('strcmp(s->id, "BLE-IBEACON-V2") == 0', main)
        self.assertNotIn("esp_wifi_80211_tx(", main)


if __name__ == "__main__":
    unittest.main()
