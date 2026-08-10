// written by diskodev
// engine/signage.h
#pragma once

#include "util.h"

// header is optional ("" = omit). Inline icon chars ({ } ^ ` | ~) are supported in text.
void Signage_Show(const char* header, const char* text);
void Signage_Update(f32 deltaTime);
void Signage_Draw(void);
bool Signage_IsActive(void);
void Signage_Dismiss(void);
