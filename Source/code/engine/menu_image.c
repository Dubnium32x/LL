// written by diskodev
// engine/menu_image.c

#include "menu_image.h"

static LCDBitmap* menuBitmap = NULL;
char s_lines[MENU_IMAGE_MAX_LINES][MENU_IMAGE_LINE_LEN];
int  s_lineCount = 0;

void MenuImage_SetInfo(cstr lines[], int lineCount) {
    s_lineCount = lineCount < MENU_IMAGE_MAX_LINES ? lineCount : MENU_IMAGE_MAX_LINES;
    for (int i = 0; i < s_lineCount; i++) {
        int len = lines[i] ? (int)__builtin_strlen(lines[i]) : 0;
        if (len >= MENU_IMAGE_LINE_LEN) len = MENU_IMAGE_LINE_LEN - 1;
        if (lines[i] && len > 0) __builtin_memcpy(s_lines[i], lines[i], len);
        s_lines[i][len] = '\0';
    }
}

// Call from kEventPause in eventHandler to push the current bitmap to the system.
void MenuImage_Set(MenuImageDrawFn drawFn, int xOffset) {
    if (!menuBitmap) {
        menuBitmap = pd->graphics->newBitmap(SCR_W, SCR_H, kColorWhite);
    }

    pd->graphics->pushContext(menuBitmap);
    pd->graphics->clear(kColorWhite);
    if (drawFn) drawFn(menuBitmap);
    pd->graphics->popContext();

    pd->system->setMenuImage(menuBitmap, xOffset);
}

void MenuImage_Clear(void) {
    pd->system->setMenuImage(NULL, 0);
    if (menuBitmap) {
        pd->graphics->freeBitmap(menuBitmap);
        menuBitmap = NULL;
    }
}
