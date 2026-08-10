// written by diskodev
// engine/drip.c
#include "drip.h"

extern PlaydateAPI* pd;

static Drip     s_drips[DRIP_MAX];
static LCDBitmap* s_stainBuf = NULL;  // accumulates dried drip trails

void Drip_Init(void) {
    memset(s_drips, 0, sizeof(s_drips));
    if (!s_stainBuf)
        s_stainBuf = pd->graphics->newBitmap(SCR_W, SCR_H, kColorWhite);
    Drip_Clear();
}

void Drip_Clear(void) {
    for (i32 i = 0; i < DRIP_MAX; i++) s_drips[i].active = false;
    if (s_stainBuf) {
        pd->graphics->pushContext(s_stainBuf);
        pd->graphics->clear(kColorWhite);
        pd->graphics->popContext();
    }
}

void Drip_Spawn(f32 x, f32 stopY) {
    for (i32 i = 0; i < DRIP_MAX; i++) {
        if (s_drips[i].active) continue;
        s_drips[i].x       = x;
        s_drips[i].tailY   = 0.0f;
        s_drips[i].tipY    = 0.0f;
        s_drips[i].speed   = 30.0f + (f32)(rand() % 61);
        s_drips[i].width   = 1 + rand() % 3;
        s_drips[i].stopY   = stopY < 0 ? (f32)SCR_H : stopY;
        s_drips[i].active  = true;
        s_drips[i].stopped = false;
        return;
    }
}

void Drip_SpawnRandom(i32 count, f32 minStopY, f32 maxStopY) {
    for (i32 i = 0; i < count; i++) {
        f32 x    = (f32)(rand() % SCR_W);
        f32 stop = minStopY + (f32)(rand() % (i32)(maxStopY - minStopY + 1));
        Drip_Spawn(x, stop);
    }
}

void Drip_Update(f32 deltaTime) {
    for (i32 i = 0; i < DRIP_MAX; i++) {
        Drip* d = &s_drips[i];
        if (!d->active || d->stopped) continue;

        d->tipY += d->speed * deltaTime;

        if (d->tipY >= d->stopY) {
            d->tipY = d->stopY;
            d->stopped = true;
            // Bake the finished drip into the stain buffer
            if (s_stainBuf) {
                pd->graphics->pushContext(s_stainBuf);
                pd->graphics->setDrawMode(kDrawModeFillBlack);
                i32 x = (i32)d->x - d->width / 2;
                i32 y = (i32)d->tailY;
                i32 h = (i32)(d->tipY - d->tailY);
                if (h > 0) pd->graphics->fillRect(x, y, d->width, h, kColorBlack);
                // blob at tip
                pd->graphics->fillEllipse(x - 1, (i32)d->tipY - 1, d->width + 2, d->width + 2, 0, 360, kColorBlack);
                pd->graphics->setDrawMode(kDrawModeCopy);
                pd->graphics->popContext();
                d->active = false;
            }
        }
    }
}

void Drip_Draw(void) {
    if (!pd) return;

    // Draw stain buffer (dried drips)
    if (s_stainBuf) {
        pd->graphics->setDrawMode(kDrawModeBlackTransparent);
        pd->graphics->drawBitmap(s_stainBuf, 0, 0, kBitmapUnflipped);
        pd->graphics->setDrawMode(kDrawModeCopy);
    }

    // Draw active (falling) drips
    pd->graphics->setDrawMode(kDrawModeFillBlack);
    for (i32 i = 0; i < DRIP_MAX; i++) {
        Drip* d = &s_drips[i];
        if (!d->active || d->stopped) continue;
        i32 x = (i32)d->x - d->width / 2;
        i32 y = (i32)d->tailY;
        i32 h = (i32)(d->tipY - d->tailY) + 1;
        if (h > 0) pd->graphics->fillRect(x, y, d->width, h, kColorBlack);
        // teardrop blob at tip
        pd->graphics->fillEllipse(x - 1, (i32)d->tipY - 1, d->width + 2, d->width + 2, 0, 360, kColorBlack);
    }
    pd->graphics->setDrawMode(kDrawModeCopy);
}
