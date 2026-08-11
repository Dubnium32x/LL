/*
    LINCOLN'S LABYRINTHINE

    Created using the Playdate SDK version 3.1.1 as of August 2026.
    Also created with C23 language standard version.

    Code is licensed under the MIT License.
*/

#include "pd_api.h"
#include <engine/engine_core.h>
#include <engine/menu_image.h>
#include <game/data/data.h>
#include <game/screens/test_screen.h>
#include <game/screens/cutscene.h>
#include <game/screens/test_screen2.h>
#include <game/screens/world_test.h>
#include <game/screens/dialogue_test.h>
#include <game/screens/phys_test.h>
#include <game/screens/phys_test2.h>
#include <game/screens/init.h>
#include <game/screens/splash.h>
#include <game/screens/intro.h>
#include <game/screens/title.h>
#include <game/screens/title.h>
#include <game/screens/light_test.h>

PlaydateAPI* pd = NULL;
LCDFont* fontFamily[5] = {0};

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

static void DrawMenuImageGlitch(LCDBitmap* canvas) {
    (void)canvas;
    LCDBitmap* lmao = Asset_LoadBitmap("assets/texture/image/LMAO");
    if (lmao) pd->graphics->drawBitmap(lmao, 0, 0, kBitmapUnflipped);
}

static void DrawMenuImageAreYouOkay(LCDBitmap* canvas) {
    (void)canvas;
    pd->graphics->fillRect(0, 0, 200, 240, kColorBlack);

    pd->graphics->setDrawMode(kDrawModeFillWhite);
    LCDFont* font = fontFamily[4];
    pd->graphics->setFont(font);
    const char* msg = "Are you okay?";
    int w = pd->graphics->getTextWidth(font, msg, __builtin_strlen(msg), kASCIIEncoding, 0);
    int x = (200 - w) / 2;
    if (x < 4) x = 4;
    pd->graphics->drawText(msg, __builtin_strlen(msg), kASCIIEncoding, x, 110);
}

static void DrawMenuImage(LCDBitmap* canvas) {
    (void)canvas;
    pd->graphics->fillRect(0, 0, 200, 240, kColorBlack);

    pd->graphics->setDrawMode(kDrawModeFillWhite);
    pd->graphics->setFont(fontFamily[4]);
    pd->graphics->drawText("PAUSED", 6, kASCIIEncoding, 10, 10);
    // pd->graphics->drawLine(10, 28, 190, 28, 1, kColorWhite);

    pd->graphics->setFont(fontFamily[1]);

    PlayerData* p = &playerData;
    char* buf = NULL; i32 bufSize = 0;
    i32 y = 38;
    const i32 step = 16;

    #define DRAW(fmt, ...) \
        pd->system->formatString(&buf, fmt, ##__VA_ARGS__); \
        pd->graphics->drawText(buf, __builtin_strlen(buf), kASCIIEncoding, 10, y); \
        pd->system->realloc(buf, 0); buf = NULL; y += step;

    DRAW("Area:  %s", AreaType_Name(p->currentArea))
    DRAW("Room:  %d",  p->currentRoom)
    DRAW("Notes: %d / %d", PlayerData_NoteCount(p), MAX_NOTES)

    #undef DRAW

    pd->graphics->drawLine(10, y + 2, 190, y + 2, 1, kColorWhite); y += 12;

    pd->graphics->drawText("Items:", 6, kASCIIEncoding, 10, y); y += step;
    for (int i = 0; i < 3; i++) {
        ItemType item = p->currentItems[i] ? *p->currentItems[i] : ITEM_NONE;
        const char* name = ItemType_Name(item);
        pd->graphics->drawText(name, __builtin_strlen(name), kASCIIEncoding, 20, y);
        y += step;
    }
}

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
    if (event == kEventPause) {
        bool onPhysTest = GetCurrentScreenType(&screenManager) == SS_TEST_4;
        MenuImageDrawFn drawFn = DrawMenuImage;
        if (onPhysTest && PhysTestScreen_IsSanityGlitchActive()) {
            drawFn = DrawMenuImageGlitch;
        } else if (onPhysTest && PhysTestScreen_IsSanityLow() && (rand() % 100) < 10) {
            // "Are you okay?" — 10% chance per pause, only while sanity is low.
            drawFn = DrawMenuImageAreYouOkay;
        }
        MenuImage_Set(drawFn, 0);
    }
    if (event == kEventInit) {
        pd = playdate;

        // Without this, rand() starts from the same default seed every launch, so every
        // "random" pick (sanity glitch selection, chance rolls, etc.) replays identically
        // from one playthrough to the next.
        unsigned int seedMs = 0;
        unsigned int seedSec = pd->system->getSecondsSinceEpoch(&seedMs);
        srand(seedSec ^ seedMs);

        Asset_InitManager();
        InitFonts();
        InitAudio();
        InitInput();
        SaveData_Init(&saveData);
        
        Visual_InitManager(&visualManager);
        Drip_Init();
        ScreenManager_Init(&screenManager);

        // RegisterScreen(&screenManager,
        //                SS_TEST_1,
        //                TestScreen_Init,
        //                TestScreen_Update,
        //                TestScreen_Draw,
        //                TestScreen_Unload);

        // RegisterScreen(&screenManager,
        //                SS_CUTSCENE,
        //                CutsceneScreen_Init,
        //                CutsceneScreen_Update,
        //                CutsceneScreen_Draw,
        //                CutsceneScreen_Unload);

        // RegisterScreen(&screenManager,
        //                SS_TEST_2,
        //                TestScreen2_Init,
        //                TestScreen2_Update,
        //                TestScreen2_Draw,
        //                TestScreen2_Unload);

        RegisterScreen(&screenManager,
                       SS_TEST_WORLD,
                       DialogueTestScreen_Init,
                       DialogueTestScreen_Update,
                       DialogueTestScreen_Draw,
                       DialogueTestScreen_Unload);
        RegisterScreen(&screenManager,
                       SS_TEST_4,
                       PhysTestScreen_Init,
                       PhysTestScreen_Update,
                       PhysTestScreen_Draw,
                       PhysTestScreen_Unload);
        RegisterScreen(&screenManager,
                       SS_TEST_5,
                       PhysTest2Screen_Init,
                       PhysTest2Screen_Update,
                       PhysTest2Screen_Draw,
                       PhysTest2Screen_Unload);

        RegisterScreen(&screenManager,
                       SS_TEST_6,
                       LightTestScreen_Init,
                       LightTestScreen_Update,
                       LightTestScreen_Draw,
                       LightTestScreen_Unload); 
        RegisterScreen(&screenManager,
                       SS_INIT,
                       InitScreen_Init,
                       InitScreen_Update,
                       InitScreen_Draw,
                       InitScreen_Unload);

        RegisterScreen(&screenManager,
                       SS_SPLASH,
                       SplashScreen_Init,
                       SplashScreen_Update,
                       SplashScreen_Draw,
                       SplashScreen_Unload);
        RegisterScreen(&screenManager,
                       SS_INTRO,
                       IntroScreen_Init,
                       IntroScreen_Update,
                       IntroScreen_Draw,
                       IntroScreen_Unload);

        RegisterScreen(&screenManager,
                       SS_TITLE,
                       TitleScreen_Init,
                       TitleScreen_Update,
                       TitleScreen_Draw,
                       TitleScreen_Unload);

        SwitchScreen(&screenManager, SS_TEST_4);

        pd->system->setUpdateCallback(Update, NULL);
    }
    return 0;
}
