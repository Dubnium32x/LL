// written by diskodev
// game/screens/cutscene.c
#include "cutscene.h"

extern PlaydateAPI* pd;

static FMVPlayer cutsceneFMV;

void CutsceneScreen_Init(void) {
    pd->system->logToConsole("CutsceneScreen: Init");
    
    // Initialize FMV with the 10 sec test animation
    FMV_Init(&cutsceneFMV, "assets/fmv/10 sec 2D Test animation", true);
    FMV_Play(&cutsceneFMV);
}

void CutsceneScreen_Update(f32 deltaTime) {
    FMV_Update(&cutsceneFMV, deltaTime);
    
    // Press A to toggle play/pause
    if (Input_IsPressed(&inputManager, INPUT_A)) {
        if (cutsceneFMV.isPlaying) {
            FMV_Pause(&cutsceneFMV);
        } else {
            FMV_Play(&cutsceneFMV);
        }
    }
    
    // Press B to restart video
    if (Input_IsPressed(&inputManager, INPUT_B)) {
        FMV_Stop(&cutsceneFMV);
        FMV_Play(&cutsceneFMV);
    }
    
    // Press UP to switch to TestScreen2 (Screen Shake & Scratch Effect Test)
    if (Input_IsPressed(&inputManager, INPUT_UP)) {
        SwitchScreen(&screenManager, SS_TEST_2);
    }

    // Press RIGHT to seek forward 30 frames (1 second)
    if (Input_IsPressed(&inputManager, INPUT_RIGHT)) {
        FMV_Seek(&cutsceneFMV, cutsceneFMV.currentFrame + 30);
    }
    // Press LEFT to seek backward 30 frames
    if (Input_IsPressed(&inputManager, INPUT_LEFT)) {
        FMV_Seek(&cutsceneFMV, cutsceneFMV.currentFrame - 30);
    }
}

void CutsceneScreen_Draw(void) {
    pd->graphics->clear(kColorWhite);
    
    // Draw current video frame
    FMV_Draw(&cutsceneFMV);
    
    // Draw status overlay if paused
    if (!cutsceneFMV.isPlaying) {
        char statusStr[64];
        snprintf(statusStr, sizeof(statusStr), "PAUSED (%d/%d)", 
                 cutsceneFMV.currentFrame + 1, cutsceneFMV.totalFrames);
        pd->graphics->drawText(statusStr, strlen(statusStr), kASCIIEncoding, 10, 10);
    }
}

void CutsceneScreen_Unload(void) {
    pd->system->logToConsole("CutsceneScreen: Unload");
    FMV_Free(&cutsceneFMV);
}
