import importlib.util
import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("site_builder", ROOT / "tools" / "build_installer_site.py")
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class InstallerManifestTests(unittest.TestCase):
    def test_c5_catalog_checksums_match_written_firmware(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            input_dir = root / "source"
            input_dir.mkdir()
            boot = b"\xe9" + b"\0" * 63
            app = b"\xe9" + b"\0" * 4095
            ota = b"\0" * 8192
            partition = (
                struct.pack("<HBBII16sI", 0x50aa, 0, 0, 0x10000, 0x200000,
                            b"factory", 0) +
                struct.pack("<HBBII16sI", 0x50aa, 1, 0, 0xe000, 0x2000,
                            b"otadata", 0)
            ).ljust(3072, b"\xff")
            merged = bytearray(b"\xff" * (0x10000 + len(app)))
            for offset, data in ((0x2000, boot), (0x8000, partition),
                                 (0xe000, ota), (0x10000, app)):
                merged[offset:offset+len(data)] = data
            for name, data in (
                ("SquachEmit.ino.bootloader.bin", boot),
                ("SquachEmit.ino.partitions.bin", partition),
                ("SquachEmit.ino.bin", app),
                ("SquachEmit.ino.merged.bin", merged),
            ):
                (input_dir / name).write_bytes(data)
            src = {"board": "c5-waveshare-28", "sha": "a" * 40,
                   "branch": "lab/safe-check", "channel": "lab",
                   "created_at": "2026-11-09T00:00:00Z", "run_id": 42,
                   "path": str(input_dir)}
            result = builder.load_source(src, root / "site")
            self.assertEqual(result["chipFamily"], "ESP32-C5")
            self.assertEqual(len(result["segments"]), 4)
            for part in result["segments"]:
                value = (root / "site" / part["path"]).read_bytes()
                self.assertEqual(part["bytes"], len(value))
                self.assertEqual(part["sha256"], hashlib.sha256(value).hexdigest())
            manifest = json.loads((root / "site" / result["manifest"]).read_text())
            self.assertEqual(manifest["builds"][0]["chipFamily"], "ESP32-C5")
            for part in manifest["builds"][0]["parts"]:
                segment = next(x for x in result["segments"] if x["name"] == part["path"])
                self.assertEqual(part["offset"], segment["offset"])

    def test_rejects_bad_or_untrusted_input(self):
        with self.assertRaises(ValueError):
            builder.verify_partitions(b"\0" * 3072)
        with self.assertRaises(ValueError):
            builder.load_source({"board": "other"}, Path("/tmp/invalid-site"))


if __name__ == "__main__":
    unittest.main()
