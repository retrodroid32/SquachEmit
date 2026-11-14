"""Firmware integrity catalog tests for the *current* three-board installer."""
import hashlib
import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("integrity_builder",ROOT/"tools"/"build_installer_site.py")
builder=importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

def partition_table():
    return (struct.pack("<HBBII16sI",0x50aa,0,0x10,0x10000,0x300000,b"factory",0)
            +struct.pack("<HBBII16sI",0x50aa,1,0,0xe000,0x2000,b"otadata",0)).ljust(3072,b"\xff")

class InstallerManifestTests(unittest.TestCase):
    def make_artifact(self,root,board):
        work=root/"source";work.mkdir()
        boot=b"\xe9"+b"\x00"*4095
        app=b"\xe9"+b"\x00"*4095
        ota=b"\x00"*8192
        parts=partition_table()
        if board=="c5-waveshare-28":
            merged=bytearray(b"\xff"*(0x10000+len(app)))
            for offset,data in ((0x2000,boot),(0x8000,parts),(0xe000,ota),(0x10000,app)):
                merged[offset:offset+len(data)]=data
            files={"SquachEmit.ino.bootloader.bin":boot,"SquachEmit.ino.partitions.bin":parts,
                   "SquachEmit.ino.bin":app,"SquachEmit.ino.merged.bin":merged}
        else:
            files={"bootloader.bin":boot,"partitions.bin":parts,
                   "ota_init.bin":ota,"firmware.bin":app}
        for name,data in files.items(): (work/name).write_bytes(data)
        return {"board":board,"sha":"a"*40,"branch":"lab/safe-preflight",
                "channel":"lab","created_at":"2026-11-11T00:00:00Z",
                "run_id":42,"path":str(work)}

    def test_all_board_families_have_four_verified_segments(self):
        for board,chip,boot in (("cyd32-st7798","ESP32",0x1000),
                                ("c5-waveshare-28","ESP32-C5",0x2000),
                                ("s3-devkit","ESP32-S3",0x0)):
            with self.subTest(board=board),tempfile.TemporaryDirectory() as tmp:
                root=Path(tmp)
                build=builder.load_source(self.make_artifact(root,board),root/"site")
                self.assertEqual(build["chipFamily"],chip)
                self.assertEqual([x["name"] for x in build["segments"]],
                                 ["bootloader.bin","partitions.bin","otadata.bin","application.bin"])
                self.assertEqual([x["offset"] for x in build["segments"]],
                                 [boot,0x8000,0xe000,0x10000])
                manifest=json.loads((root/"site"/build["manifest"]).read_text())
                self.assertEqual(manifest["builds"][0]["chipFamily"],chip)
                self.assertEqual(manifest["builds"][0]["parts"],
                    [{"path":s["name"],"offset":s["offset"]} for s in build["segments"]])
                for segment in build["segments"]:
                    contents=(root/"site"/segment["path"]).read_bytes()
                    self.assertEqual(segment["bytes"],len(contents))
                    self.assertEqual(segment["sha256"],hashlib.sha256(contents).hexdigest())
                self.assertEqual(build["sha256"],build["segments"][3]["sha256"])
                self.assertEqual(build["bytes"],build["segments"][3]["bytes"])
                self.assertEqual(build["application"],build["segments"][3]["path"])


    def test_all_lab_branches_and_all_boards_without_sha_pins(self):
        for branch,sha in (('feature/new-device-workbench','b'*40),
                           ('lab/next-case-suite','c'*40)):
            profile={
                'schema':1, 'scenario_count':172,
                'source_commit_sha':sha,
                'builds':{
                    board:{
                        'title':f'SquachEmit {board} 172-case LAB',
                        'version':'v1.1.0 MAINLINE · 172 cases',
                        'summary':'Experimental LAB firmware; physical verification pending.',
                        'highlights':['172 cases; physical verification pending.']
                    } for board in builder.BOARDS
                }
            }
            for board in builder.BOARDS:
                with self.subTest(branch=branch,board=board),tempfile.TemporaryDirectory() as tmp:
                    root=Path(tmp)
                    src=self.make_artifact(root,board)
                    src.update(branch=branch,sha=sha,source_profile=profile)
                    build=builder.load_source(src,root/'site')
                    self.assertEqual(build['channel'],'lab')
                    self.assertEqual(build['sha'],sha)
                    self.assertEqual(build['version_label'],f'LAB 172 cases · {sha[:8]}')
                    self.assertTrue(build['notes_verified_for_sha'])

    def test_only_main_can_publish_stable_and_unrelated_branches_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            src=self.make_artifact(root,'cyd32-st7798')
            src.update(branch='feature/new-suite',channel='stable')
            with self.assertRaisesRegex(ValueError,'development-branch firmware must publish to LAB'):
                builder.load_source(src,root/'no_stable')
            src.update(branch='maintenance/release-notes',channel='lab')
            with self.assertRaisesRegex(ValueError,'invalid trusted source ref'):
                builder.load_source(src,root/'no_maintenance')
            src.update(branch='main',channel='stable')
            build=builder.load_source(src,root/'stable')
            self.assertEqual(build['channel'],'stable')

    def test_publisher_always_deploys_from_main_and_supports_generic_lab(self):
        workflow=(ROOT/'.github/workflows/publish-installer.yml').read_text()
        self.assertIn('branches: [main]',workflow)
        self.assertIn('ref: main',workflow)
        self.assertIn("branch.startswith(('feature/','lab/'))",workflow)
        self.assertIn("channel='stable'",workflow)
        self.assertIn("channel='lab'",workflow)
        self.assertIn('profile=mainline_profile(sha)',workflow)
        self.assertNotIn('pinned_workbench_lab',workflow)
        self.assertNotIn('maintenance/lab-172r4-pages-publish-20261113',workflow)
        self.assertNotIn('0b7a2edcd8f7de6fc45ad320a7c0b9f6fa2e8974',workflow)

    def test_c5_merge_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            source=self.make_artifact(root,"c5-waveshare-28")
            merged=root/"source"/"SquachEmit.ino.merged.bin"
            data=bytearray(merged.read_bytes())
            data[0x2000]=0x00
            merged.write_bytes(data)
            with self.assertRaisesRegex(ValueError,"bootloader offset mismatch"):
                builder.load_source(source,root/"site")

    def test_bad_board_and_firmware_table_rejected(self):
        with self.assertRaises(ValueError):
            builder.load_source({"board":"cyd32-st7789-s3"},Path("/tmp/invalid-site"))
        with self.assertRaises(ValueError):
            builder.verify_partitions(b"\x00"*3072)

    def test_browser_fail_closed_includes_current_board_and_channel_contract(self):
        app=(ROOT/"web-installer"/"app.js").read_text()
        for snippet in ("integrityVerified && ui.confirm.checked",
                        "selected?.id !== entry.id",
                        "token !== verificationRequest",
                        "crypto.subtle.digest('SHA-256', bytes)",
                        "ui.confirm.disabled = true",
                        "ui.downloadBin.hidden = !selected || !integrityVerified",
                        "'cyd32-st7798': {prefix:'cyd32', chip:'ESP32', boot:0x1000}",
                        "'c5-waveshare-28': {prefix:'c5', chip:'ESP32-C5', boot:0x2000}",
                        "'s3-devkit': {prefix:'s3', chip:'ESP32-S3', boot:0x0000}"):
            self.assertIn(snippet,app)
        self.assertIn("node --check web-installer/app.js",
                      (ROOT/".github/workflows/ci.yml").read_text())

if __name__=="__main__": unittest.main()
