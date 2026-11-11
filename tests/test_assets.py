import csv
import json
import struct
import unittest
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from rid_vectors import advertisements, messages

class TestFixtures(unittest.TestCase):
    def setUp(self):
        self.cases = json.loads((ROOT/'tests'/'cases.json').read_text())
        self.offline = json.loads((ROOT/'tests'/'offline_vectors.json').read_text())

    def test_case_catalog_consistency(self):
        source = (ROOT/'src'/'scenarios.cpp').read_text()
        for s in self.cases:
            self.assertIn(f'"{s["id"]}"', source)
            self.assertIn(f'"{s["expected"]}"', source)
        self.assertEqual(len({s['id'] for s in self.cases}), len(self.cases))
        self.assertEqual(len(self.cases), 172)
        self.assertEqual(len(self.cases), len(__import__('re').findall(r'^\s*\{"', source, __import__('re').M)))
        self.assertEqual(next(s for s in self.cases if s['id']=='BLE-GOOGLE')['expected'], 'NONE')
        self.assertLess(len(self.cases), 254)  # synthetic per-case MAC identity limit
        self.assertTrue(all(s['text'] is None or len(s['text']) <= 26 for s in self.cases if s['kind']=='BleName'))

    def test_observation_template(self):
        with (ROOT/'tests'/'observations.csv').open(newline='') as f:
            observed = list(csv.DictReader(f))
        self.assertEqual({s['id'] for s in observed}, {s['id'] for s in self.cases})
        self.assertTrue(all(not s['observed'] for s in observed))

    def test_remote_id_31_bytes_and_fields(self):
        adv = advertisements()
        self.assertEqual(len(adv), 3)
        self.assertTrue(all(len(a) == 31 for a in adv))
        self.assertEqual(adv[0][:5], bytes([30, 0x16, 0xFA, 0xFF, 0x0D]))
        self.assertEqual(adv[0][8:28], b'SQUACHEMIT-TEST-0001')
        self.assertAlmostEqual(struct.unpack('<i', adv[1][11:15])[0]/1e7, 40.7128)
        self.assertAlmostEqual(struct.unpack('<i', adv[1][15:19])[0]/1e7, -74.006)
        self.assertEqual(struct.unpack('<H', adv[1][21:23])[0], 2240)
        self.assertAlmostEqual(struct.unpack('<i', adv[2][8:12])[0]/1e7, 40.758)

    def test_offline_frames_are_valid_offline_only(self):
        self.assertGreaterEqual(len(self.offline), 10)
        for frame in self.offline:
            packet = bytes.fromhex(frame['frame_hex'])
            self.assertGreaterEqual(len(packet), 26)
            self.assertFalse(frame['rf_transmitted'])
            self.assertTrue(packet[0] in (0x80, 0x40, 0xC0, 0xD0))
        # The exact MA-M nibble must differ from its negative neighbor
        v = {x['id']: bytes.fromhex(x['frame_hex']) for x in self.offline}
        self.assertEqual(v['OUI-AUTEL-28'][16:19], v['OUI-AUTEL-NEIGHBOR'][16:19])
        self.assertNotEqual(v['OUI-AUTEL-28'][19]>>4, v['OUI-AUTEL-NEIGHBOR'][19]>>4)

if __name__ == '__main__': unittest.main()
