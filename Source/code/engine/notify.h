// written by diskodev
// engine/notify.h
#pragma once

#include "util.h"

// system message that auto-dismisses after duration seconds
void Notify_Show(const char* text, f32 duration);
void Notify_Update(f32 deltaTime);
void Notify_Draw(void);
bool Notify_IsActive(void);
bool Notify_IsDone(void);
void Notify_Dismiss(void);
