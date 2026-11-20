// Exact source port of SquachWatch-CYD/src/theme.cpp synthwave renderer.
// Source upstream SHA 0b4ee801e295d924a15934d2f2324f1475cfd482.
// Original rendering mathematics, animation, geometry, RGB565 colors and
// random initialization preserved. The DrawBand clipping adapter uses the
// entire screen band (SquachEmit renders a whole frame, not sliced bands).
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#if defined(SQE_CYD32)
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <math.h>
namespace SquachWatchScenes {
constexpr uint16_t BG = 0x0801;
constexpr uint16_t CYAN = 0x07FF;
static inline uint16_t blend(uint16_t a, uint16_t b, uint16_t t) {
    // 8.8 fixed-point t, 0..256
    uint8_t ar = (a >> 8) & 0xF8;
    uint8_t ag = (a >> 3) & 0xFC;
    uint8_t ab = (a << 3) & 0xF8;
    uint8_t br = (b >> 8) & 0xF8;
    uint8_t bg = (b >> 3) & 0xFC;
    uint8_t bb = (b << 3) & 0xF8;
    uint8_t rr = (uint8_t)(((uint16_t)ar * (256 - t) + (uint16_t)br * t) >> 8) & 0xF8;
    uint8_t rg = (uint8_t)(((uint16_t)ag * (256 - t) + (uint16_t)bg * t) >> 8) & 0xFC;
    uint8_t rb = (uint8_t)(((uint16_t)ab * (256 - t) + (uint16_t)bb * t) >> 8) & 0xF8;
    return (uint16_t)((rr << 8) | (rg << 3) | (rb >> 3));
}

static inline uint16_t sunsetSkyColorAt(TFT_eSPI& t, int y, int yTop, int yHoriz) {
    float tt = (float)(y - yTop) / (float)(yHoriz - yTop);
    if (tt < 0.0f) tt = 0.0f;
    if (tt > 1.0f) tt = 1.0f;
    uint8_t r, g, b;
    if (tt < 0.5f) {
        float bl = tt * 2.0f;
        r = (uint8_t)(15 + bl * 150); g = 0; b = (uint8_t)(55 - bl * 20);
    } else {
        float bl = (tt - 0.5f) * 2.0f;
        r = (uint8_t)(165 + bl * 90); g = (uint8_t)(bl * 75); b = (uint8_t)(35 - bl * 15);
    }
    return t.color565(r, g, b);
}

static inline void drawSunsetSun(TFT_eSPI& t, int cx, int cy, int r, int yTop, int yHoriz) {
    t.fillCircle(cx, cy, r,      t.color565(255,  50,  80));
    t.fillCircle(cx, cy, r -  6, t.color565(255, 100,  40));
    t.fillCircle(cx, cy, r - 13, t.color565(255, 165,  20));
    t.fillCircle(cx, cy, r - 20, t.color565(255, 215,  70));
    t.fillCircle(cx, cy, r - 27 > 0 ? r - 27 : 1, t.color565(255, 240, 150));

    // Horizon cutout stripes, widening toward the bottom, each painted
    // with the sky color at that exact row so the sun blends into the
    // gradient behind it instead of showing a flat erased slit.
    for (int stripe = 1; stripe <= 9; stripe++) {
        int sy2 = cy + stripe * 4;
        if (sy2 >= cy + r) break;
        int delta = sy2 - cy;
        if (delta <= 0 || delta >= r) continue;
        int half = (int)sqrtf((float)(r * r - delta * delta));
        t.fillRect(cx - half, sy2, half * 2, 2, sunsetSkyColorAt(t, sy2, yTop, yHoriz));
    }
    t.drawCircle(cx, cy, r, t.color565(255, 235, 235));
}

static inline uint16_t sunBandColor(TFT_eSPI& t, float rr) {
    if (rr > 1.0f) rr = 1.0f;
    if (rr > 0.88f) return t.color565(255,  50,  80);
    if (rr > 0.72f) return t.color565(255, 100,  40);
    if (rr > 0.54f) return t.color565(255, 165,  20);
    if (rr > 0.36f) return t.color565(255, 215,  70);
    return                 t.color565(255, 240, 150);
}

static inline void drawSynthwave(TFT_eSPI& t, uint32_t now, int yTop, int yBottom,
                   float horizonFrac) {
    const int w = t.width();
    const int band = yBottom - yTop;
    if (band < 48 || w < 32) return;

    if (horizonFrac < 0.15f) horizonFrac = 0.15f;
    if (horizonFrac > 0.85f) horizonFrac = 0.85f;
    const int yHoriz = yTop + (int)(band * horizonFrac);
    const int skyH   = yHoriz - yTop;
    const int seaH   = yBottom - yHoriz;
    if (skyH < 8 || seaH < 8) return;

    // ---- sky -------------------------------------------------------
    for (int y = (yTop); y < (yHoriz); y++) {
        t.drawFastHLine(0, y, w, sunsetSkyColorAt(t, y, yTop, yHoriz));
    }

    // Stars spread across the real panel width rather than a fixed 240
    // -- the boot splash star field predates this scene being used on a
    // 320px-wide rotation, and bunches to the left there. Re-seeded if
    // the band changes shape, i.e. on rotate.
    static const uint8_t NSTAR = 18;
    static uint16_t stx[NSTAR];
    static uint8_t  sty[NSTAR], stph[NSTAR];
    static bool     starsInited = false;
    static int      starW = 0, starH = 0;
    if (!starsInited || starW != w || starH != skyH) {
        for (uint8_t i = 0; i < NSTAR; i++) {
            stx[i]  = (uint16_t)random(2, w - 2);
            sty[i]  = (uint8_t)random(0, skyH * 3 / 4);
            stph[i] = (uint8_t)random(0, 256);
        }
        starsInited = true; starW = w; starH = skyH;
    }
    for (uint8_t i = 0; i < NSTAR; i++) {
        uint32_t tw = (now / 10 + (uint32_t)stph[i] * 22) % 300;
        if (tw > 220) continue;
        uint8_t bri = (tw < 100) ? 255 : (uint8_t)(255 - (tw - 100) * 3);
        t.drawPixel(stx[i], yTop + sty[i], t.color565(bri, bri, (uint8_t)(bri * 0.88f)));
    }

    // ---- sun -------------------------------------------------------
    const int sunR  = (skyH * 4) / 5;
    const int sunCx = w / 2;
    // Sunk further as it grew: at this radius a third of it above the
    // horizon put the top edge off the band entirely.
    const int sunCy = yHoriz - (sunR / 4);
    drawSunsetSun(t, sunCx, sunCy, sunR, yTop, yHoriz);

    // ---- ridgeline -------------------------------------------------
    // Two layers, paler and taller behind, darker in front, so the
    // horizon has depth instead of being a bare line. Fixed profiles
    // generated once: a ridgeline that reshuffled every frame would
    // read as noise rather than landscape.
    static const uint8_t NPEAK = 9;
    static uint8_t farPk[NPEAK], nearPk[NPEAK];
    static bool ridgeInited = false;
    if (!ridgeInited) {
        for (uint8_t i = 0; i < NPEAK; i++) {
            farPk[i]  = (uint8_t)random(30, 100);
            nearPk[i] = (uint8_t)random(14,  58);
        }
        ridgeInited = true;
    }
    const int farMax  = skyH / 4;
    const int nearMax = skyH / 6;
    for (uint8_t layer = 0; layer < 2; layer++) {
        const uint8_t* pk = layer ? nearPk : farPk;
        int      maxH = layer ? nearMax : farMax;
        uint16_t c    = layer ? t.color565(28, 4, 38) : t.color565(60, 12, 68);
        int step = w / (NPEAK - 1);
        if (step < 1) step = 1;
        for (uint8_t i = 0; i + 1 < NPEAK; i++) {
            int x0 = i * step, x1 = (i + 1) * step;
            int h0 = (pk[i]     * maxH) / 100;
            int h1 = (pk[i + 1] * maxH) / 100;
            for (int x = x0; x <= x1 && x < w; x++) {
                float f = (x1 > x0) ? (float)(x - x0) / (float)(x1 - x0) : 0.0f;
                int hh = h0 + (int)((h1 - h0) * f);
                if (hh > 0) t.drawFastVLine(x, yHoriz - hh, hh, c);
            }
        }
    }

    // ---- birds -----------------------------------------------------
    // Drawn after the ridgeline so nothing occludes them: they are the
    // nearest thing in the scene. Position is derived straight from
    // `now` rather than integrated frame to frame -- no accumulated
    // state means no drift, and no dt to clamp when the frame rate
    // moves around.
    //
    // The silhouette colour is deliberately close to the darkest part of
    // the sky. Crossing the sun they read as hard cut-outs, which is the
    // shot; out over open sky they nearly vanish, which is both cheaper
    // to look at and roughly what a distant bird actually does.
    {
        static const uint8_t NBIRD = 5;
        static uint8_t birdY[NBIRD], birdSz[NBIRD];
        static bool birdsInited = false;
        static int  birdW = 0, birdH = 0;
        if (!birdsInited || birdW != w || birdH != skyH) {
            for (uint8_t i = 0; i < NBIRD; i++) {
                // Upper two thirds of the sky: low enough to cross the
                // sun, high enough to clear the ridgeline.
                birdY[i]  = (uint8_t)random(2, (skyH * 2) / 3);
                birdSz[i] = (uint8_t)random(3, 6);
            }
            birdsInited = true; birdW = w; birdH = skyH;
        }
        const uint16_t birdCol = t.color565(26, 6, 34);
        const float    span    = (float)(w + 28);
        for (uint8_t i = 0; i < NBIRD; i++) {
            const float spd = 0.009f + (float)(i % 3) * 0.005f;   // px per ms
            const float fx  = fmodf((float)now * spd + (float)i * 63.0f, span) - 14.0f;
            const int   x   = (int)fx;
            const int   y   = yTop + birdY[i];
            if (y < yTop + 1 || y >= yHoriz - 1) continue;
            const int   sz  = birdSz[i];
            // Wing beat. Each bird carries its own phase so the flock
            // does not pulse in unison, which reads as one object.
            const float flap = sinf((float)now / (120.0f + i * 17.0f) + (float)i * 1.7f);
            const int   dy   = (int)(flap * (float)sz * 0.7f);
            if (x - sz < 0 || x + sz >= w) continue;
            // Two shallow strokes per wing give the gull kink; a single
            // straight V reads as a chevron, not a bird.
            t.drawLine(x - sz,     y - dy,     x - sz / 2, y,          birdCol);
            t.drawLine(x - sz / 2, y,          x,          y - dy / 2, birdCol);
            t.drawLine(x + sz,     y - dy,     x + sz / 2, y,          birdCol);
            t.drawLine(x + sz / 2, y,          x,          y - dy / 2, birdCol);
        }
    }

    // ---- horizon bloom ---------------------------------------------
    for (int dy = -2; dy <= 1; dy++) {
        int y = yHoriz + dy;
        if (y < yTop || y >= yBottom) continue;
        uint8_t a = (dy == -1 || dy == 0) ? 235 : 120;
        t.drawFastHLine(0, y, w, blend(t.color565(90, 10, 60), t.color565(255, 150, 190), a));
    }

    // ---- water -----------------------------------------------------
    // Sky and sun, mirrored and foreshortened, in a single pass per row:
    // the surface colour first, then the sun's reflection blended over
    // it where the mirrored row crosses the sun's circle.
    //
    // The sky reflection is not displaced horizontally, because it is a
    // flat horizontal gradient -- sliding it sideways would cost time
    // and show nothing. What reads as a moving surface is a travelling
    // *brightness* ripple, one sinf per row. The sun's reflection does
    // get displaced, because there the shape is visible.
    const uint16_t waterBase = t.color565(10, 0, 30);
    for (int y = (yHoriz); y < (yBottom); y++) {
        const float d = (float)(y - yHoriz) / (float)seaH;   // 0 horizon, 1 viewer
        const int srcY = yHoriz - (int)(d * skyH * 0.82f);

        // Two ripple rates so the surface never pulses uniformly.
        float rip = sinf(d * 34.0f - (float)now / 210.0f)
                  + 0.5f * sinf(d * 61.0f + (float)now / 130.0f);
        int dim = (int)(196.0f - d * 118.0f + rip * 17.0f);
        if (dim < 24)  dim = 24;
        if (dim > 255) dim = 255;
        const uint16_t waterC =
            blend(waterBase, sunsetSkyColorAt(t, srcY, yTop, yHoriz), (uint8_t)dim);
        t.drawFastHLine(0, y, w, waterC);

        // Sun reflection: same circle equation as the real sun, taken on
        // the mirrored row.
        const int delta = srcY - sunCy;
        if (delta <= -sunR || delta >= sunR) continue;
        const int half = (int)sqrtf((float)(sunR * sunR - delta * delta));
        if (half <= 0) continue;

        // Two incommensurate rates, not one. A single sine gives gaps at
        // a perfectly fixed pitch, which the eye reads as banding --
        // regular stripes rather than water. Summing a second, unrelated
        // frequency (and a different time rate) means the pattern never
        // repeats over the surface. They also open up toward the viewer,
        // where the water is choppier.
        const float gapPhase = sinf(d * 62.0f - (float)now / 190.0f)
                             + 0.55f * sinf(d * 23.0f + (float)now / 310.0f);
        if (gapPhase > 0.05f - d * 0.40f) continue;

        const float ph  = (float)now / 260.0f;
        const float wob = sinf(d * 11.0f + ph)        * (2.0f + d * 11.0f)
                        + sinf(d * 27.0f - ph * 1.7f) * (1.0f + d *  4.5f);

        const float rr = (float)(delta < 0 ? -delta : delta) / (float)sunR;
        // Blended over the water colour for this row, not over a flat
        // dark: reflected light sits *in* the surface. Blending the
        // sun's yellows against near-black was turning them olive.
        const uint8_t a = (uint8_t)(228 - d * 128);
        const uint16_t c = blend(waterC, sunBandColor(t, rr), a);

        int x0 = sunCx - half + (int)wob;
        int xs = x0 < 0 ? 0 : x0;
        int xe = x0 + half * 2; if (xe > w) xe = w;
        if (xe > xs) t.drawFastHLine(xs, y, xe - xs, c);
    }

    // ---- grid ------------------------------------------------------
    // Rungs only. The converging lines to the vanishing point were
    // fighting the reflection: they cut across the sun's bands and
    // reasserted a hard flat plane exactly where the water was doing
    // the work of looking like a surface. The scrolling rungs alone
    // still give the floor motion and depth.
    const uint16_t gridCol = blend(BG, CYAN, 70);
    const uint32_t period = 1100;
    float basePhase = (float)(now % period) / (float)period;
    for (int i = 0; i < 7; i++) {
        float tt = fmodf(basePhase + (float)i / 7.0f, 1.0f);
        int y = yHoriz + (int)(tt * tt * seaH);
        if (y <= yHoriz || y >= yBottom) continue;
        uint8_t fade = (uint8_t)(70 + tt * 185);   // brighter as it nears the viewer
        t.drawFastHLine(0, y, w, blend(BG, gridCol, fade));
    }
}
} // namespace SquachWatchScenes
#endif
