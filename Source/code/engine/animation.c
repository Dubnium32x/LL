// written by diskodev
// engine/animation.c
#include "animation.h"

static const AnimSequence* CurrentSequence(const AnimationState* state, const AnimationSet* set) {
    if (state == NULL || set == NULL) return NULL;
    if (state->currentIndex < 0 || state->currentIndex >= set->sequenceCount) return NULL;
    return &set->sequences[state->currentIndex];
}

static i8 FindSequenceIndex(const AnimationSet* set, cstr name) {
    for (u8 i = 0; i < set->sequenceCount; i++) {
        if (String_Compare(set->sequences[i].name, name) == 0) return (i8)i;
    }
    return -1;
}

// ---- AnimationSet: shared library + bitmap table ----

void AnimSet_Init(AnimationSet* set) {
    if (set == NULL) return;
    memset(set, 0, sizeof(AnimationSet));
}

bool AnimSet_LoadTable(AnimationSet* set, cstr path) {
    if (set == NULL || pd == NULL || path == NULL) return false;

    if (set->bitmapTable != NULL) {
        pd->graphics->freeBitmapTable(set->bitmapTable);
        set->bitmapTable = NULL;
    }

    const char* err = NULL;
    set->bitmapTable = pd->graphics->loadBitmapTable(path, &err);
    if (set->bitmapTable == NULL) {
        printd(DEBUG_ANIM, "AnimSet_LoadTable: failed to load '%s': %s", path, err ? err : "unknown error");
        return false;
    }
    return true;
}

void AnimSet_Free(AnimationSet* set) {
    if (set == NULL || pd == NULL) return;
    if (set->bitmapTable != NULL) {
        pd->graphics->freeBitmapTable(set->bitmapTable);
        set->bitmapTable = NULL;
    }
}

void AnimSet_AddSequence(AnimationSet* set, const AnimSequence* seq) {
    if (set == NULL || seq == NULL) return;

    if (FindSequenceIndex(set, seq->name) >= 0) return;

    if (set->sequenceCount >= MAX_ANIM_SEQUENCES) {
        printd(DEBUG_ANIM, "AnimSet_AddSequence: sequence library full, dropping '%s'", seq->name);
        return;
    }

    set->sequences[set->sequenceCount++] = *seq;
}

// ---- AnimationState: per-instance playback cursor ----

void AnimState_Init(AnimationState* state) {
    if (state == NULL) return;
    memset(state, 0, sizeof(AnimationState));
    state->currentIndex = -1;
    state->direction = 1;
}

bool AnimState_Play(AnimationState* state, const AnimationSet* set, cstr name) {
    if (state == NULL || set == NULL || name == NULL) return false;

    i8 idx = FindSequenceIndex(set, name);
    if (idx < 0) {
        printd(DEBUG_ANIM, "AnimState_Play: sequence '%s' not found", name);
        return false;
    }

    if (idx == state->currentIndex) return true;

    state->currentIndex = idx;
    state->framePos = 0;
    state->elapsed = 0.0f;
    state->direction = 1;
    state->done = false;
    return true;
}

void AnimState_Restart(AnimationState* state) {
    if (state == NULL) return;
    state->framePos = 0;
    state->elapsed = 0.0f;
    state->direction = 1;
    state->done = false;
}

void AnimState_Pause(AnimationState* state) {
    if (state == NULL) return;
    state->paused = true;
}

void AnimState_Resume(AnimationState* state) {
    if (state == NULL) return;
    state->paused = false;
}

void AnimState_SetFrame(AnimationState* state, const AnimationSet* set, u8 frame) {
    const AnimSequence* seq = CurrentSequence(state, set);
    if (seq == NULL || seq->frameCount == 0) return;
    state->framePos = frame % seq->frameCount;
}

void AnimState_Update(AnimationState* state, const AnimationSet* set, f32 deltaTime) {
    const AnimSequence* seq = CurrentSequence(state, set);
    if (seq == NULL || seq->frameCount == 0) return;
    if (state->paused || state->done) return;

    state->elapsed += deltaTime;

    f32 frameDur = seq->frames[state->framePos].duration;
    if (frameDur <= 0.0f) frameDur = 0.1f;

    while (!state->done && state->elapsed >= frameDur) {
        state->elapsed -= frameDur;

        switch (seq->playback) {
            case ANIM_LOOP:
                state->framePos = (state->framePos + 1) % seq->frameCount;
                break;

            case ANIM_ONCE:
                if (state->framePos < seq->frameCount - 1) {
                    state->framePos++;
                } else {
                    state->done = true;
                }
                break;

            case ANIM_REVERSE_LOOP:
                state->framePos = (state->framePos == 0) ? seq->frameCount - 1 : state->framePos - 1;
                break;

            case ANIM_REVERSE_ONCE:
                if (state->framePos > 0) {
                    state->framePos--;
                } else {
                    state->done = true;
                }
                break;

            case ANIM_PINGPONG: {
                i16 next = (i16)state->framePos + state->direction;
                if (next >= seq->frameCount) {
                    state->direction = -1;
                    next = (i16)seq->frameCount - 2;
                    if (next < 0) next = 0;
                } else if (next < 0) {
                    state->direction = 1;
                    next = (seq->frameCount > 1) ? 1 : 0;
                }
                state->framePos = (u8)next;
                break;
            }
        }

        frameDur = seq->frames[state->framePos].duration;
        if (frameDur <= 0.0f) frameDur = 0.1f;
    }
}

// ---- query ----

LCDBitmap* AnimState_GetCurrentBitmap(const AnimationState* state, const AnimationSet* set) {
    if (pd == NULL || set == NULL || set->bitmapTable == NULL) return NULL;
    const AnimSequence* seq = CurrentSequence(state, set);
    if (seq == NULL || seq->frameCount == 0) return NULL;
    const AnimFrame* frame = &seq->frames[state->framePos];
    return pd->graphics->getTableBitmap(set->bitmapTable, frame->frameIndex);
}

Vec2 AnimState_GetCurrentOffset(const AnimationState* state, const AnimationSet* set) {
    const AnimSequence* seq = CurrentSequence(state, set);
    if (seq == NULL || seq->frameCount == 0) return (Vec2){0.0f, 0.0f};
    const AnimFrame* frame = &seq->frames[state->framePos];
    return (Vec2){(f32)frame->offsetX, (f32)frame->offsetY};
}

bool AnimState_IsDone(const AnimationState* state) {
    return state ? state->done : false;
}

cstr AnimState_GetCurrentName(const AnimationState* state, const AnimationSet* set) {
    const AnimSequence* seq = CurrentSequence(state, set);
    return seq ? seq->name : "";
}

u8 AnimState_GetFrameCount(const AnimationState* state, const AnimationSet* set) {
    const AnimSequence* seq = CurrentSequence(state, set);
    return seq ? seq->frameCount : 0;
}
