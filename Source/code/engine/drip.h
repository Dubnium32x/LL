// written by diskodev
// engine/drip.h
#pragma once
#include "util.h"

#define DRIP_MAX 48

typedef struct {
    f32  x;
    f32  tipY;      // leading edge (falls downward)
    f32  tailY;     // top of trail (stays fixed after drip spawns)
    f32  speed;
    i32  width;     // 1..3
    f32  stopY;     // y at which drip stops (-1 = bottom of screen)
    bool active;
    bool stopped;
} Drip;

void Drip_Init(void);
void Drip_Clear(void);
void Drip_Spawn(f32 x, f32 stopY);   // stopY < 0 uses SCR_H
void Drip_SpawnRandom(i32 count, f32 minStopY, f32 maxStopY);
void Drip_Update(f32 deltaTime);
void Drip_Draw(void);
