"""Touchscreen RUN acknowledgement and Wi-Fi dispatcher regression."""
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class TouchWifiRunTests(unittest.TestCase):
    def test_cyd_wifi_run_before_driver_start(self):
        s=(ROOT/"src/main.cpp").read_text()
        a=s.index('case Cyd32UiAction::Run:')
        b=s.index('case Cyd32UiAction::Stop:',a)
        run=s[a:b]
        for item in ('UI,RUN_TAP,%s,%s','cyd32UiSetRadioStarting(chosen->id)',
                     'cyd32UiTick(nullptr,0,false)','UI,RUN_DISPATCH,%s',
                     'startScenario(chosen);','ERROR,UI_RUN_NO_SELECTION'):
            self.assertIn(item,run)
        self.assertLess(run.index("UI,RUN_TAP"),run.index("startScenario(chosen);"))
        self.assertLess(run.index("cyd32UiSetRadioStarting"),run.index("startScenario(chosen);"))
    def test_cyd_running_state_clears_on_result(self):
        ui=(ROOT/"src/cyd32_ui.cpp").read_text()
        header=(ROOT/"include/cyd32_ui.h").read_text()
        self.assertIn('"STARTING WIFI TRANSMISSION"',ui)
        self.assertIn('starting?radioStartingProfile:',ui)
        self.assertIn('void cyd32UiSetRadioStarting(const char* scenarioId)',header)
        self.assertIn('void cyd32UiSetRadioError(const char* stage)',header)
        self.assertIn("radioStartingProfile[0]='\\0'",ui)
        self.assertIn('snprintf(radioStartError,sizeof(radioStartError)',ui)
if __name__=="__main__":unittest.main()
