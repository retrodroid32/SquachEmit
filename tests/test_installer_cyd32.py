import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("squachemit_site", ROOT / "tools" / "build_installer_site.py")
site = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(site)


class Cyd32InstallerLayoutTests(unittest.TestCase):
    def test_cyd32_classic_esp32_flash_offsets_and_binary_parts(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            input_dir = root / "artifact"
            input_dir.mkdir()
            boot = b"\xe9" + b"\x00" * 4095
            app = b"\xe9" + b"\x00" * 4095
            ota = b"\x00" * 8192
            part = b"".join([
                struct.pack("<HBBII16sI", 0x50aa, 0, 0x10, 0x10000, 0x300000,
                            b"factory", 0),
                struct.pack("<HBBII16sI", 0x50aa, 1, 0, 0xe000, 0x2000,
                            b"otadata", 0),
            ]).ljust(3072, b"\xff")
            for name, data in (("bootloader.bin", boot),
                               ("partitions.bin", part),
                               ("ota_init.bin", ota),
                               ("firmware.bin", app)):
                (input_dir / name).write_bytes(data)
            src = {
                "board": "cyd32-st7798",
                "sha": "471699cb08ad7e458b4d56a11e1f5ef265d54ed5",
                "branch": "lab/cyd32-st7798-emitter-20261109",
                "channel": "lab", "run_id": 42,
                "created_at": "2026-11-09T19:58:00Z",
                "path": str(input_dir)
            }
            output = root / "site"
            build = site.load_source(src, output)
            self.assertEqual(build["board"], "cyd32-st7798")
            self.assertEqual(build["chipFamily"], "ESP32")
            self.assertEqual(build["title"], "CYD 3.2 ST7798 Smooth UI (49 tests)")
            self.assertEqual(build["id"], "lab-cyd32-471699cb")
            self.assertTrue((output / build["application"]).exists())
            manifest = json.loads((output / build["manifest"]).read_text())
            parts = manifest["builds"][0]["parts"]
            self.assertEqual(manifest["builds"][0]["chipFamily"], "ESP32")
            self.assertEqual([part["offset"] for part in parts],
                             [0x1000, 0x8000, 0xe000, 0x10000])
            self.assertEqual([part["path"] for part in parts],
                             ["bootloader.bin","partitions.bin","otadata.bin",
                              "application.bin"])
            self.assertEqual((output / "lab/cyd32-471699cb/bootloader.bin").read_bytes(), boot)

    def test_bad_board_rejected_without_giving_s3_firmware(self):
        with self.assertRaises(ValueError):
            site.load_source({"board":"cyd32-st7789-s3"}, Path("/tmp/invalid"))
        self.assertEqual(site.BOARDS["cyd32-st7798"]["chip"], "ESP32")


if __name__ == "__main__":
    unittest.main()
