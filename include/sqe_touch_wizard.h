// SquachEmit CYD32 — exact SquachWatch touch calibration flow, adapted
// from retrodroid32/SquachWatch-CYD src/touch_cal.cpp.
// Five inset targets + final verification dot. Each target waits for a
// stable held sample and a distinct finger release, unlike TFT_eSPI's
// legacy calibrateTouch(), which can consume the same press twice.
#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include "sqe_touch_fit.h"
namespace SqeTouchWizard {
using RawReader = bool (*)(int16_t&,int16_t&);
enum class Outcome : uint8_t { SAVED, SKIPPED, NO_TOUCH, FAILED };
static float s_density=1.0f;
inline void setDensityScale(float scale){s_density=scale>0.5f?scale:1.0f;}
// ---- The interactive flow ----

namespace {

struct Screen {
    TFT_eSPI& t;
    int w, h;
    uint16_t bg, fg, accent;
    uint8_t headSize, bodySize;   // text sizes: bigger on the 3.5"
};

void centred(const Screen& s, const char* text, int y, uint8_t size, uint16_t colour) {
    s.t.setTextFont(1);
    s.t.setTextSize(size);
    s.t.setTextColor(colour, s.bg);
    s.t.setCursor((s.w - s.t.textWidth(text)) / 2, y);
    s.t.print(text);
}

// A full-screen notice, up to three lines, held for `ms`.
void notice(const Screen& s, uint16_t colour, const char* l1, const char* l2, const char* l3,
            uint32_t ms) {
    s.t.fillRect(0, 0, s.w, s.h, s.bg);
    const int lh1 = 8 * s.headSize, lh2 = 8 * s.bodySize;
    int total = lh1 + (l2 ? lh2 + 8 : 0) + (l3 ? lh2 + 4 : 0);
    int y = (s.h - total) / 2;
    centred(s, l1, y, s.headSize, colour);
    y += lh1 + 8;
    if (l2) { centred(s, l2, y, s.bodySize, s.fg); y += lh2 + 4; }
    if (l3) centred(s, l3, y, s.bodySize, s.fg);
    delay(ms);
}

// The line of instructions under the centre of the screen. Below the centre
// target, and above the bottom two, in every rotation of every panel.
int statusY(const Screen& s) { return s.h / 2 + 22 + 4 * s.bodySize; }

void status(const Screen& s, const char* text) {
    const int y = statusY(s);
    s.t.fillRect(0, y - 2, s.w, 8 * s.bodySize + 4, s.bg);
    centred(s, text, y, s.bodySize, s.fg);
}

const int RING = 16;

void drawTarget(const Screen& s, int cx, int cy, bool dot) {
    if (dot) {
        s.t.fillCircle(cx, cy, 6, s.accent);
        return;
    }
    s.t.drawCircle(cx, cy, RING, s.accent);
    s.t.drawCircle(cx, cy, RING - 1, s.accent);
    s.t.drawFastHLine(cx - RING - 6, cy, 12, s.accent);
    s.t.drawFastHLine(cx + RING - 6, cy, 12, s.accent);
    s.t.drawFastVLine(cx, cy - RING - 6, 12, s.accent);
    s.t.drawFastVLine(cx, cy + RING - 6, 12, s.accent);
    s.t.fillCircle(cx, cy, 2, s.accent);
}

void eraseTarget(const Screen& s, int cx, int cy) {
    const int r = RING + 7;
    s.t.fillRect(cx - r, cy - r, 2 * r + 1, 2 * r + 1, s.bg);
}

void insertionSort(int16_t* v, int n) {
    for (int i = 1; i < n; i++) {
        int16_t x = v[i];
        int j = i - 1;
        while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; j--; }
        v[j + 1] = x;
    }
}

// Waits for a finger on the target and takes the MEDIAN of its readings over
// holdMs of steady contact -- a resistive panel's first few milliseconds of
// a press, and its last, read wherever the pressure happens to be, and a
// median ignores them where an average would not.
//
// Lifting early starts that target again rather than keeping a short sample.
// Every wait is bounded: an unbounded one once froze a board with a flaky
// touch signal forever, with nothing in the log (see git history).
bool holdSample(const Screen& s, RawReader readRaw, int cx, int cy, bool dot, uint32_t holdMs,
                int16_t& aOut, int16_t& bOut) {
    const uint32_t TOUCH_TIMEOUT_MS   = 20000;
    const uint32_t RELEASE_TIMEOUT_MS = 5000;
    const uint32_t SETTLE_MS          = 60;   // skip the press itself
    const int      MAX_MISSES         = 6;    // ~60 ms of dropout is a lift
    const int      CAP                = 64;
    static int16_t as[CAP], bs[CAP];

    drawTarget(s, cx, cy, dot);
    const uint32_t waitStart = millis();
    for (;;) {
        int16_t a, b;
        while (!readRaw(a, b)) {
            if (millis() - waitStart > TOUCH_TIMEOUT_MS) { eraseTarget(s, cx, cy); return false; }
            delay(10);
        }
        if (!dot) s.t.fillCircle(cx, cy, RING - 5, AMBER);
        const uint32_t down = millis();
        int n = 0, misses = 0;
        bool lifted = false;
        while (millis() - down < SETTLE_MS + holdMs) {
            if (readRaw(a, b)) {
                misses = 0;
                if (millis() - down >= SETTLE_MS && n < CAP) { as[n] = a; bs[n] = b; n++; }
            } else if (++misses > MAX_MISSES) {
                lifted = true;
                break;
            }
            delay(8);
        }
        if (lifted || n < 5) {
            // Back to an empty ring and wait again.
            eraseTarget(s, cx, cy);
            drawTarget(s, cx, cy, dot);
            continue;
        }
        insertionSort(as, n);
        insertionSort(bs, n);
        aOut = as[n / 2];
        bOut = bs[n / 2];
        break;
    }
    if (!dot) s.t.fillCircle(cx, cy, RING - 5, GREEN);

    const uint32_t releaseStart = millis();
    int16_t a, b;
    int clear = 0;
    // Released means several reads in a row with nothing, not one: a
    // resistive panel drops out for a read or two mid-press.
    while (clear < 4 && millis() - releaseStart < RELEASE_TIMEOUT_MS) {
        clear = readRaw(a, b) ? 0 : clear + 1;
        delay(10);
    }
    delay(120);
    eraseTarget(s, cx, cy);
    return true;
}

void waitRelease(RawReader readRaw) {
    const uint32_t start = millis();
    int16_t a, b;
    int clear = 0;
    while (clear < 4 && millis() - start < 5000) {
        clear = readRaw(a, b) ? 0 : clear + 1;
        delay(10);
    }
}

// The first screen. START anywhere but the SKIP button -- a board whose
// touch is off is the reason this screen exists, so the button that does
// something is the big, forgiving one and SKIP has to be meant.
enum class IntroChoice { START, SKIP, TIMEOUT };
IntroChoice intro(const Screen& s, RawReader readRaw, uint8_t rot, const TouchFit::Fit* current) {
    s.t.fillRect(0, 0, s.w, s.h, s.bg);
    const int lh = 8 * s.bodySize;
    int y = s.h / 6;
    centred(s, "TOUCH SETUP", y, s.headSize, s.accent);
    y += 8 * s.headSize + 12;
    // 24 characters at most per line: the 3.5" in portrait fits no more at
    // its bigger text size.
    centred(s, "Tap and hold 1 second", y, s.bodySize, s.fg);
    y += lh + 4;
    centred(s, "on each target.", y, s.bodySize, s.fg);
    y += lh + 12;
    centred(s, "Tap anywhere to start.", y, s.bodySize, GREEN);

    int skipX = 0, skipY = 0, skipW = 0, skipH = 0;
    if (current) {
        skipW = s.w / 2;
        skipH = 18 + lh;
        skipX = (s.w - skipW) / 2;
        skipY = s.h - skipH - s.h / 10;
        s.t.drawRoundRect(skipX, skipY, skipW, skipH, 6, s.fg);
        s.t.setTextFont(1);
        s.t.setTextSize(s.bodySize);
        s.t.setTextColor(s.fg, s.bg);
        s.t.setCursor(skipX + (skipW - s.t.textWidth("SKIP")) / 2, skipY + (skipH - lh) / 2);
        s.t.print("SKIP");
    }

    // Anything already on the glass (the finger that opened this) has to
    // come off first, or it starts the calibration by itself.
    waitRelease(readRaw);
    const uint32_t start = millis();
    int16_t a, b;
    while (!readRaw(a, b)) {
        if (millis() - start > 30000) return IntroChoice::TIMEOUT;
        delay(10);
    }
    IntroChoice choice = IntroChoice::START;
    if (current) {
        float sx, sy;
        TouchFit::toScreen(*current, a, b, rot, sx, sy);
        if (sx >= skipX && sx < skipX + skipW && sy >= skipY && sy < skipY + skipH)
            choice = IntroChoice::SKIP;
    }
    waitRelease(readRaw);
    delay(150);
    return choice;
}

}  // namespace

Outcome runInteractive(TFT_eSPI& t, RawReader readRaw, uint8_t rot,
                       const TouchFit::Fit* current, int16_t minSpread,
                       uint16_t bg, uint16_t fg, uint16_t accent,
                       TouchFit::Fit& out) {
    const int w = t.width(), h = t.height();
    const int w0 = (rot & 1) ? h : w, h0 = (rot & 1) ? w : h;
    const bool big = (w > h ? w : h) >= 400;
    const Screen s = { t, w, h, bg, fg, accent, (uint8_t)(big ? 3 : 2), (uint8_t)(big ? 2 : 1) };
    const int shortSide = w < h ? w : h;

    switch (intro(s, readRaw, rot, current)) {
        case IntroChoice::SKIP:    t.fillRect(0, 0, w, h, bg); return Outcome::SKIPPED;
        case IntroChoice::TIMEOUT: t.fillRect(0, 0, w, h, bg); return Outcome::NO_TOUCH;
        default: break;
    }

    // How far a tap may land from its target and still count. Fingers are
    // not styluses and a resistive panel wanders a pixel or three; a slipped
    // finger is tens of pixels.
    const float residualTol = fmaxf(9.0f, 0.04f * shortSide) * s_density;
    const float verifyTol   = fmaxf(12.0f, 0.05f * shortSide) * s_density;

    for (int attempt = 0; attempt < 2; attempt++) {
        t.fillRect(0, 0, w, h, bg);
        float a[TouchFit::TARGETS], b[TouchFit::TARGETS];
        float nx[TouchFit::TARGETS], ny[TouchFit::TARGETS];
        float sxs[TouchFit::TARGETS], sys[TouchFit::TARGETS];
        for (int i = 0; i < TouchFit::TARGETS; i++) {
            float fx, fy;
            TouchFit::targetFrac(i, fx, fy);
            sxs[i] = fx * w;
            sys[i] = fy * h;
            char line[32];
            snprintf(line, sizeof(line), "TAP AND HOLD 1 SECOND  %d/%d", i + 1, TouchFit::TARGETS);
            status(s, line);
            Serial.printf("CYD32,TOUCH,TARGET,%d/%d\n",i+1,TouchFit::TARGETS);
            int16_t ra, rb;
            if (!holdSample(s, readRaw, (int)sxs[i], (int)sys[i], false, 900, ra, rb)) {
                notice(s, AMBER, "NO TOUCH", "Nothing changed.", nullptr, 1500);
                t.fillRect(0, 0, w, h, bg);
                return Outcome::NO_TOUCH;
            }
            a[i] = ra; b[i] = rb;
            TouchFit::screenToNative(sxs[i], sys[i], rot, w0, h0, nx[i], ny[i]);
        }

        // Diagonal corners (0 and 2, 1 and 3) must read well apart: a dead or
        // stuck digitiser reads the same number everywhere.
        auto apart = [&](int i, int j) {
            return fabsf(a[i] - a[j]) + fabsf(b[i] - b[j]) >= minSpread;
        };
        TouchFit::Fit fit;
        bool ok = apart(0, 2) && apart(1, 3) &&
                  TouchFit::solve(a, b, nx, ny, TouchFit::TARGETS, w0, h0, fit);
        float worst = 0;
        if (ok) {
            for (int i = 0; i < TouchFit::TARGETS; i++) {
                float gx, gy;
                TouchFit::toScreen(fit, a[i], b[i], rot, gx, gy);
                worst = fmaxf(worst, hypotf(gx - sxs[i], gy - sys[i]));
            }
            ok = worst <= residualTol;
        }
        Serial.printf("[touchcal] attempt %d: %s, worst tap %.1f px (limit %.1f)\n",
                      attempt + 1, ok ? "fits" : "does not fit", worst, residualTol);
        if (!ok) {
            if (attempt == 0) notice(s, AMBER, "ONCE MORE", "A tap slipped.", "Hold each target steady.", 1800);
            continue;
        }

        // The proof: one more dot, somewhere none of the five were, mapped
        // through the new fit. Shows where it landed either way.
        t.fillRect(0, 0, w, h, bg);
        status(s, "NOW TAP THE DOT");
        Serial.println("CYD32,TOUCH,CHECK_DOT");
        const int dx = (int)(0.62f * w), dy = (int)(0.32f * h);
        int16_t ra, rb;
        if (!holdSample(s, readRaw, dx, dy, true, 200, ra, rb)) {
            // Five good targets already passed; walking away from the check
            // is no reason to throw them out.
            out = fit;
            t.fillRect(0, 0, w, h, bg);
            return Outcome::SAVED;
        }
        float gx, gy;
        TouchFit::toScreen(fit, ra, rb, rot, gx, gy);
        const float err = hypotf(gx - dx, gy - dy);
        const bool good = err <= verifyTol;
        Serial.printf("[touchcal] check tap off by %.1f px (limit %.1f)\n", err, verifyTol);
        t.fillCircle(dx, dy, 6, s.accent);
        const uint16_t mark = good ? GREEN : RED;
        t.drawLine((int)gx - 6, (int)gy - 6, (int)gx + 6, (int)gy + 6, mark);
        t.drawLine((int)gx - 6, (int)gy + 6, (int)gx + 6, (int)gy - 6, mark);
        delay(700);
        if (good) {
            notice(s, GREEN, "TOUCH SET", "Works in every rotation.", nullptr, 1100);
            t.fillRect(0, 0, w, h, bg);
            out = fit;
            return Outcome::SAVED;
        }
        if (attempt == 0) notice(s, AMBER, "ONCE MORE", "That tap missed the dot.", nullptr, 1600);
    }

    notice(s, RED, "NOT SAVED", "Touch is unchanged.", "Try again from SETTINGS.", 2200);
    t.fillRect(0, 0, w, h, bg);
    return Outcome::FAILED;
}

}  // namespace SqeTouchWizard
