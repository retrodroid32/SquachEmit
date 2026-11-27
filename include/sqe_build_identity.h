#pragma once
// CI overwrites this file in the build workspace with the exact checked-out
// source commit. Local builds explicitly report UNSTAMPED, not a fake SHA.
#ifndef SQE_BUILD_SHA
#define SQE_BUILD_SHA "UNSTAMPED"
#endif
