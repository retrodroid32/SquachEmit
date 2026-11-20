// SquachEmit Squachy quips — descriptive only, NEVER receiver evidence.
// Real device categories match include/scenario_library.h. Ring is a
// future-only dormant topic until a Ring scenario actually exists.
// Static flash-resident phrases; no heap allocations in the render loop.
#pragma once
#include <stddef.h>
#include <stdint.h>
namespace SqeQuips {
enum class Topic : uint8_t {
 Idle, Transmitting, Starting, Failed, Playlist, Suite,
 Axon, Drones, Flock, General, Hacker, IBeacon, Raven,
 SerialModules, SmartGlasses, Trackers,
 AirTag, Ble, Wifi, RemoteId, Ring, Rare, Easter
};
struct Bank { const char* const* lines; uint8_t count; };
#define SQE_BANK(v) { v, (uint8_t)(sizeof(v)/sizeof(v[0])) }

// ORIGINAL R14.13 phrases + all user-approved joke suggestions.
static const char* const kIdle[]={
 "READY FOR A SIGNAL CHECK!",
 "PICK A PROFILE. I WILL EMIT!",
 "172 TESTS. ONE LITTLE SQUACH.",
 "NO RF UNTIL YOU PRESS RUN.",
 "LET US TEST THAT DETECTOR!",
 "WAKE ME WHEN WE TRANSMIT!",
 "JUST A SQUACH WITH A DREAM.",
 "I SMELL RADIO WAVES!",
 "IS THIS THING ON?",
 "MY ANTENNA IS ITCHY.",
 "HERE WATCHY WATCHY!"
};
static const char* const kTransmitting[]={
 "SENDING SYNTHETIC SIGNAL...",
 "CHECK YOUR RECEIVER NOW!",
 "TEST SIGNAL IS ON THE AIR!",
 "PEW PEW! RADIO EDITION!",
 "I'M NOT SAYING IT'S ALIENS...",
 "BEHOLD! INVISIBLE NOISE!",
 "SQUACHY GOES BRRRR!",
 "SCIENCE! PROBABLY.",
 "SETTING PHASERS TO ANNOY!"
};
static const char* const kStarting[]={
 "STARTING WIFI TRANSMISSION",
 "SUMMONING THE WIFI SPIRITS!",
 "HOLD MY ANTENNA!",
 "CONNECTING THE DOTS...",
 "WIFI WIZARDRY IN PROGRESS!"
};
static const char* const kFailed[]={
 "WIFI START FAILED",
 "WELL, THAT WAS EMBARRASSING.",
 "THE GREMLINS DID IT!",
 "TASK FAILED SUCCESSFULLY!",
 "HAVE YOU TRIED MORE MAGIC?",
 "MY ANTENNA NEEDS THERAPY."
};
static const char* const kPlaylist[]={
 "FAVORITES PLAYLIST",
 "DJ SQUACH IS IN THE HOUSE!",
 "NOW SPINNING: RADIO WAVES!",
 "THE HITS JUST KEEP COMING!",
 "THIS MIX IS FIRE! (NOT RF)"
};
static const char* const kSuite[]={
 "TEST SUITE RUNNING",
 "RELEASE THE TEST GOBLINS!",
 "LET THE CHAOS BEGIN!",
 "ONE TEST TO RULE THEM ALL!",
 "I WAS BUILT FOR THIS. MAYBE.",
 "THE SCIENCE IS SCIENCING!",
 "SENDING EVERYTHING BUT THE KITCHEN SINK!"
};

// One family per real on-device CATEGORY.
static const char* const kAxon[]={
 "CAMERA READY? ACTION!",
 "I DEMAND A BODYCAM!",
 "SMILE! YOU'RE ON SQUACH CAM!",
 "SQUACHY, TAKE ONE!",
 "BODYCAM? I NEED A FURCAM!",
 "ROLLING! BUT ONLY IN TEST MODE."
};
static const char* const kDrones[]={
 "I BELIEVE I CAN WIFI!",
 "I HAVE THE HIGH GROUND!",
 "HOUSTON, WE HAVE A SQUACH!",
 "FLYING WITHOUT LEAVING HOME!",
 "MY PROPS ARE IMAGINARY!",
 "SQUACH CONTROL TO MAJOR TOM.",
 "ALTITUDE: EXTREMELY SQUACH."
};
static const char* const kFlock[]={
 "FLOCK AROUND AND FIND OUT!",
 "WHO LET THE BIRDS OUT?",
 "NO BIRDS WERE HARMED!",
 "THE PENGUINS ARE UP TO SOMETHING!",
 "I'M JUST HERE FOR THE BIRD SEED.",
 "FLOCK YEAH! TEST MODE!"
};
static const char* const kGeneral[]={
 "THIS ONE'S A CONTROL GROUP!",
 "FALSE ALARM? THAT'S A PASS!",
 "BORING SIGNAL. IMPORTANT TEST.",
 "I'M PAID IN DEBUG LOGS.",
 "NO SPOOFED MAGIC TODAY!",
 "THE CONTROL GROUP HAS SPOKEN."
};
static const char* const kHacker[]={
 "FLIPPER? I HARDLY KNOW HER!",
 "HACKERMAN HAS ENTERED THE CHAT!",
 "MY HOODIE HAS ROOT ACCESS.",
 "PINEAPPLE ON THE RADIO MENU?",
 "THIS IS A LAB, NOT A HEIST!",
 "SQUACH.EXE HAS STOPPED CARING.",
 "HACK THE PLANET!"
};
static const char* const kIBeacon[]={
 "IBEACON, THEREFORE I AM.",
 "BEACON OF HOPE? MORE LIKE BLE.",
 "MY BEACON BRINGS ALL THE BARS.",
 "YOU HAD ME AT ADVERTISING.",
 "SMALL PACKET. BIG PERSONALITY.",
 "I HAVE A MAJOR IDENTITY CRISIS."
};
static const char* const kRaven[]={
 "NEVERMORE... UNLESS IT'S A RETEST.",
 "QUOTH THE RAVEN: BLE!",
 "SQUACHY HEARD A RAVEN CALL!",
 "CROAK? NO, THAT'S MY DEBUGGER.",
 "BIRDS OF A FREQUENCY FLOCK.",
 "EDGAR ALLAN RADIO WAVES!"
};
static const char* const kSerialModules[]={
 "AT+SQUACH=1",
 "MY BAUD RATE HAS FEELINGS.",
 "SERIAL KILLER. OF BUGS!",
 "HELLO FROM THE UART SIDE!",
 "THE MODULE SAYS HELLO!",
 "I SPEAK FLUENT AT COMMANDS."
};
static const char* const kSmartGlasses[]={
 "RAY-BANS? I WEAR FUR SHADES.",
 "LOOK, MA! HANDS-FREE RF!",
 "SQUACHY HAS MAIN CHARACTER VISION.",
 "I SEE WHAT YOU TRANSMITTED.",
 "MY SHADES HAVE DEBUG MODE.",
 "SMILE FOR MY IMAGINARY GLASSES!"
};
static const char* const kTrackers[]={
 "BEEP BEEP! WHO'S THERE?",
 "TRACK ME? CATCH ME FIRST!",
 "SQUACHY KNOWS WHERE HE IS.",
 "THIS ISN'T A TREASURE MAP.",
 "I LOST MY OTHER ANTENNA!",
 "DON'T TAG MY FUR, PLEASE."
};

// Narrow test-profile/transport topics (not additional menu categories).
static const char* const kAirTag[]={
 "I'M NOT LOST. YOU ARE!",
 "WHERE'S MY OTHER SOCK?",
 "FOLLOW THE BEEP!"
};
static const char* const kBle[]={
 "LOW ENERGY. HIGH DRAMA.",
 "BLUETOOTH AND CHILL?",
 "BLE ME AWAY!"
};
static const char* const kWifi[]={
 "BEACON AND EGGS, ANYONE?",
 "SSID? MORE LIKE SOS!",
 "THE WIFI IS STRONG WITH ME."
};
static const char* const kRemoteId[]={
 "IDENTITY CRISIS INCOMING!",
 "HELLO! I AM A TEST DRONE!",
 "MY ID IS COMPLICATED."
};
// These three requested Ring lines are included but intentionally dormant:
// no Ring scenario/category exists in the current 172-case registry.
static const char* const kRing[]={
 "DING DONG! IT'S SQUACH!",
 "SPECIAL DELIVERY: RF!",
 "YOUR DOORBELL KNOWS TOO MUCH."
};
// All 10 requested rare/easter lines are preserved verbatim.
static const char* const kRare[]={
 "I KNOW WHAT YOU DID LAST WIFI.",
 "I PUT THE SQUACH IN SQUELCH.",
 "MY OTHER ANTENNA IS A YAGI.",
 "I CAN HEAR YOUR TOASTER.",
 "THIS IS FINE. PROBABLY."
};
static const char* const kEaster[]={
 "THE GOVERNMENT STOLE MY WIFI.",
 "ERROR 404: COMMON SENSE.",
 "I'M 90% FUR, 10% RF.",
 "DON'T TELL THE FCC ABOUT ME!",
 "SHHH... I'M DOING SCIENCE."
};
static const char* const kRepeatRun="AGAIN? SERIOUSLY?";
static const char* const kLongIdle="DID YOU FORGET ABOUT ME?";

inline Bank bank(Topic topic){
 switch(topic){
  case Topic::Idle:return SQE_BANK(kIdle);
  case Topic::Transmitting:return SQE_BANK(kTransmitting);
  case Topic::Starting:return SQE_BANK(kStarting);
  case Topic::Failed:return SQE_BANK(kFailed);
  case Topic::Playlist:return SQE_BANK(kPlaylist);
  case Topic::Suite:return SQE_BANK(kSuite);
  case Topic::Axon:return SQE_BANK(kAxon);
  case Topic::Drones:return SQE_BANK(kDrones);
  case Topic::Flock:return SQE_BANK(kFlock);
  case Topic::General:return SQE_BANK(kGeneral);
  case Topic::Hacker:return SQE_BANK(kHacker);
  case Topic::IBeacon:return SQE_BANK(kIBeacon);
  case Topic::Raven:return SQE_BANK(kRaven);
  case Topic::SerialModules:return SQE_BANK(kSerialModules);
  case Topic::SmartGlasses:return SQE_BANK(kSmartGlasses);
  case Topic::Trackers:return SQE_BANK(kTrackers);
  case Topic::AirTag:return SQE_BANK(kAirTag);
  case Topic::Ble:return SQE_BANK(kBle);
  case Topic::Wifi:return SQE_BANK(kWifi);
  case Topic::RemoteId:return SQE_BANK(kRemoteId);
  case Topic::Ring:return SQE_BANK(kRing);
  case Topic::Rare:return SQE_BANK(kRare);
  case Topic::Easter:return SQE_BANK(kEaster);
 }
 return SQE_BANK(kIdle);
}
inline const char* choose(Topic topic,uint32_t draw){
 const Bank b=bank(topic);
 return b.lines[draw%b.count];
}
// The only speech banks eligible for physical device popups.
// 60% of active popups are the transmitting file's actual device category;
// 40% are the generic TRANSMITTING bank. Off-air is always IDLE.
// Invalid or unresolved active case IDs safely use TRANSMITTING only.
inline Topic stateTopic(bool onAir,Topic fileCategory,uint32_t roll){
 if(!onAir)return Topic::Idle;
 if(fileCategory<Topic::Axon||fileCategory>Topic::Trackers)
   return Topic::Transmitting;
 return roll%100u<60u?fileCategory:Topic::Transmitting;
}
// Every quip is ASCII so TFT_eSPI's classic built-in fonts can draw it.
#undef SQE_BANK
} // namespace SqeQuips
