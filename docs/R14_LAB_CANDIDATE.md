# R14 Original-Art LAB candidate

This branch is intentionally a nonproduction release candidate based on the exact SquachWatch procedural artwork integration in draft PR #60.

**Flash gate:** CI must pass host tests, compile the ESP32-2432S032R/ST7798 firmware, stage all 4 firmware flash segments and pass checksum/partition validation. Firmware is not hardware-verified until it is flashed and inspected on CYD 3.2. Preserve the unmerged source and do not tag production.

**Acceptance:** actual original SquachWatch backgrounds, original outfit geometry, smooth animation without panel erases or boot loops, native portrait orientation, working touch and RUN/STOP. Custom Black Hoodie is SquachEmit-specific.
