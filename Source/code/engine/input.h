// written by diskodev
// engine/input.h
#pragma once

#include "util.h"

#define MAX_BUTTONS 16

#define DOUBLE_TAP_WINDOW 0.25f

#define INPUT_MASK(bit) ((u16)(1u << (bit)))

typedef u16 InputState;

typedef enum {
    INPUT_A,
    INPUT_B,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,

    INPUT_CRANK_UP,
    INPUT_CRANK_DOWN,
    INPUT_CRANK_UNDOCKED,

    INPUT_COUNT
} InputBit;

typedef struct {
    InputState prevState;
    InputState curState;
    InputState pressedMask;
    InputState releasedMask;
    f32 holdTimers[MAX_BUTTONS];
    f32 lastPressedTime[MAX_BUTTONS];
    f32 totalTime;
    f32 crankAngle;
    f32 crankChange;
} InputManager;

extern InputManager inputManager;

void Input_InitManager(InputManager* manager);
void Input_UpdateManager(InputManager* manager, f32 deltaTime);
bool Input_IsDown(InputManager* manager, InputBit bit);
bool Input_IsPressed(InputManager* manager, InputBit bit);
bool Input_IsReleased(InputManager* manager, InputBit bit);
f32 Input_GetHoldTime(InputManager* manager, InputBit bit);
f32 Input_GetCrankAngle(InputManager* manager);
f32 Input_GetCrankChange(InputManager* manager);
bool Input_IsCrankDocked(InputManager* manager);
f32 Input_GetLastPressedTime(InputManager* manager, InputBit bit);
