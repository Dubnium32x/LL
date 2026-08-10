// written by diskodev
// engine/note.h
#pragma once

#include "util.h"

#define NOTE_MAX_LINES    24
#define NOTE_LINE_LEN     64
#define NOTE_LINES_PER_PG  6

typedef struct {
    char title[64];
    char lines[NOTE_MAX_LINES][NOTE_LINE_LEN];
    i32  lineCount;
} GameNote;

void Note_Show(const GameNote* note);
void Note_Update(f32 deltaTime);
void Note_Draw(void);
bool Note_IsActive(void);
bool Note_IsDone(void);
void Note_Dismiss(void);
