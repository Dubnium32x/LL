// written by diskodev
// engine/note.c
#include "note.h"
#include "text.h"
#include "input.h"
#include "dialogue.h"
#include "audio.h"

extern PlaydateAPI* pd;
extern LCDFont* fontFamily[5];
extern InputManager inputManager;
extern AudioManager audioManager;

#define NOTE_W       300
#define NOTE_H       180
#define NOTE_PAD       8
#define NOTE_FONT_TTL  0  // octosale.regular
#define NOTE_FONT_BODY 4  // BillJilly
#define NOTE_RADIUS    3

#define CRANK_DEG_PER_LINE 20.0f
#define SCROLL_LERP        0.18f

static bool     s_active;
static bool     s_done;
static GameNote s_note;
static f32      s_scrollTarget;  // target line index (float)
static f32      s_scrollY;       // lerped line index
static i32      s_maxScroll;
static i32      s_bx, s_by;
static f32      s_crankAccum;

void Note_Show(const GameNote* note) {
    if (!note) return;
    s_note        = *note;
    s_scrollTarget = 0.0f;
    s_scrollY      = 0.0f;
    s_maxScroll    = note->lineCount > NOTE_LINES_PER_PG ? note->lineCount - NOTE_LINES_PER_PG : 0;
    s_active       = true;
    s_done         = false;

    s_bx = (SCR_W - NOTE_W) / 2;
    s_by = (SCR_H - NOTE_H) / 2;
    s_crankAccum = 0.0f;
    PlaySFX(&audioManager, "assets/audio/sfx/textbox_appear", 80, false);
}

void Note_Dismiss(void) { s_active = false; s_done = true; }
bool Note_IsActive(void) { return s_active; }
bool Note_IsDone(void)   { return s_done; }

void Note_Update(f32 deltaTime) {
    (void)deltaTime;
    if (!s_active) return;

    s_crankAccum += Input_GetCrankChange(&inputManager);
    while (s_crankAccum >= CRANK_DEG_PER_LINE) {
        s_crankAccum -= CRANK_DEG_PER_LINE;
        if (s_scrollTarget < s_maxScroll) s_scrollTarget += 1.0f;
    }
    while (s_crankAccum <= -CRANK_DEG_PER_LINE) {
        s_crankAccum += CRANK_DEG_PER_LINE;
        if (s_scrollTarget > 0.0f) s_scrollTarget -= 1.0f;
    }

    if (Input_IsPressed(&inputManager, INPUT_DOWN) || Input_IsDown(&inputManager, INPUT_DOWN)) {
        if (s_scrollTarget < s_maxScroll) s_scrollTarget += 1.0f;
    }
    if (Input_IsPressed(&inputManager, INPUT_UP) || Input_IsDown(&inputManager, INPUT_UP)) {
        if (s_scrollTarget > 0.0f) s_scrollTarget -= 1.0f;
    }

    s_scrollY += (s_scrollTarget - s_scrollY) * SCROLL_LERP;

    if (Input_IsPressed(&inputManager, INPUT_A) || Input_IsPressed(&inputManager, INPUT_B)) {
        PlaySFX(&audioManager, "assets/audio/sfx/menu_back", 70, false);
        s_active = false;
        s_done   = true;
    }
}

void Note_Draw(void) {
    if (!s_active) return;

    i32 bx = s_bx, by = s_by;

    // paper: white fill + black border
    pd->graphics->fillRoundRect(bx,     by,     NOTE_W,     NOTE_H,     NOTE_RADIUS,     kColorBlack);
    pd->graphics->fillRoundRect(bx + 2, by + 2, NOTE_W - 4, NOTE_H - 4, NOTE_RADIUS - 1, kColorWhite);

    LCDFont* tf = fontFamily[NOTE_FONT_TTL];
    LCDFont* bf = fontFamily[NOTE_FONT_BODY];
    i32 tlh = tf ? pd->graphics->getFontHeight(tf) + 3 : 14;
    i32 blh = bf ? pd->graphics->getFontHeight(bf) + 3 : 13;

    i32 cy = by + NOTE_PAD;

    // title + underline
    if (s_note.title[0]) {
        DrawText(s_note.title, bx + NOTE_PAD, cy, NOTE_FONT_TTL, kColorBlack);
        cy += tlh;
        pd->graphics->drawLine(bx + NOTE_PAD, cy, bx + NOTE_W - NOTE_PAD, cy, 1, kColorBlack);
        cy += 4;
    }

    // footer y computed first so clip rect is known
    i32 footY = by + NOTE_H - NOTE_PAD - blh;

    // clip body text between header and footer
    pd->graphics->setClipRect(bx + 2, cy, NOTE_W - 4, footY - cy);

    // body lines — offset by lerped scroll position
    i32 firstLine = (i32)s_scrollY;
    i32 subPx     = (i32)((s_scrollY - firstLine) * blh);
    i32 end = firstLine + NOTE_LINES_PER_PG + 2;
    if (end > s_note.lineCount) end = s_note.lineCount;
    for (i32 i = firstLine; i < end; i++) {
        i32 ly = cy + (i - firstLine) * blh - subPx;
        DrawIconText(s_note.lines[i], bx + NOTE_PAD, ly, NOTE_FONT_BODY, kColorBlack, blh);
    }

    pd->graphics->clearClipRect();
    // footer: scroll hint left, close hint right
    i32 closeW  = 18 + (i32)GetTextWidth(" close", NOTE_FONT_BODY);
    i32 scrollW = (i32)MeasureIconText("^ ` scroll", NOTE_FONT_BODY);
    DialogueBox_DrawBtn(true, bx + NOTE_W - NOTE_PAD - closeW, footY - 2);
    DrawText(" close", bx + NOTE_W - NOTE_PAD - closeW + 18, footY, NOTE_FONT_BODY, kColorBlack);
    DrawIconText("^ ` scroll", bx + NOTE_PAD, footY, NOTE_FONT_BODY, kColorBlack, blh);
    (void)scrollW;
}
