// written by diskodev
// game/screens/outline_test.c
// Draws test_new_linc.png scaled up, with its silhouette outline flashing black/white every
// frame — every opaque pixel adjacent to a transparent (or off-image) neighbor gets redrawn
// each frame in whichever color is "on" that frame.
#include "outline_test.h"
#include <engine/engine_core.h>

extern PlaydateAPI*   pd;
extern InputManager   inputManager;
extern ScreenManager  screenManager;

#define SPRITE_PATH "assets/texture/image/test_new_linc"
#define SPRITE_SCALE 8
#define MAX_SPRITE_DIM 32

static LCDBitmap* s_sprite = NULL;
static i32 s_spriteW = 0, s_spriteH = 0;
static bool s_outline[MAX_SPRITE_DIM * MAX_SPRITE_DIM];
static i32 s_frameCount = 0;

static bool IsOpaqueAt(i32 x, i32 y) {
    if (x < 0 || y < 0 || x >= s_spriteW || y >= s_spriteH) return false;
    return pd->graphics->getBitmapPixel(s_sprite, x, y) != kColorClear;
}

void OutlineTestScreen_Init(void) {
    s_frameCount = 0;
    s_sprite = Asset_LoadBitmap(SPRITE_PATH);
    if (!s_sprite) {
        LOG("OutlineTest: failed to load %s", SPRITE_PATH);
        return;
    }

    int w = 0, h = 0;
    pd->graphics->getBitmapData(s_sprite, &w, &h, NULL, NULL, NULL);
    if (w > MAX_SPRITE_DIM || h > MAX_SPRITE_DIM) {
        LOG("OutlineTest: sprite %dx%d exceeds MAX_SPRITE_DIM %d, clamping", w, h, MAX_SPRITE_DIM);
        if (w > MAX_SPRITE_DIM) w = MAX_SPRITE_DIM;
        if (h > MAX_SPRITE_DIM) h = MAX_SPRITE_DIM;
    }
    s_spriteW = w;
    s_spriteH = h;

    // A pixel is part of the outline if it's opaque itself but has at least one transparent
    // (or off-image) neighbor — the boundary between the character and everything around it.
    for (i32 y = 0; y < s_spriteH; y++) {
        for (i32 x = 0; x < s_spriteW; x++) {
            bool isEdge = IsOpaqueAt(x, y) &&
                (!IsOpaqueAt(x - 1, y) || !IsOpaqueAt(x + 1, y) ||
                 !IsOpaqueAt(x, y - 1) || !IsOpaqueAt(x, y + 1));
            s_outline[y * MAX_SPRITE_DIM + x] = isEdge;
        }
    }
}

void OutlineTestScreen_Update(f32 deltaTime) {
    (void)deltaTime;
    s_frameCount++;

    if (Input_IsPressed(&inputManager, INPUT_B))
        SwitchScreen(&screenManager, SS_TEST_4);
}

void OutlineTestScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);
    if (!s_sprite) return;

    i32 originX = (SCR_W - s_spriteW * SPRITE_SCALE) / 2;
    i32 originY = (SCR_H - s_spriteH * SPRITE_SCALE) / 2;

    pd->graphics->drawScaledBitmap(s_sprite, originX, originY, (float)SPRITE_SCALE, (float)SPRITE_SCALE);

    LCDColor outlineColor = (s_frameCount % 2 == 0) ? kColorWhite : kColorBlack;
    for (i32 y = 0; y < s_spriteH; y++) {
        for (i32 x = 0; x < s_spriteW; x++) {
            if (!s_outline[y * MAX_SPRITE_DIM + x]) continue;
            pd->graphics->fillRect(originX + x * SPRITE_SCALE, originY + y * SPRITE_SCALE,
                                    SPRITE_SCALE, SPRITE_SCALE, outlineColor);
        }
    }
}

void OutlineTestScreen_Unload(void) {
    if (s_sprite) {
        Asset_FreeBitmap(s_sprite);
        s_sprite = NULL;
    }
}
