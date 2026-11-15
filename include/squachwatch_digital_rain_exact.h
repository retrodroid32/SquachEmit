// SquachWatch-CYD exact Digital Rain renderer extracted from src/theme.cpp.
// Only DrawBand clipping is adapted to the SquachEmit full-band LCD.
// GPL-3.0-only — original project retains copyright.
#pragma once
#if defined(SQE_CYD32)
#include "squachwatch_synthwave_exact.h"
#include <cstdlib>
namespace SquachWatchScenes {
static constexpr int MAX_COLS=96;
static constexpr int MAXTRAIL=24;
static uint8_t (*s_rainBuf)[MAXTRAIL]=nullptr;
static bool s_rainInited=false;
static bool s_redGlyphOn=false;
static uint32_t s_redGlyphSaid=0;
static int s_redX=0,s_redY=0;
static uint32_t s_redAt=0;
static float s_animK=1.0f;
static constexpr uint16_t VAPOR_PURPLE=0xBB5F;
static void releaseDigitalRain(){
  if(s_rainBuf){free(s_rainBuf);s_rainBuf=nullptr;s_rainInited=false;}
}
static inline void drawDigitalRain(TFT_eSPI& t, uint32_t now, int yStart, int yEnd, bool advance) {
    // Dense columns with long, smoothly-decaying trails. Glyphs are plain
    // ASCII (the default GLCD font can't render UTF-8 katakana correctly)
    // from a dense symbol/letter/digit set. Column count adapts to width so
    // this works from 240px portrait through cyd35's 480px landscape.
    //
    // The rule here is ADD, never subtract. An earlier pass at this shortened
    // the trails and dimmed most columns, which bought 2.6ms of frame time
    // nobody had asked for and cost the thing the effect is actually for --
    // there was simply less rain. Trails are longer than the original now,
    // not shorter, and the cheap wins below (white-hot heads, per-drop
    // colour, shimmer, glow) all cost either nothing or O(cols).
    static const int  SPACING  = 5;
    static const char GLYPHS[] =
        "01" "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "!@#$%^&*<>{}[]/\\|+=~" "SASQUACH";
    static const int  GLN      = sizeof(GLYPHS) - 1;
    static const int  MINTRAIL = 17;      // averages ~21, just under the old flat 22
    // Ordered cool -> warm so that indexing them by depth gives distance:
    // violet sits at the back, pink at the front.
    static const uint16_t HUES[4] = { VAPOR_PURPLE, CYAN, GREEN, VAPOR_PINK };
    // One switch back to the old behaviour, where hue was rolled at random
    // per drop and had nothing to do with how far away it was.
    static const bool RAIN_DEPTH_HUE = true;

    int cols = t.width() / SPACING;
    if (cols > MAX_COLS) cols = MAX_COLS;
    if (cols < 1) cols = 1;

    static int16_t yPos[MAX_COLS];
    static uint8_t ySpeed[MAX_COLS];
    static float   yTick[MAX_COLS];      // frames' worth, in s_animK units
    // Per-DROP, re-rolled every time a column recycles. Hue used to be
    // HUES[i % 3], which made column 0 permanently pink, column 1
    // permanently cyan and so on -- a fixed stripe pattern across the whole
    // screen that never changed for the life of the boot, and the most
    // obviously machine-made thing about the effect.
    static uint8_t colHue[MAX_COLS];
    static uint8_t colLen[MAX_COLS];
    // Depth, 0 far .. 255 near, driving brightness and fall speed together.
    // Deliberately a NARROW range: pushing the far end down to 27% made half
    // the screen look washed out rather than distant.
    static uint8_t colDepth[MAX_COLS];
    // A rare drop that is longer, faster and burns white most of the way
    // down. Costs nothing extra -- it is a column that already exists.
    static bool    colSurge[MAX_COLS];
    if (!s_rainBuf) {
        s_rainBuf = (uint8_t(*)[MAXTRAIL])malloc(MAX_COLS * MAXTRAIL);
        // No room for it: a flat band, the same answer fire gives. The
        // screen stays legible and the next frame tries again.
        if (!s_rainBuf) { t.fillRect(0, yStart, t.width(), yEnd - yStart, BG); return; }
        s_rainInited = false;
    }
    uint8_t (*const charBuf)[MAXTRAIL] = s_rainBuf;
    // Wind, lightning and splashes. All three are O(cols) or O(1), and none
    // of them draws an extra glyph cell -- cells are the only thing on this
    // screen that costs real pixels, so motion is where the budget goes.
    // Glyphs that break loose, grow and fade. Drawn with a transparent
    // background so they read as rising OFF the rain rather than punching
    // holes in it -- at most five, so a handful of extra glyphs a frame.
    static const uint8_t NPOP = 5;
    static uint16_t popX[NPOP], popY[NPOP];
    static uint8_t  popAge[NPOP], popHue[NPOP], popCh[NPOP];
    static uint32_t boltAt = 0, boltNext = 0;
    // CRT roll bar and the VHS head-switch tear. Both are decided once per
    // frame and applied as a per-cell comparison, so neither draws a single
    // extra glyph.
    static uint32_t sweepAt = 0;
    static uint32_t tearAt = 0, tearNext = 0;
    static uint16_t splashX[6];
    static uint8_t  splashAge[6], splashHue[6];
    static bool     fxInit = false;
    static int     lastCols = -1;

    auto respawn = [&](int i, bool anywhere) {
        colSurge[i] = (random(0, 14) == 0);
        colDepth[i] = colSurge[i] ? 255 : (uint8_t)random(0, 256);
        // Far drops fall slower; a surge drops fastest of all. ySpeed is a
        // tick divider, so smaller is faster.
        ySpeed[i]   = colSurge[i] ? 2
                                  : (uint8_t)(5 - ((uint16_t)colDepth[i] * 3u) / 255u);
        if (ySpeed[i] < 1) ySpeed[i] = 1;
        // Hue follows depth rather than being rolled at random: far drops
        // run cool, near ones warm, so the field reads as having space in
        // it instead of four colours scattered arbitrarily. Set
        // RAIN_DEPTH_HUE to false for the old random roll.
        colHue[i]   = RAIN_DEPTH_HUE ? (uint8_t)(colDepth[i] >> 6)
                                     : (uint8_t)random(0, 4);
        colLen[i]   = colSurge[i] ? MAXTRAIL : (uint8_t)random(MINTRAIL, MAXTRAIL + 1);
        yTick[i]    = (uint8_t)random(0, ySpeed[i] ? ySpeed[i] : 1);
        yPos[i]     = anywhere ? (int16_t)random(yStart, yEnd)
                               : (int16_t)(yStart - random(0, 90));
    };

    if (!s_rainInited || cols != lastCols) {
        for (int i = 0; i < cols; i++) {
            respawn(i, true);
            for (int j = 0; j < MAXTRAIL; j++) charBuf[i][j] = (uint8_t)random(0, GLN);
        }
        s_rainInited = true;
        lastCols = cols;
    }

    if (!fxInit) {
        for (uint8_t k = 0; k < 6; k++) splashAge[k] = 0;
        for (uint8_t k = 0; k < NPOP; k++) popAge[k] = 0;
        boltNext = now + (uint32_t)random(14000, 32000);
        fxInit = true;
    }

    // LIGHTNING. Every 14-32s the field flares white for ~140ms and decays.
    // Costs one comparison: it scales an alpha that is already computed.
    if (advance && now >= boltNext) {
        boltAt   = now;
        boltNext = now + (uint32_t)random(14000, 32000);
    }
    const uint32_t boltAge = now - boltAt;
    const uint16_t boltAmt = (boltAt && boltAge < 140)
                             ? (uint16_t)(160u - (boltAge * 160u) / 140u) : 0;

    // CRT SWEEP. One pass down the field every ten seconds, then nothing --
    // it used to roll continuously, which at three seconds a pass made it
    // scenery rather than an event.
    //
    // What makes the bar legible is the blanking gap: the beam is off
    // during retrace, so a few rows go DARK immediately ahead of the bright
    // edge. Dark-then-hard-bright-then-short-decay is the whole effect. An
    // earlier attempt was a pure brightness lift with a long falloff, which
    // on a black field reads as a vague haze -- there is nothing for a
    // bright edge to be bright AGAINST.
    //
    // Driven off elapsed time rather than an accumulator, so the pass takes
    // the same wall-clock time regardless of frame rate, and parking it far
    // off screen between passes means every per-cell test below is simply
    // false for the other nine seconds.
    static const uint32_t SWEEP_PERIOD_MS = 10000;
    static const uint32_t SWEEP_TRAVEL_MS = 1200;
    if (advance && (now - sweepAt) >= SWEEP_PERIOD_MS) sweepAt = now;
    const uint32_t sweepAge = now - sweepAt;
    const int      travel   = (yEnd - yStart) + 80;
    const int rollAt = (sweepAt && sweepAge < SWEEP_TRAVEL_MS)
                       ? (yStart - 40 + (int)(((uint32_t)travel * sweepAge)
                                              / SWEEP_TRAVEL_MS))
                       : -30000;

    // VHS HEAD-SWITCH TEAR. On tape the bottom few lines are written by a
    // different head and never quite line up, so they sit torn sideways.
    // Here it fires as an occasional glitch rather than permanently.
    if (advance && now >= tearNext) {
        tearAt   = now;
        tearNext = now + 5000u + (uint32_t)random(0, 9000);
    }
    const bool tearOn = (tearAt != 0) && (now - tearAt < 260u);
    const int  tearY  = yEnd - 14;
    const int  tearDx = tearOn ? (int)random(-7, 8) : 0;

    // SQUACHY DISPLACES THE RAIN. Columns behind him are knocked back and
    // pushed aside, so he stands IN the field rather than on top of it.
    // Per column, not per cell -- his footprint is one comparison against
    // each column's x.
    int sqCx = 0, sqHalf = 0, sqTop = 0, sqBot = 0;
    // SquachEmit mascot uses the 112x111 sprite, anchored at center.
    // Adapt the upstream footprint callback to this real LCD layout.
    const bool sqHere=true;
    sqCx=t.width()/2;
    sqHalf=56;
    sqTop=yStart+(t.width()>=300?55:15);
    sqBot=sqTop+111;

    // The red glyph: which column carries it, when it started, when the next.
    // Picked from the columns whose heads have just come in at the top, the
    // slowest of them, and never one falling behind him.
    static int      redCol = -1;
    static uint32_t redFrom = 0, redNext = 0;
    const bool redHere = s_redGlyphOn && s_redGlyphSaid && (uint32_t)(millis() - s_redGlyphSaid) < 500u;
    if (!redHere) redCol = -1;
    if (advance && redHere) {
        if (redCol >= 0 && (redCol >= cols ||
                            yPos[redCol] - 16 >= yEnd || now - redFrom > 12000u)) redCol = -1;
        if (redCol < 0) {
            if (!redNext) redNext = now + 20000u + (uint32_t)random(0, 20000);
            else if (now >= redNext) {
                int best = -1;
                for (int i = 0; i < cols; i++) {
                    if (yPos[i] < yStart + 16 || yPos[i] > yStart + 70) continue;
                    const int d = 3 + i * SPACING - sqCx;
                    if (sqHere && (d < 0 ? -d : d) < sqHalf + 18) continue;
                    if (best < 0 || ySpeed[i] > ySpeed[best]) best = i;
                }
                if (best >= 0) {
                    redCol = best; redFrom = now;
                    redNext = now + 30000u + (uint32_t)random(0, 30000);
                }
            }
        }
    }
    int redX = -1, redY = 0;

    // Full-band clear every frame, same as every other background style.
    // Without it a column that just wrapped skips its narrow vertical strip
    // for several frames, and nothing else ever repaints that strip -- so
    // anything drawn over it last frame (Squachy included, since he no
    // longer erases his own footprint) is left behind as a stale smear.
    t.fillRect(0, yStart, t.width(), yEnd - yStart, BG);

    t.setTextSize(1);
    // Column advance is gated: yTick is a call-counted divider, not
    // now-based, so calling this twice per logical frame would otherwise
    // fall the rain at double speed.
    if (advance) {
        for (int i = 0; i < cols; i++) {
            // Two cells somewhere in the trail flicker to different glyphs.
            // Freezing characters entirely was the right fix for the old
            // reshuffle-every-frame noise but went a step too far and left
            // the trails completely static. Rolling a couple of cells per
            // column keeps the shimmer at O(cols) rather than O(cells).
            charBuf[i][random(1, colLen[i])] = (uint8_t)random(0, GLN);
            if (random(0, 2)) charBuf[i][random(1, colLen[i])] = (uint8_t)random(0, GLN);

            yTick[i] += s_animK;
            if (yTick[i] >= (float)ySpeed[i]) {
                yTick[i] = 0.0f;
                yPos[i] += 8;
                // A fresh glyph enters at the head; everything already in
                // the buffer shifts one slot further from it.
                for (int j = MAXTRAIL - 1; j > 0; j--) charBuf[i][j] = charBuf[i][j - 1];
                charBuf[i][0] = (uint8_t)random(0, GLN);
                // SPLASH. A head reaching the floor throws a brief flare
                // sideways -- the one place the rain previously just
                // stopped existing. Six slots, oldest reused.
                if (yPos[i] >= yEnd - 8 && yPos[i] < yEnd) {
                    uint8_t slot = 0, oldest = 0;
                    for (uint8_t k = 0; k < 6; k++) {
                        if (splashAge[k] == 0) { slot = k; break; }
                        if (splashAge[k] > oldest) { oldest = splashAge[k]; slot = k; }
                    }
                    splashX[slot]   = (uint16_t)(3 + i * SPACING);
                    splashHue[slot] = colHue[i];
                    splashAge[slot] = 1;
                }
                // Occasionally a glyph breaks off the head and floats.
                if (random(0, 90) == 0 && yPos[i] > yStart + 20 && yPos[i] < yEnd - 20) {
                    for (uint8_t k = 0; k < NPOP; k++) {
                        if (popAge[k]) continue;
                        popX[k]   = (uint16_t)(3 + i * SPACING);
                        popY[k]   = (uint16_t)yPos[i];
                        popCh[k]  = charBuf[i][0];
                        popHue[k] = colHue[i];
                        popAge[k] = 1;
                        break;
                    }
                }
                if (yPos[i] > yEnd + colLen[i] * 8) respawn(i, false);
            }
        }
    }

    // The recovered framebuffer may render 2-16 separate bands. Bound the
    // *drawing* loop to its current physical rows, instead of visiting the
    // entire trail in each band (4 bands previously visited ~4x the cells).
    // The animation/state loop above remains full-screen and runs only when
    // advance=true, so the picture remains identical across bands.
    const int paintTop = (yStart);
    const int paintBottom = (yEnd);
    for (int i = 0; i < cols; i++) {
        const int len = colLen[i];
        if (yPos[i] - (len - 1) * 8 >= paintBottom ||
            yPos[i] + 8 <= paintTop) continue;
        const uint16_t hue   = HUES[colHue[i]];
        const bool     surge = colSurge[i];
        // Depth as a brightness ceiling, 170..255. Narrow on purpose.
        const uint16_t deep = (uint16_t)(170u + ((uint16_t)colDepth[i] * 85u) / 255u);
        const int xBase = 3 + i * SPACING;
        // Near drops lean further than far ones, so a gust reads with depth
        // instead of shunting the whole screen at once.
        // How far into his silhouette this column falls, 0 outside.
        int sqPush = 0, sqDim = 0;
        if (sqHere) {
            const int d = xBase - sqCx;
            const int a = d < 0 ? -d : d;
            if (a < sqHalf + 10) {
                // Nearest the middle of him gets pushed hardest and dimmed
                // most; it tapers off to nothing at the edge of the push
                // zone so there is no hard line down the screen.
                const int strength = ((sqHalf + 10) - a) * 255 / (sqHalf + 10);
                sqPush = ((d < 0 ? -1 : 1) * strength * 7) / 255;
                sqDim  = strength;
            }
        }
        for (int j = 0; j < len; j++) {
            const int16_t ry = (int16_t)(yPos[i] - j * 8);
            if (ry >= paintBottom) continue;  // still below the visible band
            if (ry + 8 <= paintTop) break;     // all later glyphs are above it
            if (ry < yStart || ry >= yEnd) continue;
            // A recovered sprite shows only one vertical strip at a time.
            // Skip off-band glyphs before costly alpha/blend/text operations.
            // State still advances once per logical frame in the loop above.
            // Near drops lean further than far ones, so a gust reads with
            // depth instead of shunting the whole screen at once.
            int x = xBase + sqPush;
            // Inside the roll bar the row is dragged sideways; on the tear
            // it is dragged further still.
            const int rd = ry - rollAt;
            // Rows inside the bright core get dragged sideways, the way a
            // tracking error smears the lines it passes through.
            if (rd >= 0 && rd < 4)       x += 5 - rd;
            if (tearOn && ry >= tearY)   x += tearDx;

            // Fade in across the top few rows. Drops used to appear at full
            // brightness the instant they crossed yStart, which popped.
            uint16_t edge = 255;
            if (ry < yStart + 16) edge = (uint16_t)(((ry - yStart) * 255) / 16);
            // Only dim behind him where he actually is vertically, so the
            // rain above and below his head is untouched.
            if (sqDim && ry >= sqTop && ry <= sqBot)
                edge = (uint16_t)((edge * (uint16_t)(255 - (sqDim * 3) / 4)) / 255u);

            const char buf[2] = { GLYPHS[charBuf[i][j]], 0 };
            uint16_t fg, bg;
            // A white-hot core that decays INTO the hue, rather than the head
            // simply being the hue. The leading character is the brightest
            // thing on screen and the colour trails behind it. On a surge the
            // white runs three cells deep instead of one.
            const int hot = surge ? 3 : 1;
            if (j < hot) {
                uint16_t a = (uint16_t)(((uint32_t)deep * edge) / 255u);
                // Heads blank across the retrace gap too -- a bar that only
                // suppressed the trails would leave the heads hanging in
                // the dark, which reads as a bug rather than a sweep.
                if (rd >= -5 && rd < 0) a = (uint16_t)(a / 7u);
                fg = blend(BG, WHITE, a);
                (void)boltAmt;
                // The glow is free: the glyph's own opaque background fill is
                // drawn either way, so it is tinted rather than left at BG.
                bg = blend(BG, hue, (uint16_t)(a / (surge ? 3u : 4u)));
            } else {
                const float f = 1.0f - (float)(j - hot) / (float)(len - hot);
                uint32_t a = (uint32_t)((surge ? 245.0f : 225.0f) * f * f);
                a = (a * deep) / 255u;
                a = (a * edge) / 255u;
                // Lightning lifts the trail toward white without touching
                // the heads, which are already white -- so the flash reads
                // as the whole field catching the light, not as a fade.
                // Retrace gap: five rows immediately ahead of the bar are
                // knocked down to near nothing. This is what the bright
                // edge is read against.
                if (rd >= -5 && rd < 0) a = (a * 22u) / 255u;
                uint16_t lift = boltAmt;
                if (rd >= 0 && rd < 18) {
                    // Hard core for three rows, then a short decay -- short
                    // on purpose, since a long tail is exactly the haze the
                    // first version turned into.
                    const uint16_t rl = (rd < 3) ? 225u
                                                 : (uint16_t)(150u - (rd - 3) * 10u);
                    if (rl > lift) lift = rl;
                }
                fg = lift ? blend(blend(BG, hue, (uint16_t)a), WHITE, lift)
                          : blend(BG, hue, (uint16_t)a);
                bg = (j <= hot + 1) ? blend(BG, hue, (uint16_t)(a / 6u)) : BG;
            }
            t.setTextColor(fg, bg);
            t.setCursor(x, ry);
            t.print(buf);
            if (i == redCol && j == 2) { redX = x; redY = ry; }
        }
    }
    // Over the column, twice the size, in red with a dark red glow. The
    // glyph itself is whatever that cell is showing.
    if (redX >= 0) {
        const char rb[2] = { GLYPHS[charBuf[redCol][2]], 0 };
        const int gx = redX - 3, gy = redY - 4;
        t.setTextSize(2);
        t.setTextColor(t.color565(255, 40, 40), t.color565(90, 0, 0));
        t.setCursor(gx, gy);
        t.print(rb);
        t.setTextSize(1);
        s_redX = gx + 6; s_redY = gy + 8; s_redAt = now ? now : 1;
    }

    // Grow-and-fade glyphs. Size steps 1 -> 2 -> 3 over the life while the
    // colour washes out, and each one drifts upward, so a character looks
    // like it is lifting off the screen toward the viewer. Transparent
    // background (single-argument setTextColor) is what makes it overlay
    // the rain instead of stamping a black box over it.
    if (advance) for (uint8_t k = 0; k < NPOP; k++) if (popAge[k]) {
        if (++popAge[k] > 15) popAge[k] = 0;
    }
    for (uint8_t k = 0; k < NPOP; k++) {
        if (!popAge[k]) continue;
        const uint8_t age = popAge[k];
        const uint8_t sz  = (age < 5) ? 1 : (age < 10 ? 2 : 3);
        const int py = (int)popY[k] - age - (sz - 1) * 4;
        if (!(py<yEnd && py+8*sz>yStart)) continue;
        const uint16_t a  = (uint16_t)(235u - (uint16_t)age * 15u);
        const char pb[2] = { GLYPHS[popCh[k]], 0 };
        t.setTextSize(sz);
        t.setTextColor(blend(BG, HUES[popHue[k]], a));
        // Re-centre as it grows so it swells about its own middle rather
        // than expanding down and to the right off its anchor.
        t.setCursor((int)popX[k] - (sz - 1) * 3, py);
        t.print(pb);
    }
    t.setTextSize(1);

    // Splashes last ~8 ticks, spreading and fading. Two short horizontal
    // strokes each, so the whole effect is at most twelve drawFastHLine
    // calls in a frame where any are alive at all.
    if (advance) for (uint8_t k = 0; k < 6; k++) if (splashAge[k]) {
        if (++splashAge[k] > 8) splashAge[k] = 0;
    }
    for (uint8_t k = 0; k < 6; k++) {
        if (!splashAge[k]) continue;
        const uint8_t  age  = splashAge[k];
        if (!true) continue;
        const int      sp   = age * 2;
        const uint16_t a    = (uint16_t)(200u - (uint16_t)age * 24u);
        const uint16_t col  = blend(BG, HUES[splashHue[k]], a);
        const int      sy   = yEnd - 2;
        t.drawFastHLine((int)splashX[k] - sp, sy, sp, col);
        t.drawFastHLine((int)splashX[k] + 1,  sy, sp, col);
    }
}
} // namespace SquachWatchScenes
#endif
