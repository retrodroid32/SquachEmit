import sys
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from capture_pair import valid_case, make_line


class PairedCaptureTests(unittest.TestCase):
    def test_bounded_explicit_case(self):
        self.assertEqual(valid_case("WIFI-AXON-CH6"), "WIFI-AXON-CH6")
        self.assertEqual(valid_case("BLE-NEG-DRONE-UUID"), "BLE-NEG-DRONE-UUID")
        for bad in ("", "hi", "RUN WIFI-AXON", "WIFI;HALT", "a" * 50):
            with self.assertRaises(ValueError):
                valid_case(bad)

    def test_raw_observation_no_inferred_label(self):
        record = make_line("receiver", b"[ble] scan note\r\n", 12345, "2026-11-09T00:00:00+00:00")
        self.assertEqual(record["line"], "[ble] scan note")
        self.assertEqual(record["source"], "receiver")
        self.assertNotIn("observed", record)
        self.assertNotIn("expected", record)


if __name__ == "__main__":
    unittest.main()
