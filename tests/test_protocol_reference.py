import sys
import unittest
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from rid_vectors import advertisements
from protocol_reference import remote_message, ble_remote_adv, beacon_remote_id, wifi_nan_remote_id, parse_ibeacon_mfg


class ProtocolReferenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.frames = {x["id"]: bytes.fromhex(x["frame_hex"]) for x in
                      json.loads((ROOT / "tests" / "offline_vectors.json").read_text())}

    def test_ble_remote_id_valid(self):
        values = list(map(ble_remote_adv, advertisements()))
        self.assertEqual([v["kind"] for v in values], ["basic", "location", "system"])
        self.assertEqual(values[0]["uas_id"], "SQUACHEMIT-TEST-0001")
        self.assertEqual(values[1]["lat_e7"], 407128000)
        self.assertEqual(values[1]["lon_e7"], -740060000)
        self.assertEqual(values[1]["altitude_geo_m"], 120)
        self.assertNotEqual(values[1]["lat_e7"], values[2]["operator_lat_e7"])

    def test_ble_remote_id_rejects_invalid(self):
        valid = bytearray(advertisements()[0])
        for index, value in ((0, 29), (1, 0x09), (2, 0), (3, 0), (4, 0)):
            damaged = bytearray(valid)
            damaged[index] = value
            self.assertIsNone(ble_remote_adv(bytes(damaged)))
        self.assertIsNone(ble_remote_adv(bytes(valid[:-1])))
        self.assertIsNone(remote_message(b"\x02" * 24))

    def test_wifi_remote_id_beacon_nan_and_negative(self):
        for sid in ("RID-WIFI-BEACON", "RID-WIFI-PARROT"):
            v = beacon_remote_id(self.frames[sid])
            self.assertIsNotNone(v, sid)
            self.assertEqual(v["uas_id"], "SQUACHEMIT-TEST-0001")
        self.assertIsNone(beacon_remote_id(self.frames["RID-WIFI-BAD-PACK"]))
        self.assertIsNone(beacon_remote_id(self.frames["OUI-FLOCK-OWN"]))
        self.assertIsNone(beacon_remote_id(self.frames["RID-WIFI-BEACON"][:-1]))
        self.assertTrue(wifi_nan_remote_id(self.frames["RID-WIFI-NAN"]))
        self.assertFalse(wifi_nan_remote_id(self.frames["OUI-DRONE"]))

    def test_ibeacon_identity_and_truncation(self):
        raw = bytes.fromhex("4c000215") + bytes(range(16)) + bytes.fromhex("1234abcdc5")
        result = parse_ibeacon_mfg(raw)
        self.assertEqual(result["major"], 0x1234)
        self.assertEqual(result["minor"], 0xabcd)
        self.assertEqual(result["measured_power"], -59)
        self.assertIsNone(parse_ibeacon_mfg(bytes.fromhex("4c0002150102")))
        self.assertIsNone(parse_ibeacon_mfg(b"\x4d" + raw[1:]))


if __name__ == "__main__":
    unittest.main()
