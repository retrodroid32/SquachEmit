# R14 LAB firmware build gate

- Target: CYD 3.2 / ST7798; **LAB firmware only**.
- Source: exact SquachWatch GPLv3 character and theme rendering engine, with SquachEmit transmitter-specific adapter.
- Must pass host fixtures and CYD32 firmware compilation, size guard, and artifact upload before offering as flashable LAB.
- A successful build is not a claim of verified panel rendering: portrait orientation, touch, animation, memory heap, and authentic costumes/backgrounds still require physical board verification.
- Never merge or tag a production release without explicit approval.
