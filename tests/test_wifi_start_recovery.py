"""Guard Wi-Fi drone start reliability and visible diagnostic stages."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class WifiStartRecoveryTest(unittest.TestCase):
 def test_stages_retry_and_truth(self):
  src=(ROOT/"src/main.cpp").read_text()
  for stage in ("STA_MODE","AP_MAC_SET","AP_MODE","AP_MAC_READBACK","CHANNEL",
                "SOFTAP_START","PACK_ENCODE","VENDOR_IE"):
   self.assertIn('wifiFailure("'+stage+'"',src)
  for marker in ("ERROR,WIFI_STAGE","ERROR,WIFI_CASE","RADIO,WIFI_ATTEMPT",
                 "WARN,WIFI_RETRY","attempt<=2 && !ok",
                 'truth("ERROR", s, "radio_start_failed");','truth("BEGIN", s);'):
   self.assertIn(marker,src)
 def test_visible_failed_start(self):
  ui=(ROOT/"src/cyd32_ui.cpp").read_text()
  hdr=(ROOT/"include/cyd32_ui.h").read_text()
  self.assertIn('"WIFI START FAILED"',ui)
  self.assertIn('failed?radioStartError:',ui)
  self.assertIn('void cyd32UiSetRadioError(const char* stage)',hdr)
  self.assertIn('void rightStatus(bool emitting,uint32_t count)',ui)
if __name__=="__main__":unittest.main()
