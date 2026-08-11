// written by diskodev
// engine/signage.h
#pragma once

#include "util.h"

// header is optional ("" = omit). Inline icon chars ({ } ^ ` | ~) are supported in text.
void Signage_Show(const char* header, const char* text);

// A headerless box (no dismiss prompt framing) that ignores A/B — can't be dismissed manually —
// and auto-closes on its own after holdDuration seconds. Used for the "glitched sign" sanity
// effect, where a sign shows unsettling text the player can't skip past.
void Signage_ShowGlitch(const char* text, f32 holdDuration);

void Signage_Update(f32 deltaTime);
void Signage_Draw(void);
bool Signage_IsActive(void);
void Signage_Dismiss(void);
