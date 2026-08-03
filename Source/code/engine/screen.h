// written by diskodev
// engine/screen.h
#pragma once

#include "edit/screen_state.h"
#include "visual.h"

typedef struct {
    void (*Init)(void);
    void (*Update)(f32 deltaTime);
    void (*Draw)(void);
    void (*Unload)(void);
} IScreen;

typedef struct {
    IScreen screens[MAX_REGISTERED_SCREENS];
    ScreenState currentScreen;
    ScreenState nextScreen;
    bool isPaused;
    bool isTransitioning;
} ScreenManager;

extern VisualManager visualManager;
extern ScreenManager screenManager;

void ScreenManager_Init(ScreenManager* manager);
void ScreenManager_Update(ScreenManager* manager, f32 deltaTime);
void ScreenManager_Draw(ScreenManager* manager);
void ScreenManager_Unload(ScreenManager* manager);

void RegisterScreen(ScreenManager* manager, ScreenState type,
                    void (*Init)(void),
                    void (*Update)(f32),
                    void (*Draw)(void),
                    void (*Unload)(void));

void SwitchScreen(ScreenManager* manager, ScreenState type);
ScreenState GetCurrentScreenType(ScreenManager* manager);
IScreen* GetActiveScreen(ScreenManager* manager);
