// written by diskodev
// game/screens/phys_test.h
#pragma once
#include <engine/util.h>

void PhysTestScreen_Init(void);
void PhysTestScreen_Update(f32 deltaTime);
void PhysTestScreen_Draw(void);
void PhysTestScreen_Unload(void);

// True while a fake "turn off" or fake crash sanity glitch is on screen — used to swap in a
// gag pause-menu image instead of the usual debug info.
bool PhysTestScreen_IsSanityGlitchActive(void);

// True while sanity is at/below the low-meter threshold (20%) — gates the blank-sign glitch
// and the "Are you okay?" pause-menu effect.
bool PhysTestScreen_IsSanityLow(void);
