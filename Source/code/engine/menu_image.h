// written by diskodev
// engine/menu_image.h
#pragma once

#include "util.h"

#define MENU_IMAGE_MAX_LINES 6
#define MENU_IMAGE_LINE_LEN  48

typedef void (*MenuImageDrawFn)(LCDBitmap* canvas);

void MenuImage_SetInfo(cstr lines[], int lineCount);
void MenuImage_Set(MenuImageDrawFn drawFn, int xOffset);
void MenuImage_Clear(void);

