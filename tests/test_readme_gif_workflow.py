"""Ensure the README GIF regeneration workflow does not race or regress firmware."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/readme-animated-demo.yml"

class ReadmeGifWorkflowTests(unittest.TestCase):
    def test_source_changed_in_docs_branch_starts_capture(self):
        yml=WORKFLOW.read_text()
        for required in (
            "src/cyd32_ui.cpp",
            "src/cyd32_squachy_port.cpp",
            "src/scenarios.cpp",
            "tools/readme_capture/**",
            "docs/squachemit-demo.gif",
            "docs/squachemit-device-library.gif",
            "docs/squachemit-test-running.gif"):
            self.assertIn(required,yml)
        self.assertIn("python tools/readme_capture/generate.py --verify",yml)

    def test_safe_concurrent_update_not_forced(self):
        yml=WORKFLOW.read_text()
        self.assertIn("concurrency:",yml)
        self.assertIn("cancel-in-progress: true",yml)
        self.assertIn('git fetch origin "$GITHUB_REF_NAME"',yml)
        self.assertIn('git rebase "origin/$GITHUB_REF_NAME"',yml)
        self.assertIn("for attempt in 1 2 3; do",yml)
        self.assertIn('git push origin "HEAD:$GITHUB_REF_NAME"',yml)
        self.assertNotIn("git push --force",yml)
        self.assertNotIn("git reset --hard",yml)

if __name__=="__main__":
    unittest.main()
