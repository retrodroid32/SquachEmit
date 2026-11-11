"""DJI model-level Remote ID cases: synthetic BLE4 and Wi-Fi Basic ID only."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODELS = ["AIR2S","AIR3","AVATA","FPV","MATRICE300RTK","MATRICE30","MATRICE350RTK","MAVIC2EA","MAVIC2PRO","MAVIC2ZOOM","MAVIC3","MAVIC3CLASSIC","MAVIC3E","MAVIC3PRO","MAVIC3T","MINI3","MINI3PRO","MINI4PRO","P4PROV2","P4RTK"]

class DjiModelMatrixTests(unittest.TestCase):
    def test_forty_synthetic_dji_cases(self):
        scenarios = json.loads((ROOT/"tests/cases.json").read_text())
        self.assertEqual(len(scenarios), 145)
        by_id = {s["id"]:s for s in scenarios}
        self.assertEqual(len(by_id), len(scenarios))
        observed_serials = set()
        for model in MODELS:
            b = by_id["RID-DJI-"+model]
            w = by_id["RID-WIFI-DJI-"+model]
            self.assertEqual((b["kind"],b["carrier"],b["expected"]), ("BleRemoteId","BLE","DRONE"))
            self.assertEqual((w["kind"],w["carrier"],w["expected"]), ("WifiRemoteId","WIFI","DRONE"))
            self.assertEqual(w["code"], 6)
            self.assertEqual(b["code"], 0)
            self.assertEqual(b["description"].split(" | ")[0],w["description"].split(" | ")[0])
            self.assertIn("SYNTHETIC",b["description"])
            self.assertIn("SYNTHETIC",w["description"])
            self.assertIn("not actual DJI RF",b["description"])
            self.assertIn("not actual DJI RF",w["description"])
            for serial in (b["text"],w["text"]):
                self.assertTrue(serial.startswith("SQE-DJI-"))
                self.assertLessEqual(len(serial), 20)
                self.assertNotIn(serial, observed_serials)
                observed_serials.add(serial)
        self.assertEqual(len(observed_serials), 40)

    def test_onair_code_preserves_synthetic_wifi_serial(self):
        code = (ROOT/"src/main.cpp").read_text()
        self.assertIn('strncmp(s->id,"RID-WIFI-DJI-",13)==0',code)
        self.assertIn("s->kind == Kind::BleRemoteId || wifiModel",code)
        self.assertIn("esp_wifi_set_vendor_ie(",code)
        # No raw DJI proprietary DroneID or custom 802.11 action frame transmitter.
        self.assertNotIn("OcuSync", code)
        ui = (ROOT/"src/cyd32_ui.cpp").read_text()
        self.assertIn('startsWith(s->id,"RID-WIFI-DJI-")',ui)
        self.assertIn('SYNTHETIC WIFI BEACON / BASIC ID',ui)

    def test_all_dji_cases_in_drones_library(self):
        header = (ROOT/"include/scenario_library.h").read_text()
        self.assertIn('startsWith(id,"RID-")',header)
        scenarios = json.loads((ROOT/"tests/cases.json").read_text())
        self.assertTrue(all(s["id"].startswith("RID-") for s in scenarios
                            if s["id"].startswith(("RID-DJI-","RID-WIFI-DJI-"))))

if __name__ == "__main__": unittest.main()
