// Original SquachWatch-CYD TERMINAL LOG renderer and XYZZY events.
// Source src/theme.cpp blob 0b4ee801e295d924a15934d2f2324f1475cfd482.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#if defined(SQE_CYD32)
#include "squachwatch_synthwave_exact.h"
#include <string.h>
namespace SquachWatchScenes {
constexpr uint16_t GREEN = 0x07E0;
constexpr uint16_t VAPOR_YELLOW = 0xFFD2;
constexpr uint16_t VAPOR_PINK = 0xFB99;
static const uint32_t XYZZY_SHOW_MS = 3500;
static const uint8_t  XYZZY_NEEDED  = 3;
static uint32_t s_xyzzyNextAt = 0, s_xyzzyStart = 0, s_xyzzyHitAt = 0;
// Where the word is RIGHT NOW, so a tap can find it -- same shape as the
// lodge and the eye: published every frame it is up, stale after 250 ms.
static int      s_xyzzyX[2] = { 0, 0 }, s_xyzzyY = 0, s_xyzzyHW = 0, s_xyzzyHH = 0;
static uint32_t s_xyzzyAt = 0;
static uint8_t  s_xyzzyTaps = 0, s_xyzzyPending = 0;

static inline uint8_t consumeXyzzy() {
    const uint8_t n = s_xyzzyPending;
    s_xyzzyPending = 0;
    return n;
}

// Bring the word up now rather than on its own clock: the console's XYZZY
// command and the emulator, so the unlock can be tested without waiting.
static inline void summonXyzzy() { if (!s_xyzzyStart) s_xyzzyNextAt = 1; }

static void drawXyzzy(TFT_eSPI& t, uint32_t now, int yStart, int bandH) {
    if (!s_xyzzyNextAt) s_xyzzyNextAt = now + (uint32_t)random(12000, 24000);
    if (!s_xyzzyStart) {
        if (now < s_xyzzyNextAt) return;
        s_xyzzyStart = now ? now : 1;
        s_xyzzyHitAt = 0;
    }
    const bool hit = s_xyzzyHitAt != 0;
    if (hit ? (now - s_xyzzyHitAt > 450) : (now - s_xyzzyStart > XYZZY_SHOW_MS)) {
        s_xyzzyStart  = 0;
        s_xyzzyHitAt  = 0;
        s_xyzzyNextAt = now + (uint32_t)random(14000, 28000);
        return;
    }
    const int w  = t.width();
    const int sz = (w >= 300) ? 2 : 1;
    const int cw = 6 * sz * 5, ch = 8 * sz;              // "XYZZY"
    const int cy = yStart + bandH / 3;
    // Tapped: it flashes inverted, then goes. Otherwise yellow on the dark,
    // with a cursor blinking after it, so it reads as typed.
    const uint16_t ink = hit ? BG : VAPOR_YELLOW, paper = hit ? VAPOR_YELLOW : BG;
    t.setTextSize(sz);
    t.setTextColor(ink, paper);
    for (uint8_t k = 0; k < 2; k++) {
        const int cx = k == 0 ? w / 5 : w - w / 5;
        t.fillRect(cx - cw / 2 - 4, cy - ch / 2 - 3, cw + 8 + 6 * sz, ch + 6, paper);
        t.drawRect(cx - cw / 2 - 4, cy - ch / 2 - 3, cw + 8 + 6 * sz, ch + 6, VAPOR_YELLOW);
        t.setCursor(cx - cw / 2, cy - ch / 2);
        t.print("XYZZY");
        if (!hit && ((now / 300) & 1)) t.fillRect(cx + cw / 2 + sz, cy - ch / 2, 5 * sz, ch, VAPOR_YELLOW);
        s_xyzzyX[k] = cx + 3 * sz;
    }
    t.setTextSize(1);
    if (!hit) {
        // Generous: it is a word to be poked at, not a target to be hit.
        s_xyzzyY = cy; s_xyzzyHW = cw / 2 + 4 + 3 * sz + 8; s_xyzzyHH = ch / 2 + 3 + 10;
        s_xyzzyAt = now ? now : 1;
    }
}

// Two independently-scrolling columns (different add intervals so they
// never sync up) side by side, so the log fills the full screen width
// instead of a narrow strip down the left.
struct TermCol { char lines[20][40]; bool special[20]; uint8_t count; uint32_t lastAdd; };

static void termGenLine(char* out, size_t outSize) {
    static const char* VERBS[] = { "INIT", "PROBE", "SCAN", "MOUNT", "AUTH", "PARSE", "TRACE", "PING", "SYNC", "LOAD" };
    static const char* NOUNS[] = { "kernel", "rf-stack", "node", "socket", "buffer", "daemon", "cache", "uplink", "packet", "handshake" };
    static const char* TAILS[] = { "OK", "DONE", "0x%02X", "READY", "--", "FAIL", "..." };
    const char* v  = VERBS[random(0, 10)];
    const char* n  = NOUNS[random(0, 10)];
    const char* tl = TAILS[random(0, 7)];
    char tbuf[12];
    if (strcmp(tl, "0x%02X") == 0) {
        snprintf(tbuf, sizeof(tbuf), "0x%02X", (unsigned)random(0, 256));
        tl = tbuf;
    }
    snprintf(out, outSize, "%-5s %-9s %s", v, n, tl);
}

// Rare easter-egg lines — a small chance each new line is one of these
// instead of the usual generated boot chatter, rendered in a
// different color so it actually stands out if you're watching.
static const char* TERM_EGG_LINES[] = {
    "SQUACHY WAS HERE",
    "// hi mom",
    "sudo make me a squachwich",
    "ACCESS: GRANTED (nice)",
    "root@squachwatch: <3",
    "you found the secret line",
};
static const uint8_t TERM_EGG_COUNT = sizeof(TERM_EGG_LINES) / sizeof(TERM_EGG_LINES[0]);

static void termAdvance(TermCol& c, uint32_t now, uint32_t interval) {
    if (now - c.lastAdd <= interval) return;
    c.lastAdd = now;
    if (c.count < 20) c.count++;
    for (uint8_t i = 19; i > 0; i--) {
        memcpy(c.lines[i], c.lines[i - 1], sizeof(c.lines[i]));
        c.special[i] = c.special[i - 1];
    }
    if (random(0, 45) == 0) {
        strncpy(c.lines[0], TERM_EGG_LINES[random(0, TERM_EGG_COUNT)], sizeof(c.lines[0]) - 1);
        c.lines[0][sizeof(c.lines[0]) - 1] = '\0';
        c.special[0] = true;
    } else {
        termGenLine(c.lines[0], sizeof(c.lines[0]));
        c.special[0] = false;
    }
}

static void termRender(TFT_eSPI& t, const TermCol& c, int x, int yStart, int visLines) {
    for (uint8_t i = 0; i < visLines && i < c.count; i++) {
        uint16_t col;
        if (c.special[i]) {
            col = VAPOR_PINK;
        } else {
            uint8_t fade = 255 - i * (255 / (visLines > 0 ? visLines : 1));
            col = blend(BG, GREEN, fade);
        }
        t.setTextColor(col, BG);
        t.setCursor(x, yStart + i * 9);
        t.print(c.lines[i]);
    }
}

static TermCol* s_termCols = nullptr;
static void releaseTerm() {
    if (s_termCols) { free(s_termCols); s_termCols = nullptr; }
}

static inline void drawTerminalLog(TFT_eSPI& t, uint32_t now, int yStart, int yEnd) {
    if (!s_termCols) {
        // calloc, not malloc: a TermCol is only correct starting empty, and
        // a static one used to get that for free.
        s_termCols = (TermCol*)calloc(2, sizeof(TermCol));
        if (!s_termCols) { t.fillRect(0, yStart, t.width(), yEnd - yStart, BG); return; }
    }
    TermCol* const cols = s_termCols;
    int w = t.width();
    int bandH = yEnd - yStart;
    if (bandH < 20) return;
    int visLines = bandH / 9;
    if (visLines > 20) visLines = 20;

    termAdvance(cols[0], now, 180);
    termAdvance(cols[1], now, 230);   // offset cadence so the two columns don't lock-step

    t.fillRect(0, yStart, w, bandH, BG);
    t.setTextSize(1);
    t.setTextWrap(false);
    termRender(t, cols[0], 4, yStart, visLines);
    termRender(t, cols[1], w / 2 + 4, yStart, visLines);
    t.drawFastVLine(w / 2, yStart, bandH, blend(BG, GREEN, 60));

    // Rare dramatic beat: a full-width banner line flashes across both
    // columns for under a second, breaking the indifferent scroll with
    // an "something just happened" moment instead of an endless,
    // personality-free log.
    static bool     flashOn = false;
    static uint32_t flashStart = 0, flashNextAt = 0;
    static uint8_t  flashLine = 0;
    static bool     flashInited = false;
    static const char* const FLASH_LINES[] = {
        "ACCESS GRANTED", "INTRUSION DETECTED", "CONNECTION ESTABLISHED",
        "ROOT SHELL OPEN", "TRACE COMPLETE",
    };
    if (!flashInited) { flashNextAt = now + (uint32_t)random(6000, 14000); flashInited = true; }
    if (!flashOn && now >= flashNextAt) {
        flashOn = true;
        flashStart = now;
        flashLine = (uint8_t)random(0, 5);
    }
    if (flashOn) {
        if (now - flashStart < 900) {
            uint16_t col = ((now / 120) % 2) ? GREEN : blend(BG, GREEN, 150);
            t.fillRect(0, yStart + bandH / 2 - 6, w, 12, BG);
            t.setTextColor(col, BG);
            int mw = t.textWidth(FLASH_LINES[flashLine]);
            t.setCursor((w - mw) / 2, yStart + bandH / 2 - 4);
            t.print(FLASH_LINES[flashLine]);
        } else {
            flashOn = false;
            flashNextAt = now + (uint32_t)random(9000, 20000);
        }
    }

    // The magic word, over everything else the terminal has up.
    drawXyzzy(t, now, yStart, bandH);
}


} // namespace SquachWatchScenes
#endif
