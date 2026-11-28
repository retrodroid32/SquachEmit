"""Prevent a historical SquachEmit ST7798 speech bubble buffer overflow."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class BubbleSafetyTests(unittest.TestCase):
    def test_second_line_bounded_by_real_detail_buffer(self):
        src = (ROOT / "src" / "cyd32_ui.cpp").read_text()
        self.assertRegex(src, r"char primary\[64\],detail\[32\]")
        self.assertRegex(src, r"speechWrap\(const char\* phrase,char first\[64\],char second\[32\]")
        self.assertIn('snprintf(second,32,"%s",whole+best+1)', src)
        self.assertIn("strlen(whole+i+1)>=32", src)
        self.assertNotIn('snprintf(second,64,"%s",whole+best+1)', src)


if __name__ == "__main__":
    unittest.main()
