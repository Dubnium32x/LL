// written by diskodev
// game/screens/init.c
#include "init.h"

extern PlaydateAPI* pd;

static cstr loadingMessage = "Loading...";
static f32 loadProgress = 0.0f;

void InitScreen_Init(void) {
    loadingMessage = "Loading...";
    loadProgress = 0.0f;

    pd->graphics->setBackgroundColor(kColorBlack);

    pd->system->logToConsole("InitScreen: Initializing...");
}

void InitScreen_Update(f32 deltaTime) {
    // Simulate loading progress for demonstration purposes
    loadProgress += deltaTime * 0.5f; // Adjust speed as needed
    if (loadProgress > 1.0f) loadProgress = 1.0f;

    if (loadProgress >= 1.0f) {
        pd->system->logToConsole("InitScreen: Complete, transitioning to Splash");
        SwitchScreen(&screenManager, SS_SPLASH);
    }
}

void InitScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);

    // Draw loading message
    DrawText(loadingMessage, 348, 228, 2, kColorWhite);
}

void InitScreen_Unload(void) {
    pd->system->logToConsole("InitScreen: Unloading...");
}
