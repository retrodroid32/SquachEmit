"""Publishing regression: workbench serves exact current firmware catalog."""
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("squachemit_site",ROOT/"tools"/"build_installer_site.py")
site=importlib.util.module_from_spec(spec)
spec.loader.exec_module(site)


class WorkbenchPublishTests(unittest.TestCase):
    def test_workbench_copy_and_catalog_equality(self):
        with tempfile.TemporaryDirectory() as tmp:
            output=Path(tmp)
            site.publish_workbench(output)
            workbench=output/"workbench"
            for name in ("index.html","style.css","core.js","app.js","cases.json","fork_expectations.json"):
                self.assertTrue((workbench/name).is_file(),name)
                self.assertGreater((workbench/name).stat().st_size,100,name)
            expected=json.loads((ROOT/"tests"/"cases.json").read_text())
            actual=json.loads((workbench/"cases.json").read_text())
            self.assertEqual(len(actual),369)
            self.assertEqual(actual,expected)
            fork=json.loads((workbench/"fork_expectations.json").read_text())
            self.assertEqual(len(fork["overrides"]),19)
            self.assertEqual(fork["baseline_expected_source"],"tests/cases.json")
            html=(workbench/"index.html").read_text()
            app=(workbench/"app.js").read_text()
            self.assertIn('FREENOVE-S3-3.5',html)
            self.assertIn('id="receiverLibrary"',html)
            self.assertIn('All 369 cases',html)
            self.assertIn('fork_expectations.json',app)
            self.assertIn("cases.length===369",app)
            self.assertIn('id="emitConnect"',(workbench/"index.html").read_text())
            self.assertIn('id="recvConnect"',(workbench/"index.html").read_text())
            self.assertIn('No automatic transmit',(workbench/"index.html").read_text())
            self.assertIn('core.parseTruth(line)',(workbench/"app.js").read_text())
            self.assertIn('uniqueRunId()', (workbench/"app.js").read_text())
            self.assertIn("trialKey", (workbench/"app.js").read_text())

    def test_installer_link_and_ci_run_workbench(self):
        landing=(ROOT/"web-installer"/"index.html").read_text()
        workflow=(ROOT/".github/workflows/publish-installer.yml").read_text()
        ci=(ROOT/".github/workflows/ci.yml").read_text()
        self.assertIn('href="./workbench/"',landing)
        self.assertIn("'workbench/**'",workflow)
        self.assertIn("'tests/cases.json'",workflow)
        self.assertIn("node tests/verify_workbench.cjs",ci)


if __name__=="__main__":unittest.main()
