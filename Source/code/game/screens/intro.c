// written by diskodev
// game/screens/intro.c
#include "intro.h"
#include <engine/engine_core.h>

extern PlaydateAPI* pd;

static FMVPlayer introFMV;
static bool hasFinished = false;
static bool buttonPressed = false;

void IntroScreen_Init(void) {
    pd->system->logToConsole("IntroScreen: Init");
    hasFinished = false;
    buttonPressed = false;
    
    // Initialize FMV with the intro animation
    FMV_Init(&introFMV, "assets/fmv/10 sec 2D Test animation", true);
    FMV_Play(&introFMV);
}

void IntroScreen_Update(f32 deltaTime) {
    FMV_Update(&introFMV, deltaTime);
    if (FMV_IsFinished(&introFMV)) {
        hasFinished = true;
        pd->system->logToConsole("IntroScreen: FMV finished, transitioning to Title");
        SwitchScreen(&screenManager, SS_TITLE);
    }
    else if ((Input_IsDown(&inputManager, INPUT_A) || Input_IsDown(&inputManager, INPUT_B)) && !buttonPressed) {
        // Skip the intro if A or B is pressed
        FMV_Stop(&introFMV);
        hasFinished = true;
        buttonPressed = true;
        pd->system->logToConsole("IntroScreen: FMV skipped, transitioning to Title");
        SwitchScreen(&screenManager, SS_TITLE);
    }
}

void IntroScreen_Draw(void) {
    pd->graphics->clear(kColorWhite);
    FMV_Draw(&introFMV);

    // draw a "Press A or B to skip" message at the bottom of the screen
    cstr skipMessage = "Press A or B to skip";

    DrawText(skipMessage, 280, 220, 4, kColorBlack);
}

void IntroScreen_Unload(void) {
    FMV_Free(&introFMV);
}
