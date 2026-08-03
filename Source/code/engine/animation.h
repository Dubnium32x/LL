// written by diskodev
// engine/animation.h
#pragma once

#include "util.h"

#define MAX_ANIM_NAME_LENGTH 64
#define MAX_ANIM_FRAMES 128
#define MAX_ANIM_SEQUENCES 64

typedef enum {
    ANIM_LOOP,          // repeats forever
    ANIM_ONCE,          // plays once then freezes on the last frame
    ANIM_PINGPONG,      // plays forward then backward, forever
    ANIM_REVERSE_LOOP,  // loops backward
    ANIM_REVERSE_ONCE   // plays backward once then freezes on the first frame
} AnimPlayback;

typedef struct {
    u16 frameIndex; // index into the AnimationSet's bitmap table
    f32 duration;   // seconds to hold this frame
    i16 offsetX;    // per-frame draw offset, useful for hitbox alignment
    i16 offsetY;
} AnimFrame;

typedef struct {
    char name[MAX_ANIM_NAME_LENGTH];
    AnimPlayback playback;
    AnimFrame frames[MAX_ANIM_FRAMES];
    u8 frameCount;
} AnimSequence;

// Shared, per character/type animation data: one bitmap table plus the
// sequence library. Load once and reference from many Caricature instances
// rather than duplicating the table and sequences per entity.
typedef struct {
    LCDBitmapTable* bitmapTable;
    AnimSequence sequences[MAX_ANIM_SEQUENCES];
    u8 sequenceCount;
} AnimationSet;

// Tiny per-instance playback cursor into a shared AnimationSet.
typedef struct {
    i8 currentIndex; // -1 = nothing playing
    u8 framePos;
    f32 elapsed;
    i8 direction;    // +1 forward, -1 backward (pingpong)
    bool done;       // true once an ANIM_ONCE/ANIM_REVERSE_ONCE sequence finishes
    bool paused;
} AnimationState;

void AnimSet_Init(AnimationSet* set);
bool AnimSet_LoadTable(AnimationSet* set, cstr path);
void AnimSet_Free(AnimationSet* set);
void AnimSet_AddSequence(AnimationSet* set, const AnimSequence* seq);

void AnimState_Init(AnimationState* state);
bool AnimState_Play(AnimationState* state, const AnimationSet* set, cstr name);
void AnimState_Restart(AnimationState* state);
void AnimState_Pause(AnimationState* state);
void AnimState_Resume(AnimationState* state);
void AnimState_SetFrame(AnimationState* state, const AnimationSet* set, u8 frame);

void AnimState_Update(AnimationState* state, const AnimationSet* set, f32 deltaTime);

LCDBitmap* AnimState_GetCurrentBitmap(const AnimationState* state, const AnimationSet* set);
Vec2 AnimState_GetCurrentOffset(const AnimationState* state, const AnimationSet* set);
bool AnimState_IsDone(const AnimationState* state);
cstr AnimState_GetCurrentName(const AnimationState* state, const AnimationSet* set);
u8 AnimState_GetFrameCount(const AnimationState* state, const AnimationSet* set);
