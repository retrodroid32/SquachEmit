// Source-identical SquachWatch-CYD firefly meadow rendering, with a
// whole-screen band adapter (no partial DrawBand slicing).
// Upstream src/theme.cpp blob 0b4ee801e295d924a15934d2f2324f1475cfd482.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#if defined(SQE_CYD32)
#include "squachwatch_synthwave_exact.h"
namespace SquachWatchScenes {
static int s_bgTextTop = -1;
constexpr uint16_t WHITE = 0xFFFF;
static const uint8_t FF_FAR = 20, FF_MID = 14, FF_NEAR = 4;
// Each firefly has a home point it wanders around and a flight pattern
// that says how. They used to share one behaviour -- a drift and a
// bounce -- and forty of the same thing is a particle system, not a
// field of insects. Six patterns, assigned by hash, each with its own
// pulse period, is what makes them read as individuals.
struct Fly {
    float   x, y;         // where it is drawn
    float   hx, hy;       // the home point the pattern orbits
    float   vx;           // drift of the home point
    float   dx, dy;       // the darter's current dash
    float   ph, pk;       // phase, and a per-fly pulse period multiplier
    uint8_t tone, pat;
    bool    rolled;
};
static Fly      s_ffFar[FF_FAR], s_ffMid[FF_MID], s_ffNear[FF_NEAR];
static bool     s_ffInit = false;
static uint32_t s_ffLastMs = 0;
// Events, each on its own clock. 0 in a *Start means not running.
static uint32_t s_ffWaveAt = 0,  s_ffWaveStart = 0;   // the sync ripple
static uint32_t s_ffGustAt = 0,  s_ffGustStart = 0;   // wind through the grass
static uint32_t s_ffEyesAt = 0,  s_ffEyesStart = 0;   // something in the treeline
static uint32_t s_ffLantAt = 0,  s_ffLantStart = 0;   // one passes close to the lens
static int      s_ffEyesX = 0;
static float    s_ffLantY = 0.0f;

static void ffSpawn(Fly& f, int w, int y0, int y1, float spd) {
    f.x  = f.hx = (float)random(0, w);
    f.y  = f.hy = (float)random(y0, y1);
    f.vx = (float)random(-100, 101) / 100.0f * spd;
    f.dx = 0.0f; f.dy = 0.0f;
    f.ph = (float)random(0, 6283) / 1000.0f;
    f.pk = 0.75f + (float)random(0, 66) / 100.0f;
    f.tone = (uint8_t)random(0, 3);
    f.pat  = (uint8_t)random(0, 6);
    f.rolled = false;
}
// The six patterns. All of them move the HOME point slowly and put the
// drawn position somewhere relative to it, so the wander is bounded
// and the wrap is clean.
//   0 hover    figure-eight around a creeping home
//   1 cruiser  steady drift with a slow rise and fall
//   2 bobber   mostly vertical, nine pixels of bob
//   3 spiral   circles a centre that slides along -- a loose helix
//   4 darter   holds still, dashes, holds again
//   5 J-flash  the real Photinus signature: rises in a hook while lit
static void ffStep(Fly& f, int w, int y0, int y1, float ds, float wind, uint32_t now) {
    const float t = (float)now / 1000.0f;
    switch (f.pat) {
        case 0:
            f.hx += f.vx * 0.25f * ds;
            f.x = f.hx + sinf(t * 0.9f + f.ph) * 8.0f;
            f.y = f.hy + sinf(t * 1.8f + f.ph) * 4.0f;
            break;
        case 1:
            f.hx += f.vx * 1.2f * ds;
            f.x = f.hx;
            f.y = f.hy + sinf(t * 0.7f + f.ph) * 5.0f;
            break;
        case 2:
            f.hx += f.vx * 0.3f * ds;
            f.x = f.hx;
            f.y = f.hy + sinf(t * 2.2f + f.ph) * 9.0f;
            break;
        case 3:
            f.hx += f.vx * 0.5f * ds;
            f.x = f.hx + cosf(t * 1.6f + f.ph) * 10.0f;
            f.y = f.hy + sinf(t * 1.6f + f.ph) * 6.0f;
            break;
        case 4: {
            const float c = fmodf(t * 0.7f + f.ph, 1.0f);
            if (c < 0.22f) { f.hx += f.dx * ds * 2.2f; f.hy += f.dy * ds * 2.2f; }
            else if (c > 0.97f && !f.rolled) {
                f.dx = (float)random(-50, 51) / 60.0f;
                f.dy = (float)random(-50, 51) / 90.0f;
                f.rolled = true;
            }
            if (c < 0.9f) f.rolled = false;
            f.x = f.hx; f.y = f.hy;
            break;
        }
        default: {
            f.hx += f.vx * 0.5f * ds;
            const float pu = 0.5f + 0.5f * sinf((float)now / (700.0f * f.pk) + f.ph * 3.0f);
            f.x = f.hx + pu * 3.0f;
            f.y = f.hy - pu * 10.0f;
            break;
        }
    }
    // The wind nudges them at a quarter of what it did. At full strength
    // a gust swept every firefly off the screen, which is not what wind
    // does to an insect that weighs nothing and is flying on purpose --
    // the grass shows the wind; the fireflies mostly ignore it.
    f.hx += wind * 0.25f * ds;
    if (f.hx < -4.0f)           f.hx = (float)w + 3.0f;
    if (f.hx > (float)w + 4.0f) f.hx = -3.0f;
    if (f.hy < (float)y0 + 4.0f) f.hy = (float)y0 + 4.0f;
    if (f.hy > (float)y1 - 4.0f) f.hy = (float)y1 - 4.0f;
}
static inline uint16_t ffTone(TFT_eSPI& t, uint8_t k) {
    return (k == 0) ? t.color565(219, 255, 73)
         : (k == 1) ? t.color565(255, 219, 36)
                    : t.color565(146, 255, 109);
}
static inline float ffPulse(const Fly& f, uint32_t now, float per) {
    // per is the depth's base period; pk spreads it per fly so no two
    // in the same plane ever blink in step for long.
    return 0.5f + 0.5f * sinf((float)now / (per * f.pk) + f.ph * 3.0f);
}
// The bloom. Two spans per row -- a wide dim one and a narrow bright one
// over it -- with the falloff dithered per row so the edge is a fade and
// not a ring. Radial, not just vertical: the outer span narrows with the
// circle and the inner one is half of it.
static void ffGlow(TFT_eSPI& t, int x, int y, int r, uint16_t col, uint16_t bg,
                   int str, int y0, int y1) {
    static const int8_t GD[4] = { -7, 3, 7, -3 };
    for (int dy = -r; dy <= r; dy++) {
        const int yy = y + dy;
        if (yy < y0 || yy >= y1) continue;
        const int half = (int)sqrtf((float)(r * r - dy * dy));
        if (half < 1) continue;
        const float fr = 1.0f - fabsf((float)dy) / (float)(r + 1);
        int ao = (int)(fr * fr * (float)str * 0.42f) + GD[yy & 3];
        int ai = (int)(fr * fr * (float)str)         + GD[yy & 3];
        if (ao > 255) ao = 255;
        if (ai > 255) ai = 255;
        if (ao > 3) t.drawFastHLine(x - half, yy, half * 2 + 1, blend(bg, col, (uint16_t)ao));
        const int ih = half / 2;
        if (ai > 3) t.drawFastHLine(x - ih, yy, ih * 2 + 1, blend(bg, col, (uint16_t)ai));
    }
}

static inline void drawFireflies(TFT_eSPI& t, uint32_t now, int yStart, int yEnd) {
    const int w = t.width();
    // Same floor rule as SNOWFALL, and for the same reason: stop a pixel
    // short of the counters, which are plain text with no outline. Also
    // s_bgTextTop rather than s_bgFloor for the same reason -- the soil band
    // is the thing being stopped, and nothing in this scene stands on it.
    const int yBot  = ((s_bgTextTop > yStart + 40 && s_bgTextTop <= yEnd) ? s_bgTextTop : yEnd) - 1;
    const int bandH = yBot - yStart;
    if (bandH < 40) return;
    const int horizon  = yStart + (bandH * 58) / 100;   // treeline stands here
    const int grassTop = yStart + (bandH * 78) / 100;   // near grass starts here

    if (!s_ffInit) {
        for (uint8_t i = 0; i < FF_FAR;  i++) ffSpawn(s_ffFar[i],  w, horizon + 2, grassTop - 2, 0.10f);
        for (uint8_t i = 0; i < FF_MID;  i++) ffSpawn(s_ffMid[i],  w, horizon - 14, yBot - 8, 0.16f);
        for (uint8_t i = 0; i < FF_NEAR; i++) ffSpawn(s_ffNear[i], w, horizon + 4, yBot - 6, 0.22f);
        s_ffLastMs = now;
        s_ffWaveAt = now + (uint32_t)random(9000, 16000);
        s_ffGustAt = now + (uint32_t)random(18000, 30000);
        s_ffEyesAt = now + (uint32_t)random(14000, 26000);
        s_ffLantAt = now + (uint32_t)random(20000, 34000);
        s_ffInit = true;
    }
    uint32_t dt = now - s_ffLastMs;
    if (dt > 200u) dt = 200u;
    const bool  step = (now != s_ffLastMs);
    if (step) s_ffLastMs = now;
    const float ds = (float)dt / 16.0f;

    // ---- the slow things ----------------------------------------------
    // Dusk. 1 is the warm horizon just after sunset, 0 is full night;
    // it swings between them over about two and a half minutes, and the
    // stars come out as it goes.
    const float dusk = 0.5f + 0.5f * cosf((float)now / 26000.0f);
    // Wind: a small ambient breeze, and a gust every twenty-odd seconds
    // that ramps up over a second and a half and dies away again.
    float wind = sinf((float)now / 3000.0f) * 0.12f;
    if (step) {
        if (!s_ffGustStart && now >= s_ffGustAt) s_ffGustStart = now;
        if (!s_ffWaveStart && now >= s_ffWaveAt) s_ffWaveStart = now;
        if (!s_ffEyesStart && now >= s_ffEyesAt) { s_ffEyesStart = now; s_ffEyesX = random(24, w - 24); }
        if (!s_ffLantStart && now >= s_ffLantAt) { s_ffLantStart = now; s_ffLantY = (float)random(grassTop - 10, yBot - 14); }
    }
    if (s_ffGustStart) {
        const float gk = (float)(now - s_ffGustStart) / 3500.0f;
        if (gk >= 1.0f) { s_ffGustStart = 0; s_ffGustAt = now + (uint32_t)random(18000, 34000); }
        else wind += sinf(gk * 3.14159f) * 1.0f;
    }
    float waveT = -1.0f;
    if (s_ffWaveStart) {
        waveT = (float)(now - s_ffWaveStart) / 2400.0f;
        if (waveT > 1.15f) { s_ffWaveStart = 0; s_ffWaveAt = now + (uint32_t)random(11000, 20000); waveT = -1.0f; }
    }

    if (step) {
        for (uint8_t i = 0; i < FF_FAR;  i++) ffStep(s_ffFar[i],  w, horizon + 2,  grassTop - 2, ds * 0.5f, wind * 0.4f, now);
        for (uint8_t i = 0; i < FF_MID;  i++) ffStep(s_ffMid[i],  w, horizon - 14, yBot - 8,     ds,        wind,        now);
        for (uint8_t i = 0; i < FF_NEAR; i++) ffStep(s_ffNear[i], w, horizon + 4,  yBot - 6,     ds * 1.5f, wind * 1.4f, now);
    }

    // ---- sky ------------------------------------------------------------
    // Blue kept over 42 at both ends so it lands on 85 rather than 0 --
    // the same trap the SNOWFALL sky fell into first. The horizon swings
    // from a warm dusk purple to a cooler night blue with `dusk`.
    const uint16_t skyTop = t.color565(8, 10, 96);
    const uint16_t skyHor = blend(t.color565(40, 48, 140), t.color565(128, 44, 96),
                                  (uint16_t)(dusk * 255.0f));
    static const int8_t SKY_DITH[4] = { -9, 4, 9, -4 };
    const int skyH = horizon - yStart;
    for (int y = (yStart); y < (horizon); y++) {
        int k = ((y - yStart) * 255) / (skyH > 1 ? skyH - 1 : 1) + SKY_DITH[(y - yStart) & 3];
        if (k < 0) k = 0;
        if (k > 255) k = 255;
        t.drawFastHLine(0, y, w, blend(skyTop, skyHor, (uint16_t)k));
    }
    auto skyAt = [&](int y) -> uint16_t {
        int k = ((y - yStart) * 255) / (skyH > 1 ? skyH - 1 : 1);
        if (k < 0) k = 0;
        if (k > 255) k = 255;
        return blend(skyTop, skyHor, (uint16_t)k);
    };

    // Stars come out as the dusk fades. Twinkling on individual periods.
    const float starVis = (1.0f - dusk) * (1.0f - dusk);
    if (starVis > 0.04f) {
        for (uint8_t i = 0; i < 26; i++) {
            uint32_t h = (uint32_t)(i + 5) * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
            const int sx = (int)(h % (uint32_t)w);
            const int sy = yStart + 2 + (int)((h >> 11) % (uint32_t)(skyH > 24 ? skyH - 20 : 4));
            const float tw = 0.5f + 0.5f * sinf((float)now / (700.0f + (float)(h % 800u)) + (float)i);
            t.drawPixel(sx, sy, blend(skyAt(sy), WHITE, (uint16_t)(starVis * (70.0f + tw * 150.0f))));
        }
    }

    // The moon, low and to the right, with a halo. The halo is per-row
    // -- sampled against the sky AT THAT ROW -- which is the thing the
    // SNOWFALL moon never got because its three attempts all sampled the
    // sky once. And it pushes red and green rather than blue, so the
    // falloff has eight steps to work with instead of four.
    {
        const int mx = (w * 79) / 100, my = yStart + (skyH * 22) / 100;
        const int hr = 13;
        for (int dy = -hr; dy <= hr; dy++) {
            const int yy = my + dy;
            if (yy < yStart || yy >= horizon) continue;
            const int half = (int)sqrtf((float)(hr * hr - dy * dy));
            const float fr = 1.0f - fabsf((float)dy) / (float)(hr + 1);
            int a = (int)(fr * fr * 70.0f) + SKY_DITH[yy & 3];
            if (a <= 2) continue;
            const uint16_t sk = skyAt(yy);
            const uint16_t warm = blend(sk, t.color565(255, 240, 200), (uint16_t)a);
            t.drawFastHLine(mx - half, yy, half * 2 + 1, warm);
        }
        t.fillCircle(mx, my, 5, t.color565(236, 236, 210));
        t.fillCircle(mx - 2, my - 1, 2, t.color565(200, 200, 176));
        t.drawCircle(mx, my, 5, t.color565(219, 219, 182));
    }

    // ---- treeline -----------------------------------------------------
    // Ragged black silhouette: a base row of wide blocks and a spike
    // above every other one. Pure silhouette -- no detail survives this
    // dark anyway, and the uneven top edge is all that reads.
    {
        const uint16_t tree = t.color565(3, 8, 10);
        for (int x = 0; x < w; x += 8) {
            uint32_t h = (uint32_t)(x / 8 + 40) * 2654435761u; h ^= h >> 13;
            const int hh = 6 + (int)(h % 14u);
            t.fillRect(x, horizon - hh, 8, hh + 2, tree);
            if ((h & 2u) != 0u) t.fillRect(x + 2 + (int)(h % 3u), horizon - hh - 5, 3, 6, tree);
        }
    }

    // Something in the trees. Two green eyes, for three seconds, with
    // one blink. Nothing else about it is ever shown.
    if (s_ffEyesStart) {
        const uint32_t ea = now - s_ffEyesStart;
        if (ea > 3200u) { s_ffEyesStart = 0; s_ffEyesAt = now + (uint32_t)random(16000, 30000); }
        else if (!(ea > 1700u && ea < 1850u)) {
            const uint16_t eye = t.color565(146, 255, 109);
            t.fillRect(s_ffEyesX - 4, horizon - 9, 2, 2, eye);
            t.fillRect(s_ffEyesX + 2, horizon - 9, 2, 2, eye);
        }
    }

    // ---- the far field, and the mist on it ----------------------------
    const uint16_t field = t.color565(8, 40, 12);
    t.fillRect(0, horizon, w, grassTop - horizon, field);
    // Two fog bands drifting at different speeds, thicker low down. A
    // firefly inside one stops being a point and becomes a halo.
    const uint16_t fog = t.color565(100, 104, 120);
    auto fogAt = [&](int y) -> float {
        float f = 0.0f;
        for (uint8_t b = 0; b < 2; b++) {
            const float cy = (float)grassTop - 9.0f - (float)b * 9.0f
                           + sinf((float)now / (2600.0f + (float)b * 900.0f)) * 3.0f;
            const float th = 6.0f + (float)b * 2.0f;
            const float d = fabsf((float)y - cy) / th;
            if (d < 1.0f) f += (1.0f - d) * (0.55f - (float)b * 0.2f);
        }
        return f > 1.0f ? 1.0f : f;
    };
    for (int y = (horizon + 2); y < (grassTop); y++) {
        const float f = fogAt(y);
        if (f < 0.04f) continue;
        int a = (int)(f * 120.0f) + SKY_DITH[y & 3] / 2;
        if (a < 2) continue;
        t.drawFastHLine(0, y, w, blend(field, fog, (uint16_t)a));
    }

    // Far fireflies: single pixels over the far field. In the fog they
    // bloom into small halos instead.
    for (uint8_t i = 0; i < FF_FAR; i++) {
        const Fly& f = s_ffFar[i];
        const float pu = ffPulse(f, now, 1000.0f);
        if (pu < 0.35f) continue;
        const int x = (int)f.x, y = (int)f.y;
        const float fg = fogAt(y);
        if (fg > 0.3f) ffGlow(t, x, y, 3, ffTone(t, f.tone), blend(field, fog, (uint16_t)(fg * 120.0f)),
                              (int)(pu * 150.0f), horizon, grassTop);
        else t.drawPixel(x, y, blend(field, ffTone(t, f.tone), (uint16_t)(pu * 230.0f)));
    }

    // ---- the near grass ------------------------------------------------
    const uint16_t soil  = t.color565(6, 30, 10);
    const uint16_t blade = t.color565(12, 64, 20);
    t.fillRect(0, grassTop, w, yBot - grassTop, soil);

    // Where the near light is, for the grass to read. The lantern counts
    // as a fifth, brighter source while it is passing.
    struct Lamp { int x, y; float s; uint8_t tone; };
    Lamp lamps[FF_NEAR + 1];
    uint8_t nl = 0;
    for (uint8_t i = 0; i < FF_NEAR; i++) {
        const float pu = ffPulse(s_ffNear[i], now, 650.0f);
        if (pu > 0.4f) lamps[nl++] = { (int)s_ffNear[i].x, (int)s_ffNear[i].y, pu, s_ffNear[i].tone };
    }
    float lantK = -1.0f;
    int   lantX = 0;
    if (s_ffLantStart) {
        lantK = (float)(now - s_ffLantStart) / 7000.0f;
        if (lantK >= 1.0f) { s_ffLantStart = 0; s_ffLantAt = now + (uint32_t)random(24000, 40000); lantK = -1.0f; }
        else {
            lantX = (int)(-20.0f + lantK * (float)(w + 40));
            lamps[nl++] = { lantX, (int)s_ffLantY, 1.3f, 1 };
        }
    }

    // Blades, one every four pixels, hashed heights. A gust leans them.
    // Each blade takes its colour from the nearest bright near firefly:
    // that tint, falling off with distance, is the light landing on the
    // grass, and it costs nothing because the blades were being drawn
    // anyway.
    for (int x = 0; x < w; x += 4) {
        uint32_t h = (uint32_t)(x / 4 + 9) * 2654435761u; h ^= h >> 13;
        const int bh = 3 + (int)(h % 7u);
        uint16_t c = blade;
        float best = 0.0f; uint8_t bt = 0;
        for (uint8_t i = 0; i < nl; i++) {
            const float dx = (float)(x - lamps[i].x), dy = (float)(yBot - bh - lamps[i].y);
            const float d = sqrtf(dx * dx + dy * dy) / 30.0f;
            if (d < 1.0f) { const float v = (1.0f - d) * lamps[i].s; if (v > best) { best = v; bt = lamps[i].tone; } }
        }
        if (best > 0.02f) c = blend(blade, ffTone(t, bt), (uint16_t)(best * 190.0f));
        const int lean = (int)(wind * (3.0f + (float)(h % 4u)));
        if (lean) t.drawLine(x, yBot, x + lean, yBot - bh, c);
        else      t.drawFastVLine(x, yBot - bh, bh, c);
    }

    // ---- mid fireflies: a small cross, over the grass ----------------
    for (uint8_t i = 0; i < FF_MID; i++) {
        const Fly& f = s_ffMid[i];
        const float pu = ffPulse(f, now, 800.0f);
        if (pu < 0.3f) continue;
        const int x = (int)f.x, y = (int)f.y;
        const uint16_t bg = (y < horizon) ? skyAt(y) : ((y < grassTop) ? field : soil);
        const uint16_t c = blend(bg, ffTone(t, f.tone), (uint16_t)(pu * 255.0f));
        if (fabsf(wind) > 0.4f) t.drawFastHLine(x - (int)(wind * 10.0f), y, (int)fabsf(wind * 10.0f), blend(bg, c, 110));
        t.drawFastHLine(x - 1, y, 3, c);
        t.drawFastVLine(x, y - 1, 3, c);
    }

    // ---- near fireflies: the blooms -----------------------------------
    for (uint8_t i = 0; i < FF_NEAR; i++) {
        const Fly& f = s_ffNear[i];
        const float pu = ffPulse(f, now, 650.0f);
        if (pu < 0.2f) continue;
        const int x = (int)f.x, y = (int)f.y;
        const uint16_t bg = (y < grassTop) ? field : soil;
        ffGlow(t, x, y, 5, ffTone(t, f.tone), bg, (int)(pu * 255.0f), horizon, yBot);
        t.drawFastHLine(x - 1, y, 3, blend(WHITE, ffTone(t, f.tone), 90));
        t.drawFastVLine(x, y - 1, 3, blend(WHITE, ffTone(t, f.tone), 90));
    }

    // One passes close to the lens now and then: a big soft bloom with a
    // faint horizontal flare across the frame.
    if (lantK >= 0.0f) {
        const float ed = (lantK < 0.15f) ? lantK / 0.15f : ((lantK > 0.85f) ? (1.0f - lantK) / 0.15f : 1.0f);
        const int ly = (int)s_ffLantY;
        const uint16_t lc = ffTone(t, 1);
        const uint16_t bg = (ly < grassTop) ? field : soil;
        t.drawFastHLine(lantX - 44, ly, 88, blend(bg, lc, (uint16_t)(ed * 60.0f)));
        ffGlow(t, lantX, ly, 9, lc, bg, (int)(ed * 255.0f), horizon, yBot);
        t.fillRect(lantX - 1, ly - 1, 3, 3, blend(WHITE, lc, 60));
    }

    // ---- the sync ripple ------------------------------------------------
    // Every so often the whole field flashes together in a travelling
    // wave. It was the one good idea in the old version, so it stays:
    // anything within the band gets a hot white core for a moment.
    if (waveT >= 0.0f) {
        auto hit = [&](const Fly& f, int cross) {
            const float xn = f.x / (float)w;
            if (fabsf(xn - waveT) >= 0.07f) return;
            const int x = (int)f.x, y = (int)f.y;
            if (cross) { t.drawFastHLine(x - 2, y, 5, WHITE); t.drawFastVLine(x, y - 2, 5, WHITE); }
            else t.drawPixel(x, y, WHITE);
        };
        for (uint8_t i = 0; i < FF_FAR;  i++) hit(s_ffFar[i],  0);
        for (uint8_t i = 0; i < FF_MID;  i++) hit(s_ffMid[i],  1);
        for (uint8_t i = 0; i < FF_NEAR; i++) hit(s_ffNear[i], 1);
    }

    // Below the scene's own floor, the soil carries on to the bottom of the
    // band: the counters sit on dark plates of their own, and a black shelf
    // under the grass read as the scene running out.
    if (yEnd > yBot) t.fillRect(0, yBot, w, yEnd - yBot, t.color565(6, 30, 10));
}


} // namespace SquachWatchScenes
#endif
