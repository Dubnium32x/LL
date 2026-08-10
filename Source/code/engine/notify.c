// written by diskodev
// engine/notify.c
#include "notify.h"
#include "text.h"
#include "input.h"
#include "dialogue.h"
#include "audio.h"

extern PlaydateAPI* pd;
extern LCDFont* fontFamily[5];
extern InputManager inputManager;
extern AudioManager audioManager;

#define NOTIFY_PAD      6
#define NOTIFY_MAX_W  220
#define NOTIFY_FONT     4   // BillJilly
#define NOTIFY_RADIUS   4
#define NOTIFY_X        8
#define NOTIFY_Y        8

static bool s_active;
static bool s_done;
static char s_text[128];
static i32  s_boxW;
static i32  s_boxH;
static f32  s_timer;
static f32  s_duration;

void Notify_Show(const char* text, f32 duration) {
    strncpy(s_text, text ? text : "", sizeof(s_text) - 1);
    s_text[sizeof(s_text) - 1] = '\0';
    s_active   = true;
    s_done     = false;
    s_duration = duration > 0.0f ? duration : 2.5f;
    s_timer    = 0.0f;

    LCDFont* f = fontFamily[NOTIFY_FONT];
    i32 fh = f ? pd->graphics->getFontHeight(f) : 10;

    i32 textW = (i32)MeasureIconText(s_text, NOTIFY_FONT);
    if (textW > NOTIFY_MAX_W - NOTIFY_PAD * 2)
        textW = NOTIFY_MAX_W - NOTIFY_PAD * 2;

    s_boxW = textW + NOTIFY_PAD * 2;
    s_boxH = fh + NOTIFY_PAD * 2;

    PlaySFX(&audioManager, "assets/audio/sfx/textbox_appear", 60, false);
}

void Notify_Dismiss(void) { s_active = false; s_done = true; }
bool Notify_IsActive(void) { return s_active; }
bool Notify_IsDone(void)   { return s_done; }

void Notify_Update(f32 deltaTime) {
    if (!s_active) return;
    s_timer += deltaTime;
    if (s_timer >= s_duration) {
        s_active = false;
        s_done   = true;
    }
}

void Notify_Draw(void) {
    if (!s_active) return;

    i32 bx = NOTIFY_X, by = NOTIFY_Y;

    pd->graphics->fillRoundRect(bx,     by,     s_boxW,     s_boxH,     NOTIFY_RADIUS,     kColorBlack);
    pd->graphics->fillRoundRect(bx + 2, by + 2, s_boxW - 4, s_boxH - 4, NOTIFY_RADIUS - 1, kColorWhite);

    LCDFont* f = fontFamily[NOTIFY_FONT];
    i32 fh = f ? pd->graphics->getFontHeight(f) : 10;
    i32 lh = fh + 2;

    i32 cx = bx + NOTIFY_PAD;
    i32 cy = by + NOTIFY_PAD;

    DrawIconText(s_text, cx, cy, NOTIFY_FONT, kColorBlack, lh);
}
