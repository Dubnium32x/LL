// written by diskodev
// engine/caric.c
#include "caric.h"

static u16 nextCaricId = 0;

void Caric_Init(Caricature* caric, cstr name, LCDBitmap* bitmap,
               Vec2 position, Vec2 scale, CaricType type) {
    if (caric == NULL) return;
    memset(caric, 0, sizeof(Caricature));

    caric->id = nextCaricId++;
    caric->name = name;
    caric->bitmap = bitmap;
    caric->animSet = NULL;
    AnimState_Init(&caric->animState);
    caric->position = position;
    caric->scale = scale;
    caric->origin = (Vec2){DIV2, DIV2};
    caric->rotation = 0.0f;
    caric->drawMode = kDrawModeCopy;
    caric->isAnimated = false;
    caric->isVisible = true;
    caric->type = type;
    caric->flipX = false;
    caric->flipY = false;

    if (bitmap == NULL) {
        printd(DEBUG_CARIC, "Caric_Init: '%s' initialized with NULL bitmap", name ? name : "?");
    }
}

void Caric_InitAnimated(Caricature* caric, cstr name,
                        const AnimationSet* animSet, cstr initialSequence,
                        Vec2 position, Vec2 scale, CaricType type) {
    if (caric == NULL) return;
    memset(caric, 0, sizeof(Caricature));

    caric->id = nextCaricId++;
    caric->name = name;
    caric->bitmap = NULL;
    caric->animSet = animSet;
    AnimState_Init(&caric->animState);
    caric->position = position;
    caric->scale = scale;
    caric->origin = (Vec2){DIV2, DIV2};
    caric->rotation = 0.0f;
    caric->drawMode = kDrawModeCopy;
    caric->isAnimated = true;
    caric->isVisible = true;
    caric->type = type;
    caric->flipX = false;
    caric->flipY = false;

    if (animSet == NULL) {
        printd(DEBUG_CARIC, "Caric_InitAnimated: '%s' initialized with no animation set", name ? name : "?");
    } else if (initialSequence != NULL) {
        AnimState_Play(&caric->animState, caric->animSet, initialSequence);
    }
}

void Caric_Update(Caricature* caric, f32 deltaTime) {
    if (caric == NULL || !caric->isAnimated) return;
    AnimState_Update(&caric->animState, caric->animSet, deltaTime);
}

LCDBitmap* Caric_GetCurrentBitmap(Caricature* caric) {
    if (caric == NULL) return NULL;
    return caric->isAnimated ? AnimState_GetCurrentBitmap(&caric->animState, caric->animSet) : caric->bitmap;
}

void Caric_Draw(Caricature* caric) {
    if (caric == NULL || pd == NULL || !caric->isVisible) return;

    LCDBitmap* frame = Caric_GetCurrentBitmap(caric);
    if (frame == NULL) {
        printd(DEBUG_CARIC, "Caric_Draw: '%s' has no bitmap to draw", caric->name ? caric->name : "?");
        return;
    }

    pd->graphics->setDrawMode(caric->drawMode);

    Vec2 offset = caric->isAnimated
                  ? AnimState_GetCurrentOffset(&caric->animState, caric->animSet)
                  : (Vec2){0.0f, 0.0f};
    int drawX = (int)caric->position.x + (int)(caric->flipX ? -offset.x : offset.x);
    int drawY = (int)caric->position.y + (int)offset.y;

    f32 xScale = caric->flipX ? -caric->scale.x : caric->scale.x;
    f32 yScale = caric->flipY ? -caric->scale.y : caric->scale.y;

    pd->graphics->drawRotatedBitmap(frame, drawX, drawY,
                                    caric->rotation, caric->origin.x, caric->origin.y,
                                    xScale, yScale);

    pd->graphics->setDrawMode(kDrawModeCopy);
}

void Caric_SetPosition(Caricature* caric, Vec2 newPosition) {
    if (caric == NULL) return;
    caric->position = newPosition;
}

void Caric_SetScale(Caricature* caric, Vec2 newScale) {
    if (caric == NULL) return;
    caric->scale = newScale;
}

void Caric_SetVisible(Caricature* caric, bool isVisible) {
    if (caric == NULL) return;
    caric->isVisible = isVisible;
}

Vec2 Caric_GetPosition(Caricature* caric) {
    return caric ? caric->position : (Vec2){0.0f, 0.0f};
}

Vec2 Caric_GetScale(Caricature* caric) {
    return caric ? caric->scale : (Vec2){0.0f, 0.0f};
}

Vec2 Caric_GetOrigin(Caricature* caric) {
    return caric ? caric->origin : (Vec2){0.0f, 0.0f};
}

void Caric_SetDrawMode(Caricature* caric, LCDBitmapDrawMode mode) {
    if (caric == NULL) return;
    caric->drawMode = mode;
}

void Caric_SetFlipX(Caricature* caric, bool flip) {
    if (caric == NULL) return;
    caric->flipX = flip;
}

void Caric_SetFlipY(Caricature* caric, bool flip) {
    if (caric == NULL) return;
    caric->flipY = flip;
}

bool Caric_PlayAnimation(Caricature* caric, cstr sequenceName) {
    if (caric == NULL || !caric->isAnimated) return false;
    return AnimState_Play(&caric->animState, caric->animSet, sequenceName);
}

void Caric_StartAnimation(Caricature* caric) {
    if (caric == NULL) return;
    AnimState_Resume(&caric->animState);
}

void Caric_StopAnimation(Caricature* caric) {
    if (caric == NULL) return;
    AnimState_Pause(&caric->animState);
}

void Caric_ResetAnimation(Caricature* caric) {
    if (caric == NULL) return;
    AnimState_Restart(&caric->animState);
}

void Caric_SetFrame(Caricature* caric, u8 frame) {
    if (caric == NULL) return;
    AnimState_SetFrame(&caric->animState, caric->animSet, frame);
}

u8 Caric_GetFrame(Caricature* caric) {
    return caric ? caric->animState.framePos : 0;
}

u8 Caric_GetFrameCount(Caricature* caric) {
    return caric ? AnimState_GetFrameCount(&caric->animState, caric->animSet) : 0;
}

bool Caric_IsAnimationDone(Caricature* caric) {
    return caric ? AnimState_IsDone(&caric->animState) : false;
}

cstr Caric_GetAnimationName(Caricature* caric) {
    return caric ? AnimState_GetCurrentName(&caric->animState, caric->animSet) : "";
}

CaricType Caric_GetType(Caricature* caric) {
    return caric ? caric->type : CARIC_TYPE_NULL;
}

bool Caric_IsVisible(Caricature* caric) {
    return caric ? caric->isVisible : false;
}

void Caric_SetOrigin(Caricature* caric, Vec2 newOrigin) {
    if (caric == NULL) return;
    caric->origin = newOrigin;
}