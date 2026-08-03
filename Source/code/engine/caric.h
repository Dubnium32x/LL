// written by diskodev
// engine/caric.h
#pragma once

#include "util.h"
#include "animation.h"

typedef enum {
    CARIC_TYPE_PLAYER,
    CARIC_TYPE_ENEMY,
    CARIC_TYPE_NPC,
    CARIC_TYPE_ITEM,
    CARIC_TYPE_BACKGROUND,
    CARIC_TYPE_EFFECT,
    CARIC_TYPE_UI,
    CARIC_TYPE_TRIGGER,
    CARIC_TYPE_DECOR,
    CARIC_TYPE_OTHERS,
    CARIC_TYPE_NULL
} CaricType;

typedef struct {
    u16 id;

    cstr name;
    LCDBitmap* bitmap;
    const AnimationSet* animSet;
    AnimationState animState;
    Vec2 position;
    Vec2 scale;
    Vec2 origin;
    f32 rotation;
    LCDBitmapDrawMode drawMode;
    bool isAnimated;
    bool isVisible;
    CaricType type;
    bool flipX;
    bool flipY;
} Caricature;

void Caric_Init(Caricature* caric, cstr name, LCDBitmap* bitmap,
               Vec2 position, Vec2 scale, CaricType type);

void Caric_InitAnimated(Caricature* caric, cstr name,
                        const AnimationSet* animSet, cstr initialSequence,
                        Vec2 position, Vec2 scale, CaricType type);

void Caric_Update(Caricature* caric, f32 deltaTime);

void Caric_Draw(Caricature* caric);

LCDBitmap* Caric_GetCurrentBitmap(Caricature* caric);

void Caric_SetPosition(Caricature* caric, Vec2 newPosition);
void Caric_SetScale(Caricature* caric, Vec2 newScale);
void Caric_SetVisible(Caricature* caric, bool isVisible);
void Caric_SetOrigin(Caricature* caric, Vec2 newOrigin);

Vec2 Caric_GetPosition(Caricature* caric);
Vec2 Caric_GetScale(Caricature* caric);
Vec2 Caric_GetOrigin(Caricature* caric);

void Caric_SetDrawMode(Caricature* caric, LCDBitmapDrawMode mode);
void Caric_SetFlipX(Caricature* caric, bool flip);
void Caric_SetFlipY(Caricature* caric, bool flip);

bool Caric_PlayAnimation(Caricature* caric, cstr sequenceName);
void Caric_StartAnimation(Caricature* caric);
void Caric_StopAnimation(Caricature* caric);
void Caric_ResetAnimation(Caricature* caric);
void Caric_SetFrame(Caricature* caric, u8 frame);
u8 Caric_GetFrame(Caricature* caric);
u8 Caric_GetFrameCount(Caricature* caric);
bool Caric_IsAnimationDone(Caricature* caric);
cstr Caric_GetAnimationName(Caricature* caric);

CaricType Caric_GetType(Caricature* caric);
bool Caric_IsVisible(Caricature* caric);
