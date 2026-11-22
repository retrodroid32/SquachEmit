"""R14.29: host-compile real CYD3.2 battery code and verify boot consensus."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class BatteryFilterTests(unittest.TestCase):
    def test_filtered_voltage_and_boot_consensus(self):
        cc = shutil.which("g++") or shutil.which("clang++")
        if cc is None:
            self.skipTest("C++ compiler unavailable")
        code = r"""
#include "sqe_battery_filter.h"
#include <assert.h>
static void set(uint16_t (&v)[SqeBattery::kSamples],uint16_t mv){
  for(auto &x:v)x=mv;
}
int main(){
  static_assert(SqeBattery::kSamples==32, "ADC samples");
  static_assert(SqeBattery::kConsensusWindows==4, "cross-window consensus");
  uint16_t adc[SqeBattery::kSamples]={0};
  set(adc,2050);
  SqeBattery::Meter m;
  for(int i=0;i<3;++i) {
    assert(m.accept(adc));
    assert(m.displayedPercent==-1); // no first-sample confidence
  }
  assert(m.accept(adc));
  assert(m.rawMillivolts==4100);
  assert(m.filteredMillivolts==4100);
  assert(m.displayedPercent==90);

  // An isolated 80% reading at boot must not become a displayed estimate.
  SqeBattery::Meter reboot;
  set(adc,2000);assert(reboot.accept(adc));
  assert(reboot.displayedPercent==-1);
  set(adc,2050);
  for(int i=0;i<3;++i) {
    assert(reboot.accept(adc));
    assert(reboot.displayedPercent==-1);
  }
  assert(reboot.accept(adc)); // first 4000mV window falls out
  assert(reboot.displayedPercent==90);

  // A high boot spike also gets dropped before a percentage is shown.
  SqeBattery::Meter spike;
  set(adc,2120);assert(spike.accept(adc)); // 4240mV spike
  assert(spike.displayedPercent==-1);
  set(adc,2050);
  for(int i=0;i<4;++i)spike.accept(adc);
  assert(spike.displayedPercent==90);

  // Outliers do not change the trimmed mean or displayed bucket.
  adc[0]=100;adc[1]=2900;adc[2]=100;adc[3]=2900;
  assert(m.accept(adc));
  assert(m.rawMillivolts==4100);
  assert(m.displayedPercent==90);

  // Random-ish small drift should not cause SOC bounce.
  set(adc,2054);
  assert(m.accept(adc));
  assert(m.displayedPercent==90);

  // Single bad windows leave the last trusted state intact, then reset.
  set(adc,300);
  assert(!m.accept(adc)); assert(m.displayedPercent==90);
  assert(!m.accept(adc)); assert(m.displayedPercent==90);
  assert(!m.accept(adc)); assert(m.displayedPercent==-1);

  // Valid readings after failed ADC must again reach four-window consensus.
  set(adc,2100);
  for(int i=0;i<3;++i){assert(m.accept(adc));assert(m.displayedPercent==-1);}
  assert(m.accept(adc));
  assert(m.filteredMillivolts==4200);
  assert(m.displayedPercent==100);

  // Repeated disagreement returns UNKNOWN rather than fake precision.
  SqeBattery::Meter noisy;
  set(adc,2000);
  for(int i=0;i<4;++i)noisy.accept(adc);
  assert(noisy.displayedPercent==80);
  set(adc,2120);
  assert(noisy.accept(adc));
  assert(noisy.displayedPercent==80); // allow one transient
  assert(noisy.accept(adc));
  assert(noisy.displayedPercent==-1); // reject extended disagreement

  // Wide within-window noise should be rejected, even with plausible mean.
  for(size_t i=0;i<SqeBattery::kSamples;++i)adc[i]=(i&1)?1900:2200;
  assert(!noisy.accept(adc));
  assert(SqeBattery::voltagePercent(3200)==3);
  assert(SqeBattery::voltagePercent(4200)==100);
  return 0;
}
"""
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)
            (p/"battery.cpp").write_text(code)
            exe=p/"battery_test"
            subprocess.run([cc,"-std=c++17","-Wall","-Wextra","-Werror",
                            "-I",str(ROOT/"include"),str(p/"battery.cpp"),
                            "-o",str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_adc_is_stabilized_and_sampled_incrementally(self):
        src=(ROOT/"src"/"cyd32_ui.cpp").read_text(encoding="utf-8")
        self.assertIn("if(now<3000u)return;",src)
        self.assertIn("batteryPercent<0?3000u:15000u",src)
        self.assertIn("batteryAdcSamples[batteryAdcCount++]",src)
        self.assertIn("i<4&&batteryAdcCount<SqeBattery::kSamples",src)
        self.assertIn("batteryMeter.accept(batteryAdcSamples)",src)
        self.assertIn('"~%u%%"',src)
        self.assertIn('prevBatteryVoltageStep',src)
        self.assertIn('nextBatteryVoltageStep',src)
        self.assertIn("SqeBatteryCalibration::roundedPercent5(correctedBatteryMv())",src)
        self.assertIn("filtered_mV=",src)
        self.assertIn("core_spread_mV=",src)
        self.assertNotIn("batteryCharging",src)
        self.assertNotIn("batteryChargingEstimated",src)


if __name__ == "__main__":
    unittest.main()
