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

	// When true, ScreenManager_Draw skips its automatic post-screen Visual_DrawFade call — lets
	// a screen draw the fade itself at a specific point in its own draw order (e.g. behind its
	// HUD instead of over the whole frame). The screen is responsible for calling Visual_DrawFade
	// itself wherever it wants it, and for clearing this back to false on Unload.
	bool autoFadeDrawSuppressed;

	// Screen Shake
	f32 shakeIntensity;
	f32 shakeDuration;
	f32 shakeTimer;
	i32 shakeOffsetX;
	i32 shakeOffsetY;

        // Static transition
        f32 staticTimer;
        f32 staticDuration;

        // TV Snow (black + white speckle noise)
        f32 snowTimer;
        f32 snowDuration;
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
void Visual_DrawDarken(f32 amount);  // one-shot dark dither overlay, independent of fade state
void Visual_SetAutoFadeDrawSuppressed(VisualManager* manager, bool suppressed);

// Screen Shake API
void Visual_StartShake(VisualManager* manager, f32 intensity, f32 duration);
void Visual_ApplyShakeOffset(const VisualManager* manager);
void Visual_ClearShakeOffset(void);

void Visual_TriggerStatic(VisualManager* manager, f32 duration);
void Visual_DrawStatic(VisualManager* manager);  // draws & ticks down timer

// TV Snow — black + white speckle noise (real TV-static look), separate from Visual_DrawStatic
void Visual_TriggerTVSnow(VisualManager* manager, f32 duration);
void Visual_DrawTVSnow(VisualManager* manager);  // draws & ticks down timer

// Screen effects
typedef enum {
    VISUAL_EFFECT_WAVE,   // scanline horizontal shift
    VISUAL_EFFECT_GRAIN,  // pixel dilation + noise (puffy)
} VisualEffectType;

void Visual_BeginEffect(void);
void Visual_EndEffect(void);
void Visual_DrawEffect(VisualEffectType type, f32 level, f32 time);

