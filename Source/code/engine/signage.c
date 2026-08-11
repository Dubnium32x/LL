// written by diskodev
// engine/signage.c
#include "signage.h"
#include "text.h"
#include "input.h"
#include "dialogue.h"
#include "audio.h"

extern PlaydateAPI* pd;
extern LCDFont* fontFamily[5];
extern InputManager inputManager;
extern AudioManager audioManager;

#define SIG_W        300
#define SIG_PAD        8
#define SIG_RADIUS     5
#define SIG_FONT_HDR   0  // octosale.regular
#define SIG_FONT_BODY  1  // resmont.light
#define SIG_WRAP_LINES 6
#define SIG_LINE_LEN  96

static bool  s_active;
static char  s_header[64];
static char  s_wrapped[SIG_WRAP_LINES][SIG_LINE_LEN];
static i32   s_wrapCount;
static i32   s_boxH;
static i32   s_boxX, s_boxY;
static bool  s_blank;       // true while showing a blank sign — ignores A/B, auto-closes itself
static f32   s_blankTimer;

static void WrapBody(const char* text, i32 maxW) {
    s_wrapCount = 0;
    char buf[512];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char cur[SIG_LINE_LEN] = "";
    char* word = strtok(buf, " ");
    while (word && s_wrapCount < SIG_WRAP_LINES) {
        char test[SIG_LINE_LEN];
        if (cur[0]) snprintf(test, sizeof(test), "%s %s", cur, word);
        else        snprintf(test, sizeof(test), "%s",    word);

        if (MeasureIconText(test, SIG_FONT_BODY) > maxW && cur[0]) {
            strncpy(s_wrapped[s_wrapCount++], cur, SIG_LINE_LEN - 1);
            strncpy(cur, word, sizeof(cur) - 1);
        } else {
            strncpy(cur, test, sizeof(cur) - 1);
        }
        word = strtok(NULL, " ");
    }
    if (cur[0] && s_wrapCount < SIG_WRAP_LINES)
        strncpy(s_wrapped[s_wrapCount++], cur, SIG_LINE_LEN - 1);
}

void Signage_Show(const char* header, const char* text) {
    s_blank = false;
    strncpy(s_header, header ? header : "", sizeof(s_header) - 1);

    LCDFont* bf = fontFamily[SIG_FONT_BODY];
    LCDFont* hf = fontFamily[SIG_FONT_HDR];
    i32 blh = bf ? pd->graphics->getFontHeight(bf) + 2 : 12;
    i32 hlh = hf ? pd->graphics->getFontHeight(hf) + 3 : 14;

    WrapBody(text, SIG_W - SIG_PAD * 2);

    s_boxH = SIG_PAD;
    if (s_header[0]) s_boxH += hlh + 4; // header + separator gap
    s_boxH += s_wrapCount * blh + SIG_PAD;
    if (s_boxH < 36) s_boxH = 36;

    s_boxX = (SCR_W - SIG_W) / 2;
    s_boxY = (SCR_H - s_boxH) / 2;
    s_active = true;
    PlaySFX(&audioManager, "assets/audio/sfx/textbox_appear", 80, false);
}

void Signage_ShowGlitch(const char* text, f32 holdDuration) {
    Signage_Show("", text);
    s_blank      = true;
    s_blankTimer = holdDuration;
}

void Signage_Dismiss(void) {
    if (s_active) PlaySFX(&audioManager, "assets/audio/sfx/textbox_proceed", 70, false);
    s_active = false;
    s_blank  = false;
}
bool Signage_IsActive(void) { return s_active; }

void Signage_Update(f32 deltaTime) {
    if (!s_active) return;

    if (s_blank) {
        // Can't be dismissed manually — A/B are ignored, it just runs out its own clock.
        s_blankTimer -= deltaTime;
        if (s_blankTimer <= 0.0f) {
            PlaySFX(&audioManager, "assets/audio/sfx/textbox_proceed", 70, false);
            s_active = false;
            s_blank  = false;
        }
        return;
    }

    if (Input_IsPressed(&inputManager, INPUT_A) || Input_IsPressed(&inputManager, INPUT_B)) {
        PlaySFX(&audioManager, "assets/audio/sfx/textbox_proceed", 70, false);
        s_active = false;
    }
}

void Signage_Draw(void) {
    if (!s_active) return;

    i32 bx = s_boxX, by = s_boxY;

    // double border: outer black, gap 2, inner black line, white fill
    pd->graphics->fillRoundRect(bx,     by,     SIG_W,     s_boxH,     SIG_RADIUS,     kColorBlack);
    pd->graphics->fillRoundRect(bx + 2, by + 2, SIG_W - 4, s_boxH - 4, SIG_RADIUS - 1, kColorWhite);
    pd->graphics->drawRoundRect(bx + 2, by + 2, SIG_W - 4, s_boxH - 4, SIG_RADIUS - 1, 2, kColorBlack);

    LCDFont* bf = fontFamily[SIG_FONT_BODY];
    LCDFont* hf = fontFamily[SIG_FONT_HDR];
    i32 blh = bf ? pd->graphics->getFontHeight(bf) + 2 : 12;
    i32 hlh = hf ? pd->graphics->getFontHeight(hf) + 3 : 14;

    i32 cy = by + SIG_PAD;

    if (s_header[0]) {
        i32 hw = MeasureIconText(s_header, SIG_FONT_HDR);
        DrawIconText(s_header, bx + (SIG_W - hw) / 2, cy, SIG_FONT_HDR, kColorBlack, hlh);
        cy += hlh;
        // divider line
        pd->graphics->drawLine(bx + SIG_PAD * 2, cy, bx + SIG_W - SIG_PAD * 2, cy, 1, kColorBlack);
        cy += 4;
    }

    for (i32 i = 0; i < s_wrapCount; i++) {
        i32 lw = MeasureIconText(s_wrapped[i], SIG_FONT_BODY);
        DrawIconText(s_wrapped[i], bx + (SIG_W - lw) / 2, cy, SIG_FONT_BODY, kColorBlack, blh);
        cy += blh;
    }
}
