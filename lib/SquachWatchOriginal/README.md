# SquachWatch Original Engine — R14 integration

Original project: https://github.com/retrodroid32/SquachWatch-CYD

This directory contains a direct source copy of the original procedural
Squachy renderer (body, animation, outfits) and full ten-background Theme
renderer. They retain upstream GPL-3.0-only licensing; see
`licenses/SquachWatch-GPL-3.0.txt`.

Source blob identifiers:
- `src/squachy.cpp`: `0222b45cf33330fa10fd45e4ba9efb5c67f9ddbf`
- `src/theme.cpp`: `0b4ee801e295d924a15934d2f2324f1475cfd482`

SquachEmit-specific board integration is isolated behind
`SQE_CYD32` and `SQE_ORIGINAL_SQUACHY`. Radio and detection logic
from SquachWatch is not intended to run on the SquachEmit transmitter.
The original artist/logic is shared, not recreated.

This is an EXPERIMENTAL draft implementation, not a firmware release;
build and physical CYD 3.2 verification must pass before any deployment.
