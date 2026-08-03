// written by diskodev
// engine/visual.h
#pragma once
#include "util.h"

typedef enum {
	VISUAL_FADE_BLACK,
	VISUAL_FADE_WHITE
} VisualFadeColor;

typedef enum {
	VISUAL_FADE_NONE,
	VISUAL_FADE_IN,
	VISUAL_FADE_OUT
} VisualFadeType;

typedef struct {
	VisualFadeColor color;
	VisualFadeType type;
	f32 duration;
	f32 elapsed;
	bool active;
} VisualManager;

extern VisualManager visualManager;

void Visual_InitManager(VisualManager* manager);
void Visual_ResetManager(VisualManager* manager);
void Visual_UpdateManager(VisualManager* manager, f32 deltaTime);

void Visual_StartFade(VisualManager* manager, VisualFadeColor color, VisualFadeType type, f32 duration);
void Visual_FadeInBlack(VisualManager* manager, f32 duration);
void Visual_FadeOutBlack(VisualManager* manager, f32 duration);
void Visual_FadeInWhite(VisualManager* manager, f32 duration);
void Visual_FadeOutWhite(VisualManager* manager, f32 duration);
void Visual_StopFade(VisualManager* manager);

bool Visual_IsFadeActive(const VisualManager* manager);
f32 Visual_GetFadeProgress(const VisualManager* manager);
void Visual_DrawFade(const VisualManager* manager);

