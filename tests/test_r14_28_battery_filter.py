"""R14.28 voltage filter regression: host-compile real C++ used by CYD32 firmware."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class BatteryFilterTests(unittest.TestCase):
    def test_filtered_voltage_and_boot_recovery(self):
        cc = shutil.which("g++") or shutil.which("clang++")
        if cc is None:
            self.skipTest("C++ compiler unavailable")
        code = r"""
#include "sqe_battery_filter.h"
#include <assert.h>
int main(){
  static_assert(SqeBattery::kSamples==32, "ADC trim window");
  uint16_t adc[SqeBattery::kSamples]={0};
  for(auto &v:adc)v=2050;
  SqeBattery::Meter m;
  assert(m.accept(adc));
  assert(m.rawMillivolts==4100);
  assert(m.filteredMillivolts==4100);
  assert(m.displayedPercent==90); // 92% voltage estimate, display in 5% steps
  // Outliers must not change the reading at all.
  adc[0]=100;adc[1]=2900;adc[2]=100;adc[3]=2900;
  assert(m.accept(adc));
  assert(m.rawMillivolts==4100);
  assert(m.displayedPercent==90);
  // Mild ADC variation should not bounce the displayed bucket.
  for(auto &v:adc)v=2054;
  assert(m.accept(adc));
  assert(m.displayedPercent==90);
  // A single bad read window is not permission to erase the gauge.
  for(auto &v:adc)v=300;
  assert(!m.accept(adc)); assert(m.displayedPercent==90);
  assert(!m.accept(adc)); assert(m.displayedPercent==90);
  assert(!m.accept(adc)); assert(m.displayedPercent==-1);
  // Genuine valid reading after disconnection/boot recovers.
  for(auto &v:adc)v=2100;
  assert(m.accept(adc));
  assert(m.rawMillivolts==4200);
  assert(m.filteredMillivolts==4200);
  assert(m.displayedPercent==100);
  // Fully-charged readings remain approximate voltage estimates.
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
        self.assertIn("batteryAdcSamples[batteryAdcCount++]",src)
        self.assertIn("i<4 && batteryAdcCount<SqeBattery::kSamples",src)
        self.assertIn("batteryMeter.accept(batteryAdcSamples)",src)
        self.assertIn('"~%d%%"',src)
        self.assertIn("filtered_mV=",src)
        self.assertIn("spread_mV=",src)


if __name__ == "__main__":
    unittest.main()
