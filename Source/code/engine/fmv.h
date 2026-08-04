// written by diskodev
// engine/fmv.h
#pragma once

#include "util.h"

typedef struct {
    LCDVideoPlayer* player;
    int currentFrame;
    int totalFrames;
    int width;
    int height;
    f32 framerate;
    bool isPlaying;
    bool isLooping;
    f32 frameAccumulator;
    cstr videoPath;
} FMVPlayer;

extern PlaydateAPI* pd;

void FMV_Init(FMVPlayer* fmv, cstr videoPath, bool loop);
void FMV_Update(FMVPlayer* fmv, f32 deltaTime);
void FMV_Draw(FMVPlayer* fmv);
void FMV_Free(FMVPlayer* fmv);
void FMV_Play(FMVPlayer* fmv);
void FMV_Pause(FMVPlayer* fmv);
void FMV_Stop(FMVPlayer* fmv);
void FMV_Seek(FMVPlayer* fmv, int frame);
bool FMV_IsFinished(FMVPlayer* fmv);
