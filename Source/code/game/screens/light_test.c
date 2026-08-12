// written by diskodev
// game/screens/light_test.c
// Showcase of the four light types: lantern, candle, beam, floor.
#include "light_test.h"
#include <engine/engine_core.h>
#include <game/data/objects.h>
#include <math.h>

extern PlaydateAPI*   pd;
extern InputManager   inputManager;
extern ScreenManager  screenManager;
extern LCDFont* fontFamily[5];

// ---- Dither patterns (8-byte pattern + 8-byte mask = 16 bytes) ----
// Each byte is one row of the 8x8 tile. All mask bytes 0xFF = fully opaque.
static const uint8_t kPat75[16] = {
    0xFF,0xEE,0xFF,0xBB,0xFF,0xEE,0xFF,0xBB,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t kPat50[16] = {
    0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t kPat25[16] = {
    0x88,0x00,0x22,0x00,0x88,0x00,0x22,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t kPat12[16] = {
    0x80,0x00,0x08,0x00,0x80,0x00,0x08,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

static f32  s_time    = 0.0f;
static LCDBitmapTable* s_objTable = NULL;

static void DrawObjSprite(i32 x, i32 y, i32 id, bool invert) {
    if (!s_objTable) return;
    LCDBitmap* bmp = pd->graphics->getTableBitmap(s_objTable, id);
    if (!bmp) return;
    LCDBitmapDrawMode mode = invert ? kDrawModeInverted : kDrawModeBlackTransparent;
    pd->graphics->setDrawMode(mode);
    pd->graphics->drawBitmap(bmp, x, y, kBitmapUnflipped);
    pd->graphics->setDrawMode(kDrawModeCopy);
}

// ---- Helpers ----

// Fills a horizontal scanline using a pattern chosen by density (0..1).
static void ScanlineDither(i32 x, i32 y, i32 w, f32 density) {
    if (w <= 0) return;
    LCDColor col;
    if      (density > 0.87f) col = kColorWhite;
    else if (density > 0.62f) col = (LCDColor)(uintptr_t)kPat75;
    else if (density > 0.37f) col = (LCDColor)(uintptr_t)kPat50;
    else if (density > 0.18f) col = (LCDColor)(uintptr_t)kPat25;
    else if (density > 0.06f) col = (LCDColor)(uintptr_t)kPat12;
    else                      return; // too dim to draw
    pd->graphics->fillRect(x, y, w, 1, col);
}

// Draws a radial circular light at (cx, cy) with given outer radius.
// Radius breathes gently over time.
static void DrawRadialLight(i32 cx, i32 cy, i32 radius, f32 time) {
    i32 r = radius + (i32)(sinf(time * 1.8f) * 3.0f);
    i32 r2 = r * r;
    i32 yMin = cy - r, yMax = cy + r;
    for (i32 y = yMin; y <= yMax; y++) {
        i32 dy = y - cy;
        i32 span = (i32)sqrtf((f32)(r2 - dy * dy));
        if (span <= 0) continue;
        // For each scanline, compute density = 1 - (dist from center / radius)
        // We use multiple sub-columns for the density falloff:
        for (i32 dx = -span; dx <= span; dx++) {
            // only draw every other pixel column to batch into fillRect calls
            // Instead do full row with density based on |dy| and |dx|
            (void)dx; break; // use row-level density below
        }
        // density at the edge of this row: use max radius at this y
        f32 distEdge = (f32)dy / (f32)r;
        f32 centerDensity = 1.0f - fabsf(distEdge);
        // inner bright region, middle, outer dim
        i32 rInner = (i32)(r * 0.25f);
        i32 rMid   = (i32)(r * 0.55f);
        i32 rOuter = span;
        i32 rInnerSpan = 0, rMidSpan = 0;
        {
            i32 ri2 = rInner * rInner - dy * dy;
            if (ri2 > 0) rInnerSpan = (i32)sqrtf((f32)ri2);
        }
        {
            i32 rm2 = rMid * rMid - dy * dy;
            if (rm2 > 0) rMidSpan = (i32)sqrtf((f32)rm2);
        }
        i32 x0 = cx - rOuter;
        // outer ring: 12-25%
        if (rOuter > rMidSpan && rMidSpan >= 0) {
            ScanlineDither(cx - rOuter, y, rOuter - rMidSpan, 0.15f * centerDensity + 0.05f);
            ScanlineDither(cx + rMidSpan, y, rOuter - rMidSpan, 0.15f * centerDensity + 0.05f);
        }
        // middle ring: 50%
        if (rMidSpan > rInnerSpan) {
            ScanlineDither(cx - rMidSpan, y, rMidSpan - rInnerSpan, 0.45f);
            ScanlineDither(cx + rInnerSpan, y, rMidSpan - rInnerSpan, 0.45f);
        }
        // inner: 75%
        if (rInnerSpan > 2) {
            // shimmer: vary density slightly per row using time
            f32 shimmer = 0.80f + sinf(time * 4.3f + (f32)y * 0.7f) * 0.08f;
            ScanlineDither(cx - rInnerSpan, y, rInnerSpan * 2, shimmer);
        }
        // bright core (2px)
        if (dy == 0 || dy == -1) {
            ScanlineDither(cx - 2, y, 4, 1.0f);
        }
        (void)x0;
    }
}

// Draws a candle: small radial + oval vertically stretched + flicker + sway.
static void DrawCandle(i32 cx, i32 cy, i32 radius, f32 flicker, f32 time) {
    cx += (i32)(sinf(time * 2.9f) * 2.0f + sinf(time * 5.3f) * 1.0f);
    i32 r  = (i32)(radius * (0.85f + flicker * 0.15f));
    i32 ry = (i32)(r * 1.4f); // taller oval for flame
    for (i32 y = cy - ry; y <= cy + r / 2; y++) {
        i32 dy = y - cy;
        // Ellipse equation: (dx/r)^2 + (dy/ry)^2 <= 1
        f32 dyNorm = (f32)dy / (f32)ry;
        f32 dxMax2 = 1.0f - dyNorm * dyNorm;
        if (dxMax2 <= 0.0f) continue;
        i32 span = (i32)((f32)r * sqrtf(dxMax2));
        if (span <= 0) continue;
        f32 t = sqrtf(dyNorm * dyNorm + (0.0f)); // dist from center as fraction
        f32 density = 1.0f - (fabsf((f32)dy) / (f32)ry) * 0.7f;
        if (density < 0.08f) density = 0.08f;
        i32 innerSpan = (i32)(span * 0.35f);
        // outer halo
        if (span > innerSpan) {
            ScanlineDither(cx - span, y, span - innerSpan, density * 0.3f);
            ScanlineDither(cx + innerSpan, y, span - innerSpan, density * 0.3f);
        }
        // inner glow
        ScanlineDither(cx - innerSpan, y, innerSpan * 2, density * 0.85f + sinf(time * 6.1f + (f32)y * 0.5f) * 0.08f);
        (void)t;
    }
    // bright flame tip
    ScanlineDither(cx - 1, cy - (i32)(ry * 0.5f), 2, 1.0f);
    ScanlineDither(cx - 2, cy - (i32)(ry * 0.3f), 4, 1.0f);
}

// Draws a downward light beam from (cx, originY) with given spread and length.
// Shimmer moves down the beam over time.
static void DrawBeam(i32 cx, i32 originY, i32 maxHalfWidth, i32 length, f32 time) {
    for (i32 dy = 0; dy < length; dy++) {
        f32 t = (f32)dy / (f32)length;
        i32 halfW = (i32)(t * maxHalfWidth);
        f32 density = 1.0f - t * 0.85f;
        // shimmer band scrolls downward
        density += sinf(time * 3.0f - (f32)dy * 0.18f) * 0.12f;
        if (density < 0.0f) density = 0.0f;
        if (halfW <= 0) halfW = 1;
        ScanlineDither(cx - halfW, originY + dy, halfW * 2, density);
    }
    // bright source at top
    ScanlineDither(cx - 2, originY, 4, 1.0f);
}

// Draws a floor light spreading upward from (cx, floorY).
// Brightness bands drift upward over time.
static void DrawFloorLight(i32 cx, i32 floorY, i32 height, f32 time) {
    for (i32 dy = 0; dy < height; dy++) {
        f32 t = (f32)dy / (f32)height;
        // narrows as it goes up
        i32 halfW = (i32)((1.0f - t * 0.6f) * 22.0f);
        f32 density = 1.0f - t * 0.88f;
        // bands drift upward: positive time offset moves pattern toward smaller dy
        density += sinf(time * 2.5f + (f32)dy * 0.22f) * 0.10f;
        if (density < 0.0f) density = 0.0f;
        ScanlineDither(cx - halfW, floorY - dy, halfW * 2, density);
    }
    // bright floor oval
    for (i32 dy = -3; dy <= 3; dy++) {
        f32 t = 1.0f - fabsf((f32)dy / 3.0f) * 0.3f;
        ScanlineDither(cx - 22, floorY + dy, 44, t);
    }
}

// ---- Screen lifecycle ----

void LightTestScreen_Init(void) {
    s_time = 0.0f;
    s_objTable = Asset_LoadBitmapTable("assets/texture/tileset/BASE_TILES_OBJECTS");
}

void LightTestScreen_Update(f32 deltaTime) {
    s_time += deltaTime;

    if (Input_IsPressed(&inputManager, INPUT_B))
        SwitchScreen(&screenManager, SS_TEST_4);
    if (Input_IsPressed(&inputManager, INPUT_UP))
        SwitchScreen(&screenManager, SS_TEST_7);
}

void LightTestScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);

    // Dividers
    pd->graphics->drawLine(100, 0, 100, 240, 1, kColorWhite);
    pd->graphics->drawLine(0, 120, 200, 120, 1, kColorWhite);

    // Labels
    pd->graphics->setDrawMode(kDrawModeFillWhite);
    pd->graphics->setFont(fontFamily[1]);
    pd->graphics->drawText("Lantern",  7, kASCIIEncoding, 4,   2);
    pd->graphics->drawText("Candle",   6, kASCIIEncoding, 108, 2);
    pd->graphics->drawText("Beam",     4, kASCIIEncoding, 4,   122);
    pd->graphics->drawText("Floor",    5, kASCIIEncoding, 108, 122);
    pd->graphics->setDrawMode(kDrawModeCopy);

    // Light A — lantern (top-left, centre 50,70)
    DrawRadialLight(50, 70, 38, s_time);
    DrawObjSprite(50 - 8, 70 - 8, OBJ_LIGHT_A, true);

    // Light B — candle (top-right); sprite centred on the glow origin
    f32 flicker = (sinf(s_time * 11.3f) * 0.5f + 0.5f) *
                  (sinf(s_time * 7.1f)  * 0.5f + 0.5f);
    DrawCandle(150, 70, 28, flicker, s_time);
    DrawObjSprite(150 - 8, 70 - 8, OBJ_LIGHT_B, true);

    // Light C — beam; sprite is the ceiling fixture, beam shoots from its bottom edge
    DrawObjSprite(50 - 8, 133, OBJ_LIGHT_C, false);
    DrawBeam(50, 140, 28, 72, s_time);

    // Light D — floor light; sprite sits at floor level, light spreads upward
    DrawFloorLight(150, 220, 82, s_time);
    DrawObjSprite(150 - 8, 207, OBJ_LIGHT_D, true);
}

void LightTestScreen_Unload(void) {
    s_time = 0.0f;
    if (s_objTable) { Asset_FreeBitmapTable(s_objTable); s_objTable = NULL; }
}
