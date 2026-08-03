// written by diskodev
// engine/input.c
#include "input.h"

InputManager inputManager = {0};

void Input_InitManager(InputManager* manager) {
    if (manager == NULL || pd == NULL) return;
    memset(manager, 0, sizeof(InputManager));
    for (int i = 0; i < MAX_BUTTONS; i++) {
        manager->lastPressedTime[i] = -1.0f;
    }
}

void Input_UpdateManager(InputManager* manager, f32 deltaTime) {
    if (manager == NULL || pd == NULL) return;

    manager->totalTime += deltaTime;

    PDButtons current, pushed, released;
    pd->system->getButtonState(&current, &pushed, &released);

    InputState newState = 0;
    if (current & kButtonA) newState |= INPUT_MASK(INPUT_A);
    if (current & kButtonB) newState |= INPUT_MASK(INPUT_B);
    if (current & kButtonUp) newState |= INPUT_MASK(INPUT_UP);
    if (current & kButtonDown) newState |= INPUT_MASK(INPUT_DOWN);
    if (current & kButtonLeft) newState |= INPUT_MASK(INPUT_LEFT);
    if (current & kButtonRight) newState |= INPUT_MASK(INPUT_RIGHT);

    manager->prevState = manager->curState;
    manager->curState = newState;

    f32 currentCrankAngle = pd->system->getCrankAngle();
    manager->crankChange = currentCrankAngle - manager->crankAngle;
    manager->crankAngle = currentCrankAngle;

    if (manager->crankChange > 180.0f) manager->crankChange -= 360.0f;
    if (manager->crankChange < -180.0f) manager->crankChange += 360.0f;

    if (!pd->system->isCrankDocked()) {
        manager->curState |= INPUT_MASK(INPUT_CRANK_UNDOCKED);
        if (manager->crankChange < -5.0f) {
            manager->curState |= INPUT_MASK(INPUT_CRANK_DOWN);
        } else if (manager->crankChange > 5.0f) {
            manager->curState |= INPUT_MASK(INPUT_CRANK_UP);
        }
    }

    manager->pressedMask = (manager->curState & ~manager->prevState);
    manager->releasedMask = (~manager->curState & manager->prevState);

    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (manager->curState & INPUT_MASK(i)) {
            manager->holdTimers[i] += deltaTime;
        } else {
            manager->holdTimers[i] = 0.0f;
        }
        if (manager->pressedMask & INPUT_MASK(i)) {
            manager->lastPressedTime[i] = manager->totalTime;
        }
    }
}

bool Input_IsDown(InputManager* manager, InputBit bit) {
    if (manager == NULL) return false;
    return (manager->curState & INPUT_MASK(bit)) != 0;
}

bool Input_IsPressed(InputManager* manager, InputBit bit) {
    if (manager == NULL) return false;
    return (manager->pressedMask & INPUT_MASK(bit)) != 0;
}

bool Input_IsReleased(InputManager* manager, InputBit bit) {
    if (manager == NULL) return false;
    return (manager->releasedMask & INPUT_MASK(bit)) != 0;
}

f32 Input_GetHoldTime(InputManager* manager, InputBit bit) {
    if (manager == NULL || bit >= MAX_BUTTONS) return 0.0f;
    return manager->holdTimers[bit];
}

f32 Input_GetCrankAngle(InputManager* manager) {
    if (manager == NULL) return 0.0f;
    return manager->crankAngle;
}

f32 Input_GetCrankChange(InputManager* manager) {
    if (manager == NULL) return 0.0f;
    return manager->crankChange;
}

bool Input_IsCrankDocked(InputManager* manager) {
    if (manager == NULL || pd == NULL) return true;
    return pd->system->isCrankDocked();
}

f32 Input_GetLastPressedTime(InputManager* manager, InputBit bit) {
    if (manager == NULL || bit >= MAX_BUTTONS) return 0.0f;
    return manager->lastPressedTime[bit];
}
