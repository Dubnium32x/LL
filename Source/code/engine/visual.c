// written by diskodev
// engine/visual.c
#include "visual.h"

VisualManager visualManager = {0};

static LCDSolidColor Visual_GetSolidColor(VisualFadeColor color) {
	return color == VISUAL_FADE_WHITE ? kColorWhite : kColorBlack;
}

static void Visual_BuildFadePattern(f32 amount, VisualFadeColor color, LCDPattern pattern) {
	static const u8 bayer8x8[8][8] = {
		{ 0, 48, 12, 60, 3, 51, 15, 63 },
		{ 32, 16, 44, 28, 35, 19, 47, 31 },
		{ 8, 56, 4, 52, 11, 59, 7, 55 },
		{ 40, 24, 36, 20, 43, 27, 39, 23 },
		{ 2, 50, 14, 62, 1, 49, 13, 61 },
		{ 34, 18, 46, 30, 33, 17, 45, 29 },
		{ 10, 58, 6, 54, 9, 57, 5, 53 },
		{ 42, 26, 38, 22, 41, 25, 37, 21 }
	};
	const i32 threshold = (i32)(Clamp(amount, 0.0f, 1.0f) * 64.0f + 0.5f);
	const u8 fill = color == VISUAL_FADE_WHITE ? 0xff : 0x00;

	for (i32 row = 0; row < 8; row++) {
		u8 mask = 0;
		pattern[row] = fill;
		for (i32 col = 0; col < 8; col++) {
			if (bayer8x8[row][col] < threshold) {
				mask |= (u8)(1u << (7 - col));
			}
		}
		pattern[row + 8] = mask;
	}
}

void Visual_ResetManager(VisualManager* manager) {
	if (manager == NULL) return;
	memset(manager, 0, sizeof(VisualManager));
	manager->color = VISUAL_FADE_BLACK;
	manager->type = VISUAL_FADE_NONE;
	manager->duration = 0.0f;
	manager->elapsed = 0.0f;
	manager->active = false;
}

void Visual_InitManager(VisualManager* manager) {
	Visual_ResetManager(manager);
}

void Visual_UpdateManager(VisualManager* manager, f32 deltaTime) {
	if (manager == NULL || !manager->active) return;

	manager->elapsed += MaxF(0.0f, deltaTime);
	if (manager->elapsed >= manager->duration) {
		manager->elapsed = manager->duration;
		manager->active = false;
	}
}

void Visual_StartFade(VisualManager* manager, VisualFadeColor color, VisualFadeType type, f32 duration) {
	if (manager == NULL) return;

	if (type == VISUAL_FADE_NONE) {
		Visual_StopFade(manager);
		return;
	}

	manager->color = color;
	manager->type = type;
	manager->duration = MaxF(0.001f, duration);
	manager->elapsed = 0.0f;
	manager->active = true;
}

void Visual_FadeInBlack(VisualManager* manager, f32 duration) {
	Visual_StartFade(manager, VISUAL_FADE_BLACK, VISUAL_FADE_IN, duration);
}

void Visual_FadeOutBlack(VisualManager* manager, f32 duration) {
	Visual_StartFade(manager, VISUAL_FADE_BLACK, VISUAL_FADE_OUT, duration);
}

void Visual_FadeInWhite(VisualManager* manager, f32 duration) {
	Visual_StartFade(manager, VISUAL_FADE_WHITE, VISUAL_FADE_IN, duration);
}

void Visual_FadeOutWhite(VisualManager* manager, f32 duration) {
	Visual_StartFade(manager, VISUAL_FADE_WHITE, VISUAL_FADE_OUT, duration);
}

void Visual_StopFade(VisualManager* manager) {
	if (manager == NULL) return;
	manager->type = VISUAL_FADE_NONE;
	manager->duration = 0.0f;
	manager->elapsed = 0.0f;
	manager->active = false;
}

bool Visual_IsFadeActive(const VisualManager* manager) {
	return manager ? manager->active : false;
}

f32 Visual_GetFadeProgress(const VisualManager* manager) {
	if (manager == NULL || manager->duration <= 0.0f) return 1.0f;
	return Clamp(manager->elapsed / manager->duration, 0.0f, 1.0f);
}

void Visual_DrawFade(const VisualManager* manager) {
	if (manager == NULL || pd == NULL) return;
	if (manager->type == VISUAL_FADE_NONE) return;

	f32 progress = Visual_GetFadeProgress(manager);
	f32 amount = manager->type == VISUAL_FADE_IN ? (1.0f - progress) : progress;
	amount = Clamp(amount, 0.0f, 1.0f);

	if (amount <= 0.0f) return;
	if (amount >= 1.0f) {
		pd->graphics->fillRect(0, 0, SCR_W, SCR_H, Visual_GetSolidColor(manager->color));
		return;
	}

	LCDPattern pattern;
	Visual_BuildFadePattern(amount, manager->color, pattern);
	pd->graphics->fillRect(0, 0, SCR_W, SCR_H, (LCDColor)pattern);
}