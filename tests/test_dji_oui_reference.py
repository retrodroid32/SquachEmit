"""Verify DJI OUIs as weak passive vendor context; never emulate vendor MACs."""
from pathlib import Path
import importlib.util
import json
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("dji_oui_reference",ROOT/"tools/dji_oui_reference.py")
dji=importlib.util.module_from_spec(spec)
spec.loader.exec_module(dji)
ref=json.loads((ROOT/"tools/dji_oui_reference.json").read_text())

class DjiOuiTests(unittest.TestCase):
    def test_exact_verified_allocations(self):
        expected={"04:A8:5A","0C:9A:E6","34:D2:62","48:1C:B9",
                  "4C:43:F6","58:B8:58","60:60:1F","88:29:85",
                  "8C:58:23","E4:7A:2C"}
        self.assertEqual({x["prefix"] for x in ref["prefixes"]},expected)
        self.assertEqual(len(ref["prefixes"]),10)
        self.assertEqual(ref["udger_vendor_id"],10425)
        for item in ref["prefixes"]:
            self.assertEqual(int(item["prefix"][:2],16)&3,0)
            self.assertRegex(item["registered"],r"^\d{4}-\d{2}-\d{2}$")
            result=dji.classify(item["prefix"]+":11:22:33",ref)
            self.assertEqual(result["classification"],"DJI_REGISTERED_OUI")
            self.assertEqual(result["registration_date"],item["registered"])
            self.assertIsNone(result["is_drone"])
            self.assertIsNone(result["is_remote_id"])
            self.assertIsNone(result["model"])

    def test_supported_mac_formats(self):
        for form in ("34:d2:62:11:22:33","34-D2-62-11-22-33",
                     "34D262112233","34D2.6211.2233"):
            self.assertEqual(dji.classify(form,ref)["classification"],"DJI_REGISTERED_OUI")
        for invalid in ("","34:D2:62","34:D2:62:11:22:GG",
                        "34D2.6211.223344","34:D2:62:11:22:33:44"):
            with self.assertRaises(ValueError):
                dji.normalize_mac(invalid)

    def test_synthetic_air2s_mac_is_not_a_dji_oui(self):
        result=dji.classify("02:B0:CB:D8:04:26",ref)
        self.assertTrue(result["locally_administered"])
        self.assertEqual(result["classification"],"UNKNOWN_VENDOR")
        main=(ROOT/"src/main.cpp").read_text()
        self.assertIn("SquachIdentity::derive(",main)
        self.assertNotIn("dji_oui_reference",main)

    def test_unknown_is_not_proof_of_no_drone(self):
        for mac in ("00:11:22:33:44:55","02:11:22:33:44:55","FF:FF:FF:FF:FF:FF"):
            result=dji.classify(mac,ref)
            self.assertEqual(result["classification"],"UNKNOWN_VENDOR")
            self.assertIsNone(result["is_drone"])
            self.assertIsNone(result["is_remote_id"])
            self.assertIsNone(result["model"])

if __name__=="__main__":unittest.main()
