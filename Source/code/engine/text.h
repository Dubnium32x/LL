// written by diskodev
// engine/text.h
#pragma once

#include "util.h"

static inline void DrawText(cstr text, i32 x, i32 y, u8 fontFamIndex, LCDColor color) {
    if (!pd || !pd->graphics || !text) return;
    if (fontFamIndex < 0 || fontFamIndex >= 4) return;

    LCDFont* font = fontFamily[fontFamIndex];
    if (!font) return;

    pd->graphics->setFont(font);
    pd->graphics->setDrawMode(color == kColorWhite ? kDrawModeFillWhite : kDrawModeFillBlack);
    pd->graphics->drawText(text, strlen(text), kASCIIEncoding, x, y);
}

static inline void DrawTextFast(cstr text, i32 x, i32 y) {
    if (!pd || !pd->graphics || !text) return;

    LCDFont* font = fontFamily[2]; // Use the third font in the family for fast drawing
    if (!font) return;

    pd->graphics->setFont(font);
    pd->graphics->setDrawMode(kDrawModeXOR); // Use XOR mode for faster drawing, regardless of background color
    pd->graphics->drawText(text, strlen(text), kASCIIEncoding, x, y);
}

static inline void LoadFonts(PlaydateAPI* pd) {
    if (!pd) return;

    const char* err = NULL;
    fontFamily[0] = pd->graphics->loadFont(FONT_PATH "octosale.regular.fnt", &err);
    if (fontFamily[0] == NULL && err) LOG("LoadFonts: %s", err);

    err = NULL;
    fontFamily[1] = pd->graphics->loadFont(FONT_PATH "resmont.light.fnt", &err);
    if (fontFamily[1] == NULL && err) LOG("LoadFonts: %s", err);

    err = NULL;
    fontFamily[2] = pd->graphics->loadFont(FONT_PATH "sonic-hud-life.fnt", &err);
    if (fontFamily[2] == NULL && err) LOG("LoadFonts: %s", err);

    err = NULL;
    fontFamily[3] = pd->graphics->loadFont(FONT_PATH "stories-thinking.regular.fnt", &err);
    if (fontFamily[3] == NULL && err) LOG("LoadFonts: %s", err);
}