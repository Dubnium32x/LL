// written by diskodev
// engine/dialogue.h
#pragma once

#include "util.h"

#define DIALOGUE_MAX_LINES    32
#define DIALOGUE_MAX_CHOICES   8
#define DIALOGUE_MAX_TEXT_LEN 256*4
#define DIALOGUE_MAX_NAME_LEN  32
#define DIALOGUE_MAX_PATH_LEN  64

typedef struct {
    char speakerName[DIALOGUE_MAX_NAME_LEN]; // "" = no name label
    char portraitPath[DIALOGUE_MAX_PATH_LEN]; // "" = no portrait
    char text[DIALOGUE_MAX_TEXT_LEN];
} DialogueLine;

typedef struct {
    char label[64];
    i32  value; // returned to callback when chosen
} DialogueChoice;

typedef void (*DialogueChoiceCallback)(i32 choiceValue);

// anchorX/Y = world-space center of the speaker; box floats near it
void DialogueBox_Show(const DialogueLine* lines, i32 lineCount, i32 anchorX, i32 anchorY);

// queue choices to appear after the last dialogue line is dismissed
void DialogueBox_PushChoices(const DialogueChoice* choices, i32 count, DialogueChoiceCallback callback);

void DialogueBox_Update(f32 deltaTime);
void DialogueBox_Draw(void);
void DialogueBox_DrawBtn(bool isA, i32 x, i32 y);
bool DialogueBox_IsActive(void);
bool DialogueBox_IsDone(void);
void DialogueBox_Dismiss(void);

// shared icon-text utilities (usable by other modules)
void DrawIconText(const char* text, i32 x, i32 y, u8 fontIdx, LCDColor color, i32 lineH);
i32  MeasureIconText(const char* text, u8 fontIdx);
