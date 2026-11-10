# Squachy source-port LAB (CYD 3.2)

The mascot's base proportions, cowlick, shades, face, limb shapes, idle bob,
wave, blink, bounce, walk, dance, startled and sleepy expressions are source
adaptations from SquachWatch-CYD src/squachy.cpp, licensed GPLv3.

This is a **core-character port**, not yet the full 8,291-line animation state
machine, clothing system, speech behavior, or visit/mesh interaction engine.
It is kept isolated from SquachEmit's RF engine and serial controls.

The attached 112x111 16-bit sprite consumes about 24.3 KB heap; animation
updates only its rectangle at roughly 8 frames per second. If allocation
fails, the previous static character remains. Confirm both RAM and panel
behavior in hardware. Do not merge until hardware verification.

License: source-derived port is GPL-3.0-only, not MIT. Firmware distributed
with the linked port requires GPLv3 compliance for the combined work. Other
MIT-authored source retains its MIT notices. See LICENSES/GPL-3.0.txt.
