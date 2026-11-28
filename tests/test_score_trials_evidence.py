"""Regression parity for receiver evidence scoring; no hardware observation is synthesized."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import score_trials as scorer


class TrialEvidenceScoringTests(unittest.TestCase):
    def setUp(self):
        self.cases = {
            "BLE-NEG-DRONE-UUID": {"expected": "NONE"},
            "BLE-FLOCK-MFG": {"expected": "FLOCK"},
        }
        self.base = {
            "run_id": "manual-001", "receiver": "FREENOVE-S3-3.5",
            "receiver_sha": "a" * 40, "emitter_sha": "b" * 40,
            "id": "BLE-NEG-DRONE-UUID", "emission": "ok",
            "window": "complete", "observed": "NONE", "latency_ms": "",
            "notes": "A LOG snapshot contained zero matching rows.",
        }

    def test_blank_log_cannot_claim_true_negative(self):
        rows = scorer.score_rows([self.base], self.cases)
        self.assertEqual(rows[0]["result"], "UNTESTED")

    def test_attested_negative_can_be_scored(self):
        row = {**self.base, "notes":
               "SCAN-VERIFIED: uninterrupted receiver scanning independently verified"}
        self.assertEqual(scorer.score_rows([row], self.cases)[0]["result"], "TN")

    def test_missing_negative_evidence_cannot_claim_false_negative(self):
        row = {**self.base, "id": "BLE-FLOCK-MFG"}
        self.assertEqual(scorer.score_rows([row], self.cases)[0]["result"], "UNTESTED")

    def test_valid_freenove_positive_is_supported(self):
        row = {**self.base, "id": "BLE-FLOCK-MFG", "observed": "FLOCK"}
        self.assertEqual(scorer.score_rows([row], self.cases)[0]["result"], "TP")

    def test_fork_overrides_do_not_modify_upstream_expected(self):
        upstream = scorer.receiver_cases("upstream")
        fork = scorer.receiver_cases("fork")
        self.assertEqual(len(upstream), 369)
        self.assertEqual(len(fork), 369)
        self.assertEqual(upstream["FORK-BLE-SKIM-BT04"]["expected"], "NONE")
        self.assertEqual(fork["FORK-BLE-SKIM-BT04"]["expected"], "SKIMMER")
        self.assertEqual(upstream["BLE-FLOCK-MFG"]["expected"],
                         fork["BLE-FLOCK-MFG"]["expected"])
        with self.assertRaises(ValueError):
            scorer.receiver_cases("unknown")

    def test_invalid_shas_cannot_count_positive(self):
        row = {**self.base, "id": "BLE-FLOCK-MFG", "observed": "FLOCK"}
        for invalid in ("unknown", "x" * 40, "a" * 41, ""):
            with self.subTest(invalid=invalid):
                with self.assertRaisesRegex(ValueError, "valid receiver_sha"):
                    scorer.score_rows([{**row, "receiver_sha": invalid}], self.cases)


if __name__ == "__main__":
    unittest.main()
