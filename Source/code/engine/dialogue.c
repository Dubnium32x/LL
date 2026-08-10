// written by diskodev
// engine/dialogue.c
#include "dialogue.h"
#include "text.h"
#include "input.h"
#include "audio.h"
#include <game/data/data.h>

extern PlaydateAPI* pd;
extern LCDFont* fontFamily[5];
extern InputManager inputManager;
extern AudioManager audioManager;
extern SaveData saveData;

// ── layout constants ─────────────────────────────────────────────────────────
#define BOX_W          310
#define BOX_H           72
#define BOX_PAD          7
#define BOX_RADIUS       5
#define PORTRAIT_SIZE   32
// base speeds for Slow / Normal / Fast
static const f32 s_speedTable[3] = { 14.0f, 28.0f, 52.0f };
#define CHARS_PER_SEC  (s_speedTable[saveData.textSpeed < 3 ? saveData.textSpeed : 1])
#define NAME_FONT        0   // octosale.regular
#define BODY_FONT        4   // BillJilly
#define WRAP_MAX_LINES   4
#define WRAP_LINE_LEN  128
#define BTN_SIZE        16  // btn_a / btn_b sprite size

// ── state ─────────────────────────────────────────────────────────────────────
typedef enum { DS_IDLE, DS_TYPING, DS_WAITING, DS_CHOOSING, DS_DONE } DialogueState;

static DialogueLine           s_lines[DIALOGUE_MAX_LINES];
static i32                    s_lineCount;
static i32                    s_lineIndex;
static i32                    s_anchorX, s_anchorY;

static DialogueChoice         s_choices[DIALOGUE_MAX_CHOICES];
static i32                    s_choiceCount;
static i32                    s_choiceIndex;
static DialogueChoiceCallback s_choiceCallback;
static bool                   s_hasPendingChoices;

static f32                    s_charsToShow;
static i32                    s_totalChars;
static i32                    s_lastTickChar;
static i32                    s_boxH;
static i32                    s_boxTopY;
static char                   s_wrapped[WRAP_MAX_LINES][WRAP_LINE_LEN];
static i32                    s_wrappedCount;

static LCDBitmap*             s_portrait;
static char                   s_loadedPath[DIALOGUE_MAX_PATH_LEN];

static LCDBitmap*             s_btnA;
static LCDBitmap*             s_btnB;
static LCDBitmap*             s_btnUp;
static LCDBitmap*             s_btnDown;
static LCDBitmap*             s_btnLeft;
static LCDBitmap*             s_btnRight;

// icon chars: { = A, } = B, ^ = Up, ` = Down, | = Left, ~ = Right
static bool IsIconChar(char c) {
    return c=='{' || c=='}' || c=='^' || c=='`' || c=='|' || c=='~';
}

static DialogueState          s_state = DS_IDLE;

// ── helpers ───────────────────────────────────────────────────────────────────
static void LoadButtonIcons(void) {
    const char* err = NULL;
#define LOAD_ICON(ptr, path) if (!(ptr)) { err=NULL; (ptr)=pd->graphics->loadBitmap((path),&err); if(!(ptr)) LOG("DialogueBox: %s: %s",(path),err?err:"?"); }
    LOAD_ICON(s_btnA,     "assets/texture/image/ui/btn_a")
    LOAD_ICON(s_btnB,     "assets/texture/image/ui/btn_b")
    LOAD_ICON(s_btnUp,    "assets/texture/image/ui/btn_up")
    LOAD_ICON(s_btnDown,  "assets/texture/image/ui/btn_down")
    LOAD_ICON(s_btnLeft,  "assets/texture/image/ui/btn_left")
    LOAD_ICON(s_btnRight, "assets/texture/image/ui/btn_right")
#undef LOAD_ICON
}

static void FreePortrait(void) {
    if (s_portrait) {
        pd->graphics->freeBitmap(s_portrait);
        s_portrait = NULL;
        s_loadedPath[0] = '\0';
    }
}

static void LoadPortrait(const char* path) {
    if (!path || !path[0]) { FreePortrait(); return; }
    if (strcmp(path, s_loadedPath) == 0) return;
    FreePortrait();
    const char* err = NULL;
    s_portrait = pd->graphics->loadBitmap(path, &err);
    if (!s_portrait)
        LOG("DialogueBox: portrait '%s': %s", path, err ? err : "unknown error");
    else
        strncpy(s_loadedPath, path, DIALOGUE_MAX_PATH_LEN - 1);
}

static LCDBitmap* GetIconBitmap(char c) {
    switch (c) {
        case '{': return s_btnA;
        case '}': return s_btnB;
        case '^': return s_btnUp;
        case '`': return s_btnDown;
        case '|': return s_btnLeft;
        case '~': return s_btnRight;
        default:  return NULL;
    }
}

// pixel width accounting for inline icon chars
static i32 MeasureTagged(const char* text) {
    i32 w = 0;
    char run[WRAP_LINE_LEN]; i32 rlen = 0;
    for (const char* p = text; *p; p++) {
        if (IsIconChar(*p)) {
            if (rlen) { run[rlen]='\0'; w+=(i32)GetTextWidth(run,BODY_FONT); rlen=0; }
            w += BTN_SIZE + 2;
        } else { run[rlen++] = *p; }
    }
    if (rlen) { run[rlen]='\0'; w+=(i32)GetTextWidth(run,BODY_FONT); }
    return w;
}

// draws text with inline icons, vertically centering icons within lineH
static void DrawTagged(const char* text, i32 x, i32 y, i32 lineH) {
    i32 cx = x;
    char run[WRAP_LINE_LEN]; i32 rlen = 0;
    for (const char* p = text; *p; p++) {
        if (IsIconChar(*p)) {
            if (rlen) { run[rlen]='\0'; DrawText(run,cx,y,BODY_FONT,kColorBlack); cx+=(i32)GetTextWidth(run,BODY_FONT); rlen=0; }
            LCDBitmap* bmp = GetIconBitmap(*p);
            if (bmp) {
                pd->graphics->setDrawMode(kDrawModeCopy);
                pd->graphics->drawBitmap(bmp, cx, y + (lineH - BTN_SIZE) / 2 - 2, kBitmapUnflipped);
            }
            cx += BTN_SIZE + 2;
        } else { run[rlen++] = *p; }
    }
    if (rlen) { run[rlen]='\0'; DrawText(run,cx,y,BODY_FONT,kColorBlack); }
}

static void WrapText(const char* text, i32 maxWidth) {
    s_wrappedCount = 0;
    s_totalChars   = 0;
    if (!text || !text[0]) return;

    char buf[DIALOGUE_MAX_TEXT_LEN];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char cur[WRAP_LINE_LEN] = "";
    char* word = strtok(buf, " ");

    while (word && s_wrappedCount < WRAP_MAX_LINES) {
        char test[WRAP_LINE_LEN];
        if (cur[0]) snprintf(test, sizeof(test), "%s %s", cur, word);
        else        snprintf(test, sizeof(test), "%s",    word);

        if ((i32)MeasureTagged(test) > maxWidth && cur[0]) {
            strncpy(s_wrapped[s_wrappedCount++], cur, WRAP_LINE_LEN - 1);
            s_totalChars += (i32)strlen(cur);
            strncpy(cur, word, sizeof(cur) - 1);
        } else {
            strncpy(cur, test, sizeof(cur) - 1);
        }
        word = strtok(NULL, " ");
    }
    if (cur[0] && s_wrappedCount < WRAP_MAX_LINES) {
        strncpy(s_wrapped[s_wrappedCount++], cur, WRAP_LINE_LEN - 1);
        s_totalChars += (i32)strlen(cur);
    }
}

static void LoadLine(i32 index) {
    const DialogueLine* line = &s_lines[index];
    LoadPortrait(line->portraitPath);

    // preprocess inline tags before wrapping
    char processed[DIALOGUE_MAX_TEXT_LEN];
    strncpy(processed, line->text, sizeof(processed) - 1);
    processed[sizeof(processed) - 1] = '\0';

    i32 textOffX = BOX_PAD + (s_portrait ? PORTRAIT_SIZE + BOX_PAD : 0);
    // reserve right-side space for the A-button advance indicator
    WrapText(processed, BOX_W - textOffX - BOX_PAD - BTN_SIZE - 4);

    LCDFont* bf = fontFamily[BODY_FONT];
    i32 lh = bf ? pd->graphics->getFontHeight(bf) + 2 : 12;
    s_boxH = BOX_PAD + s_wrappedCount * lh + BOX_PAD + BTN_SIZE / 2;
    if (s_boxH < 40) s_boxH = 40;

    // pin top so box doesn't jump between lines
    i32 maxBoxH = BOX_PAD + WRAP_MAX_LINES * lh + BOX_PAD + BTN_SIZE / 2;
    s_boxTopY = s_anchorY - maxBoxH + 23;
    if (s_boxTopY < 2) s_boxTopY = s_anchorY + 12;

    s_charsToShow  = 0.0f;
    s_lastTickChar  = 0;
    s_state = DS_TYPING;
    PlaySFX(&audioManager, "assets/audio/sfx/textbox_appear", 70, false);
}

static void GetBoxPos(i32* bx, i32* by) {
    *bx = s_anchorX - BOX_W / 2;
    *by = s_boxTopY;

    if (*bx < 2)                   *bx = 2;
    if (*bx + BOX_W > SCR_W - 2)  *bx = SCR_W - 2 - BOX_W;
    if (*by + s_boxH > SCR_H - 2) *by = SCR_H - 2 - s_boxH;
}

// ── public API ────────────────────────────────────────────────────────────────
void DialogueBox_Show(const DialogueLine* lines, i32 lineCount, i32 anchorX, i32 anchorY) {
    if (!lines || lineCount <= 0) return;
    i32 n = lineCount < DIALOGUE_MAX_LINES ? lineCount : DIALOGUE_MAX_LINES;
    memcpy(s_lines, lines, sizeof(DialogueLine) * n);
    s_lineCount          = n;
    s_lineIndex          = 0;
    s_anchorX            = anchorX;
    s_anchorY            = anchorY;
    s_hasPendingChoices  = false;
    s_choiceCount        = 0;
    LoadButtonIcons();
    LoadLine(0);
}

void DialogueBox_PushChoices(const DialogueChoice* choices, i32 count, DialogueChoiceCallback callback) {
    if (!choices || count <= 0) return;
    i32 n = count < DIALOGUE_MAX_CHOICES ? count : DIALOGUE_MAX_CHOICES;
    memcpy(s_choices, choices, sizeof(DialogueChoice) * n);
    s_choiceCount        = n;
    s_choiceIndex        = 0;
    s_choiceCallback     = callback;
    s_hasPendingChoices  = true;

    LCDFont* bf = fontFamily[BODY_FONT];
    i32 lh = bf ? pd->graphics->getFontHeight(bf) + 2 : 12;
    i32 h = BOX_PAD + n * lh + lh + BOX_PAD; // extra lh for "{ to select" hint
    if (h > s_boxH) s_boxH = h;
}

bool DialogueBox_IsActive(void) { return s_state != DS_IDLE && s_state != DS_DONE; }
bool DialogueBox_IsDone(void)   { return s_state == DS_DONE; }

void DialogueBox_Dismiss(void) {
    FreePortrait();
    s_state = DS_IDLE;
}

void DialogueBox_Update(f32 deltaTime) {
    switch (s_state) {
        case DS_TYPING:
            s_charsToShow += CHARS_PER_SEC * deltaTime;
            if ((i32)s_charsToShow >= s_totalChars)
                s_charsToShow = (f32)s_totalChars;
            // A skips the typewriter
            if (Input_IsPressed(&inputManager, INPUT_A))
                s_charsToShow = (f32)s_totalChars;
            // blip every 3 revealed characters
            if ((i32)s_charsToShow >= s_lastTickChar + 1) {
                s_lastTickChar = (i32)s_charsToShow;
                PlaySFX(&audioManager, "assets/audio/sfx/text_letter_blipwav", 50, false);
            }
            if ((i32)s_charsToShow >= s_totalChars) {
                s_state = DS_WAITING;
                PlaySFX(&audioManager, "assets/audio/sfx/textbox_complete", 60, false);
            }
            break;

        case DS_WAITING:
            if (Input_IsPressed(&inputManager, INPUT_A)) {
                PlaySFX(&audioManager, "assets/audio/sfx/textbox_proceed", 70, false);
                s_lineIndex++;
                if (s_lineIndex >= s_lineCount) {
                    if (s_hasPendingChoices) {
                        s_state = DS_CHOOSING;
                        s_choiceIndex = 0;
                    } else {
                        FreePortrait();
                        s_state = DS_DONE;
                    }
                } else {
                    LoadLine(s_lineIndex);
                }
            }
            break;

        case DS_CHOOSING:
            if (Input_IsPressed(&inputManager, INPUT_UP)) {
                s_choiceIndex = (s_choiceIndex + s_choiceCount - 1) % s_choiceCount;
                PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 70, false);
            }
            if (Input_IsPressed(&inputManager, INPUT_DOWN)) {
                s_choiceIndex = (s_choiceIndex + 1) % s_choiceCount;
                PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 70, false);
            }
            if (Input_IsPressed(&inputManager, INPUT_A)) {
                i32 val = s_choices[s_choiceIndex].value;
                FreePortrait();
                s_state = DS_DONE;
                if (s_choiceCallback) s_choiceCallback(val);
            }
            break;

        default:
            break;
    }
}

void DialogueBox_DrawBtn(bool isA, i32 x, i32 y) {
    LoadButtonIcons();
    LCDBitmap* bmp = isA ? s_btnA : s_btnB;
    if (bmp) {
        pd->graphics->setDrawMode(kDrawModeCopy);
        pd->graphics->drawBitmap(bmp, x, y, kBitmapUnflipped);
    }
}

void DialogueBox_Draw(void) {
    if (s_state == DS_IDLE || s_state == DS_DONE) return;

    i32 bx, by;
    GetBoxPos(&bx, &by);

    // outer black round-rect, inset 2px white fill
    pd->graphics->fillRoundRect(bx,     by,     BOX_W,      s_boxH,     BOX_RADIUS,     kColorBlack);
    pd->graphics->fillRoundRect(bx + 2, by + 2, BOX_W - 4, s_boxH - 4, BOX_RADIUS - 1, kColorWhite);

    i32 textX = bx + BOX_PAD;
    i32 textY = by + BOX_PAD;

    // portrait
    if (s_portrait) {
        pd->graphics->setDrawMode(kDrawModeCopy);
        pd->graphics->drawBitmap(s_portrait, bx + BOX_PAD, by + BOX_PAD, kBitmapUnflipped);
        textX = bx + BOX_PAD + PORTRAIT_SIZE + BOX_PAD;
    }

    // floating name tab above box
    i32 cur = s_lineIndex < s_lineCount ? s_lineIndex : s_lineCount - 1;
    const DialogueLine* line = &s_lines[cur];
    i32 nameH = 0;
    if (line->speakerName[0]) {
        LCDFont* nf = fontFamily[NAME_FONT];
        i32 fh   = nf ? pd->graphics->getFontHeight(nf) : 10;
        i32 tw   = (i32)GetTextWidth(line->speakerName, NAME_FONT);
        i32 tabW = tw + 10;
        i32 tabH = fh + 4;
        i32 tabX = textX - 2;
        i32 tabY = by - tabH - 1;
        pd->graphics->fillRoundRect(tabX, tabY, tabW, tabH + BOX_RADIUS, BOX_RADIUS, kColorBlack);
        // flat bottom on tab so it sits flush against the box
        pd->graphics->fillRect(tabX, tabY + tabH - 1, tabW, BOX_RADIUS + 1, kColorBlack);
        DrawText(line->speakerName, tabX + 5, tabY + 2, NAME_FONT, kColorWhite);
        nameH = 0; // name is outside the box now
    }

    i32 bodyY = textY + nameH;

    if (s_state == DS_CHOOSING) {
        LCDFont* bf = fontFamily[BODY_FONT];
        i32 lh = bf ? pd->graphics->getFontHeight(bf) + 2 : 12;
        i32 triW = 6, triH = 10;
        for (i32 i = 0; i < s_choiceCount; i++) {
            i32 cy = bodyY + i * lh;
            if (i == s_choiceIndex) {
                // filled triangle cursor pointing right
                i32 ty = cy + (lh - triH) / 2;
                pd->graphics->fillTriangle(
                    textX,         ty,
                    textX,         ty + triH,
                    textX + triW,  ty + triH / 2,
                    kColorBlack);
                DrawText(s_choices[i].label, textX + triW + 4, cy, BODY_FONT, kColorBlack);
            } else {
                DrawText(s_choices[i].label, textX + triW + 4, cy, BODY_FONT, kColorBlack);
            }
        }
        // "{ to select" hint
        i32 hintY = bodyY + s_choiceCount * lh + 2;
        i32 hintX = textX;
        if (s_btnA) {
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawBitmap(s_btnA, hintX, hintY + (lh - BTN_SIZE) / 2 - 2, kBitmapUnflipped);
        }
        DrawText(" to select", hintX + BTN_SIZE + 2, hintY, BODY_FONT, kColorBlack);
    } else {
        LCDFont* bf = fontFamily[BODY_FONT];
        i32 lh = bf ? pd->graphics->getFontHeight(bf) + 2 : 12;
        i32 charsLeft = (i32)s_charsToShow;
        for (i32 i = 0; i < s_wrappedCount && charsLeft > 0; i++) {
            i32 lineLen = (i32)strlen(s_wrapped[i]);
            i32 show = charsLeft < lineLen ? charsLeft : lineLen;
            char slice[WRAP_LINE_LEN];
            memcpy(slice, s_wrapped[i], show);
            slice[show] = '\0';
            DrawTagged(slice, textX, bodyY + i * lh, lh);
            charsLeft -= lineLen;
        }
        // A button advance indicator when fully shown
        if (s_state == DS_WAITING) {
            i32 ix = bx + BOX_W - BOX_PAD - BTN_SIZE - 2;
            i32 iy = by + s_boxH - BOX_PAD - BTN_SIZE - 2;
            pd->graphics->setDrawMode(kDrawModeCopy);
            if (s_btnA)
                pd->graphics->drawBitmap(s_btnA, ix, iy, kBitmapUnflipped);
        }
    }
}

void DrawIconText(const char* text, i32 x, i32 y, u8 fontIdx, LCDColor color, i32 lineH) {
    LoadButtonIcons();
    i32 cx = x;
    char run[256]; i32 rlen = 0;
    for (const char* p = text; *p; p++) {
        if (IsIconChar(*p)) {
            if (rlen) { run[rlen]='\0'; DrawText(run,cx,y,fontIdx,color); cx+=(i32)GetTextWidth(run,fontIdx); rlen=0; }
            LCDBitmap* bmp = GetIconBitmap(*p);
            if (bmp) {
                pd->graphics->setDrawMode(kDrawModeCopy);
                pd->graphics->drawBitmap(bmp, cx, y + (lineH - 16) / 2 - 2, kBitmapUnflipped);
            }
            cx += 18;
        } else { run[rlen++] = *p; }
    }
    if (rlen) { run[rlen]='\0'; DrawText(run,cx,y,fontIdx,color); }
}

i32 MeasureIconText(const char* text, u8 fontIdx) {
    i32 w = 0;
    char run[256]; i32 rlen = 0;
    for (const char* p = text; *p; p++) {
        if (IsIconChar(*p)) {
            if (rlen) { run[rlen]='\0'; w+=(i32)GetTextWidth(run,fontIdx); rlen=0; }
            w += 18;
        } else { run[rlen++] = *p; }
    }
    if (rlen) { run[rlen]='\0'; w+=(i32)GetTextWidth(run,fontIdx); }
    return w;
}
