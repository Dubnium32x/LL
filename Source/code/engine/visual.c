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
	manager->shakeIntensity = 0.0f;
	manager->shakeDuration = 0.0f;
	manager->shakeTimer = 0.0f;
	manager->shakeOffsetX = 0;
	manager->shakeOffsetY = 0;
}

void Visual_InitManager(VisualManager* manager) {
	Visual_ResetManager(manager);
}

void Visual_UpdateManager(VisualManager* manager, f32 deltaTime) {
	if (manager == NULL) return;

	// Fade update
	if (manager->active) {
		manager->elapsed += MaxF(0.0f, deltaTime);
		if (manager->elapsed >= manager->duration) {
			manager->elapsed = manager->duration;
			manager->active = false;
		}
	}

	// Shake update
	if (manager->shakeTimer > 0.0f) {
		manager->shakeTimer -= MaxF(0.0f, deltaTime);
		if (manager->shakeTimer <= 0.0f) {
			manager->shakeTimer = 0.0f;
			manager->shakeOffsetX = 0;
			manager->shakeOffsetY = 0;
		} else {
			// Calculate decaying intensity
			f32 currentIntensity = manager->shakeIntensity * (manager->shakeTimer / manager->shakeDuration);
			i32 maxOffset = (i32)currentIntensity;
			if (maxOffset < 1) maxOffset = 1;
			manager->shakeOffsetX = (rand() % (maxOffset * 2 + 1)) - maxOffset;
			manager->shakeOffsetY = (rand() % (maxOffset * 2 + 1)) - maxOffset;
		}
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

void Visual_StartShake(VisualManager* manager, f32 intensity, f32 duration) {
	if (manager == NULL) return;
	manager->shakeIntensity = intensity;
	manager->shakeDuration = MaxF(0.001f, duration);
	manager->shakeTimer = manager->shakeDuration;
}

void Visual_ApplyShakeOffset(const VisualManager* manager) {
	if (manager == NULL || pd == NULL) return;
	if (manager->shakeOffsetX != 0 || manager->shakeOffsetY != 0) {
		pd->graphics->setDrawOffset(manager->shakeOffsetX, manager->shakeOffsetY);
	}
}

void Visual_ClearShakeOffset(void) {
	if (pd == NULL) return;
	pd->graphics->setDrawOffset(0, 0);
}

void Visual_TriggerStatic(VisualManager* manager, f32 duration) {
	if (!manager) return;
	manager->staticDuration = duration;
	manager->staticTimer    = duration;
}

void Visual_DrawStatic(VisualManager* manager) {
	if (!manager || !pd || manager->staticTimer <= 0.0f) return;

	// Density fades out as timer winds down
	f32 t        = manager->staticTimer / manager->staticDuration;
	i32 noisePx  = (i32)(t * 4000.0f);
	u32 seed     = (u32)(manager->staticTimer * 10000.0f);

	pd->graphics->setDrawMode(kDrawModeXOR);
	for (i32 i = 0; i < noisePx; i++) {
		seed = seed * 1664525u + 1013904223u;
		i32 nx = (i32)((seed >> 16) & 0x1FF) % SCR_W;
		seed = seed * 1664525u + 1013904223u;
		i32 ny = (i32)((seed >> 16) & 0xFF) % SCR_H;
		pd->graphics->fillRect(nx, ny, 2, 2, kColorBlack);
	}
	pd->graphics->setDrawMode(kDrawModeCopy);

	manager->staticTimer -= 1.0f / 30.0f;
	if (manager->staticTimer < 0.0f) manager->staticTimer = 0.0f;
}

// ---- Screen effects --------------------------------------------------------

static LCDBitmap* s_effectBuf = NULL;

void Visual_BeginEffect(void) {
	if (!pd) return;
	if (!s_effectBuf)
		s_effectBuf = pd->graphics->newBitmap(SCR_W, SCR_H, kColorWhite);
	pd->graphics->pushContext(s_effectBuf);
}

void Visual_EndEffect(void) {
	if (!pd) return;
	pd->graphics->popContext();
}

static void DrawEffect_Wave(f32 level, f32 time) {
	f32 amp = level * 10.0f;
	for (i32 y = 0; y < SCR_H; y++) {
		i32 shift = (i32)(sinf(y * 0.08f + time * 4.0f) * amp
		             + sinf(y * 0.03f + time * 1.7f) * amp * 0.4f);
		pd->graphics->setClipRect(0, y, SCR_W, 1);
		pd->graphics->drawBitmap(s_effectBuf, shift, 0, kBitmapUnflipped);
	}
	pd->graphics->clearClipRect();
}

static void DrawEffect_Grain(f32 level, f32 time) {
	pd->graphics->drawBitmap(s_effectBuf, 0, 0, kBitmapUnflipped);
	// Scatter XOR noise — flips pixels so it grains both light and dark areas
	u32 seed = (u32)(time * 60.0f);
	i32 noisePx = (i32)(level * 1200.0f);
	pd->graphics->setDrawMode(kDrawModeXOR);
	for (i32 i = 0; i < noisePx; i++) {
		seed = seed * 1664525u + 1013904223u;
		i32 nx = (i32)((seed >> 16) & 0x1FF) % SCR_W;
		seed = seed * 1664525u + 1013904223u;
		i32 ny = (i32)((seed >> 16) & 0xFF) % SCR_H;
		pd->graphics->fillRect(nx, ny, 2, 2, kColorBlack);
	}
	pd->graphics->setDrawMode(kDrawModeCopy);
}

void Visual_DrawEffect(VisualEffectType type, f32 level, f32 time) {
	if (!pd || !s_effectBuf || level <= 0.0f) {
		if (s_effectBuf) pd->graphics->drawBitmap(s_effectBuf, 0, 0, kBitmapUnflipped);
		return;
	}
	switch (type) {
		case VISUAL_EFFECT_WAVE:  DrawEffect_Wave(level, time);  break;
		case VISUAL_EFFECT_GRAIN: DrawEffect_Grain(level, time); break;
	}
}
