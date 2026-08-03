/*
    LINCOLN'S LABYRINTHINE

    Created using the Playdate SDK version 3.1.1 as of August 2026.
    Also created with C23 language standard version.

    Code is licensed under the MIT License.
*/

#include "pd_api.h"
#include <engine/engine_core.h>
#include <game/screens/test_screen.h>

PlaydateAPI* pd = NULL;
LCDFont* fontFamily[4] = {0};

static void InitFonts(void) {
    LoadFonts(pd);
}

static void InitAudio(void) {
    Audio_InitManager(&audioManager);
}

static void InitInput(void) {
    Input_InitManager(&inputManager);
}

static int Update(void* userdata) {
    (void)userdata;
    f32 deltaTime = 1.0f / 30.0f;
    
    Input_UpdateManager(&inputManager, deltaTime);
    Audio_UpdateManager(&audioManager, deltaTime);
    ScreenManager_Update(&screenManager, deltaTime);
    ScreenManager_Draw(&screenManager);
    
    return 1;
}

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
    if (event == kEventInit) {
        pd = playdate;
        
        InitFonts();
        InitAudio();
        InitInput();
        
        ScreenManager_Init(&screenManager);

        RegisterScreen(&screenManager,
                       SS_TEST_1,
                       TestScreen_Init,
                       TestScreen_Update,
                       TestScreen_Draw,
                       TestScreen_Unload);

        SwitchScreen(&screenManager, SS_TEST_1);

        pd->system->setUpdateCallback(Update, NULL);
    }
    return 0;
}