"""Audit actual on-air identity exceptions in the 172-case v1.1.0 catalog.

This compiles the *same* C++ identity/registered-OUI helpers used by the
firmware. It cannot prove physical RF reception or manufacturer authenticity.
"""
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]


class OnAirIdentityAudit(unittest.TestCase):
    def test_catalog_contains_exactly_37_explicit_vendor_beacons(self):
        cases=json.loads((ROOT/'tests/cases.json').read_text())
        ids=[x['id'] for x in cases]
        oui_cases=[x for x in cases if x['id'].startswith('OUI-')]
        self.assertEqual(len(cases),172)
        self.assertEqual(len(ids),len(set(ids)))
        self.assertEqual(len(oui_cases),37)
        self.assertEqual(sum(x['id'].startswith('OUI-DJI-') for x in oui_cases),10)
        self.assertEqual(sum(not x['id'].startswith('OUI-DJI-') for x in oui_cases),27)
        dji=(ROOT/'include/dji_oui_lab.h').read_text()
        other=(ROOT/'include/uav_vendor_oui_lab.h').read_text()
        for case in oui_cases:
            with self.subTest(id=case['id']):
                self.assertEqual((case['carrier'],case['kind']),('WIFI','WifiAp'))
                self.assertIn('NO RID',case['description'])
                self.assertTrue(case['text'].startswith('SQE-LAB-'))
                self.assertIn(case['id'],dji if case['id'].startswith('OUI-DJI-') else other)
        main=(ROOT/'src/main.cpp').read_text()
        self.assertIn('SquachDjiOuiLab::apply(s->id,out)',main)
        self.assertIn('SquachUavVendorLab::apply(s->id,out)',main)
        self.assertIn('TEST,OUI_ONLY',main)

    def test_native_default_and_registered_prefix_boundaries(self):
        compiler=shutil.which('g++') or shutil.which('clang++')
        if not compiler:self.skipTest('C++ compiler not installed')
        cpp=r'''
#include "identity.h"
#include "dji_oui_lab.h"
#include "uav_vendor_oui_lab.h"
#include <cassert>
#include <cstring>
#include <set>
#include <string>
int main() {
 static_assert(SquachDjiOuiLab::kCount==10, "DJI count");
 static_assert(SquachUavVendorLab::kCount==27, "extended UAS count");
 std::set<std::string> ids;
 for(size_t i=0;i<172;++i) {
   uint8_t w[6],b[6];char tmp[18];
   SquachIdentity::derive(0x112233445566ULL,i,SquachIdentity::Radio::Wifi,w);
   SquachIdentity::derive(0x112233445566ULL,i,SquachIdentity::Radio::Ble,b);
   assert((w[0]&3)==2);assert((b[0]&0xc0)==0xc0);
   SquachIdentity::text(w,tmp);assert(ids.insert(tmp).second);
   SquachIdentity::text(b,tmp);assert(ids.insert(tmp).second);
 }
 assert(ids.size()==344);
 for(size_t i=0;i<SquachDjiOuiLab::kCount;++i) {
   const auto& e=SquachDjiOuiLab::kCases[i];
   uint8_t a[6],before[6];
   SquachIdentity::derive(0x112233445566ULL,135+i,SquachIdentity::Radio::Wifi,a);
   memcpy(before,a,6);
   assert(SquachDjiOuiLab::apply(e.caseId,a));
   assert(memcmp(a,e.prefix,3)==0);
   assert(memcmp(a+3,before+3,3)==0);
   assert((a[0]&3)==0);
 }
 for(size_t i=0;i<SquachUavVendorLab::kCount;++i) {
   const auto& e=SquachUavVendorLab::kCases[i];
   uint8_t a[6],before[6];
   SquachIdentity::derive(0x112233445566ULL,145+i,SquachIdentity::Radio::Wifi,a);
   memcpy(before,a,6);
   assert(SquachUavVendorLab::apply(e.caseId,a));
   assert(e.bits==24||e.bits==28||e.bits==36);
   size_t full=e.bits/8;
   assert(memcmp(a,e.prefix,full)==0);
   if(e.bits%8) {
     assert((a[full]&0xf0)==(e.prefix[full]&0xf0));
     assert((a[full]&0x0f)==(before[full]&0x0f));
   }
   for(size_t j=full+(e.bits%8!=0);j<6;++j)assert(a[j]==before[j]);
   assert((a[0]&3)==0);
 }
 uint8_t safe[6]={2,1,2,3,4,5};
 assert(!SquachDjiOuiLab::apply("RID-DJI-MINI3",safe));
 assert(!SquachUavVendorLab::apply("RID-WIFI-DJI-MINI3",safe));
 assert(safe[0]==2);
 return 0;
}'''
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/'audit.cpp'
            exe=Path(d)/'audit'
            src.write_text(cpp)
            subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-Werror',
                            '-I',str(ROOT/'include'),str(src),'-o',str(exe)],
                           check=True,capture_output=True,text=True)
            subprocess.run([str(exe)],check=True,capture_output=True,text=True)

    def test_docs_disclose_on_air_prefix_exception(self):
        notes=(ROOT/'docs/IDENTITY_AUDIT.md').read_text()
        limits=(ROOT/'docs/LIMITATIONS.md').read_text()
        dji=(ROOT/'docs/DJI_MODEL_PROFILES.md').read_text()
        self.assertIn('37 deliberate exceptions',notes)
        self.assertIn('37 bounded OUI-only',limits)
        self.assertIn('registered DJI /24',dji)
        self.assertNotIn('only by *offline*',limits)


if __name__=='__main__':unittest.main()
