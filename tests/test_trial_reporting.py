import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from score_trials import classify, score_rows, summarize, render_md


class TrialReportingTests(unittest.TestCase):
    def setUp(self):
        self.cases = {"P": {"expected": "DRONE"}, "N": {"expected": "NONE"}}

    def row(self, sid, observed, receiver="CYD-2.8", run_id="a",
            emission="ok", window="complete", latency_ms=""):
        return {"run_id": run_id, "receiver": receiver,
                "receiver_sha": "watch-1.22", "emitter_sha": "lab-1234",
                "id": sid, "emission": emission, "window": window,
                "observed": observed, "latency_ms": latency_ms}

    def test_no_imaginary_successes(self):
        self.assertEqual(classify(self.row("P", ""), "DRONE"), "UNTESTED")
        self.assertEqual(classify(self.row("P", "DRONE", emission="failed"), "DRONE"), "EXCLUDED")
        self.assertEqual(classify(self.row("N", "NONE", window="incomplete"), "NONE"), "EXCLUDED")
        self.assertEqual(classify(self.row("P", "NONE"), "DRONE"), "FN")

    def test_separate_receivers_and_wrong_class(self):
        rows = [
            self.row("P", "DRONE", latency_ms="1200"),
            self.row("P", "NONE", receiver="CYD-3.2"),
            self.row("N", "AXON"),
            self.row("P", "AXON", run_id="b"),
        ]
        scored = score_rows(rows, self.cases)
        a = summarize([x for x in scored if x["receiver"] == "CYD-2.8"])
        b = summarize([x for x in scored if x["receiver"] == "CYD-3.2"])
        self.assertEqual((a["tp"], a["fp"], a["fn"], a["wrong_class"]), (1, 2, 1, 1))
        self.assertEqual((b["tp"], b["fn"]), (0, 1))
        self.assertIn("CYD-3.2", render_md(scored))

    def test_specificity_uses_negative_controls_only(self):
        rows = [
            self.row("N", "NONE", run_id="clean"),
            self.row("P", "AXON", run_id="wrong-positive"),
        ]
        result = summarize(score_rows(rows, self.cases))
        self.assertEqual(result["tn"], 1)
        self.assertEqual(result["wrong_class"], 1)
        self.assertEqual(result["specificity_pct"], 100.0)

    def test_invalid_input_is_rejected(self):
        with self.assertRaises(ValueError):
            score_rows([self.row("P", "DRONE"), self.row("P", "DRONE")], self.cases)
        with self.assertRaises(ValueError):
            score_rows([self.row("P", "DRONE", latency_ms="-2")], self.cases)
        with self.assertRaises(ValueError):
            score_rows([self.row("P", "DRONE", latency_ms="nan")], self.cases)
        with self.assertRaises(ValueError):
            score_rows([self.row("P", "DRONE", receiver="CYD-9.0")], self.cases)
        self.assertEqual(classify(self.row("P", "DRONE", window=""), "DRONE"), "UNTESTED")

    def test_empty_dataset_not_pass(self):
        empty = summarize([])
        self.assertEqual(empty["counted"], 0)
        self.assertIsNone(empty["precision_pct"])
        self.assertIsNone(empty["recall_pct"])


if __name__ == "__main__":
    unittest.main()
