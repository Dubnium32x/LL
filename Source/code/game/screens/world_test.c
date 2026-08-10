// written by diskodev
// game/screens/world_test.c
#include "world_test.h"
#include <engine/engine_core.h>
#include <game/data/world.h>

extern PlaydateAPI* pd;

static i32 camX = 0;
static i32 camY = 0;

void WorldTestScreen_Init(void) {
    pd->system->logToConsole("WorldTestScreen: Init");

    camX = 0;
    camY = 0;

    // Load tileset bitmap table
    // Playdate compiler converts BASE_TILES-table-16-16.png -> BASE_TILES.pdt
    cstr tilesetPath = "assets/texture/tileset/BASE_TILES";
    World_Init(pd, &gWorld, tilesetPath);

    // Prepare paths for each layer
    // room0.csv will be loaded on LAYER_BG (or LAYER_CENTER)
    cstr layerPaths[LAYER_COUNT] = {
        "assets/data/csv/rooms/room0.csv", // LAYER_BG
        NULL,                              // LAYER_CENTER
        NULL,                              // LAYER_OBJECTS
        NULL,                              // LAYER_FG
        NULL                               // LAYER_COLLISION
    };

    if (World_LoadCSV(pd, &gWorld, layerPaths)) {
        pd->system->logToConsole("WorldTestScreen: successfully loaded room0.csv!");
    } else {
        pd->system->logToConsole("WorldTestScreen: failed to load room0.csv");
    }
}

void WorldTestScreen_Update(f32 deltaTime) {
    i32 moveSpeed = 60;
    if (Input_IsDown(&inputManager, INPUT_A)) moveSpeed = 150;

    if (Input_IsDown(&inputManager, INPUT_LEFT))  camX -= (i32)(moveSpeed * deltaTime);
    if (Input_IsDown(&inputManager, INPUT_RIGHT)) camX += (i32)(moveSpeed * deltaTime);
    if (Input_IsDown(&inputManager, INPUT_UP))    camY -= (i32)(moveSpeed * deltaTime);
    if (Input_IsDown(&inputManager, INPUT_DOWN))  camY += (i32)(moveSpeed * deltaTime);

    // Switch screens if needed
    if (Input_IsPressed(&inputManager, INPUT_B)) {
        SwitchScreen(&screenManager, SS_TEST_2);
    }
}

void WorldTestScreen_Draw(void) {
    // Set clear color or background pattern if needed
    pd->graphics->clear(kColorWhite);

    // Draw tilemap world layer
    World_Draw(pd, &gWorld, camX, camY);

    // Draw UI overlay using XOR draw mode so text is visible over white or black tiles
    pd->graphics->setDrawMode(kDrawModeXOR);
    char info1[64];
    char info2[64];
    snprintf(info1, sizeof(info1), "World CSV Test (room0.csv)");
    snprintf(info2, sizeof(info2), "DPad=Cam (%d,%d) A=Fast B=Test2", camX, camY);
    DrawText(info1, 10, 10, 4, kColorBlack);
    DrawText(info2, 10, 30, 4, kColorBlack);
    // Reset draw mode
    pd->graphics->setDrawMode(kDrawModeCopy);
}

void WorldTestScreen_Unload(void) {
    pd->system->logToConsole("WorldTestScreen: Unload");
    World_Free(pd, &gWorld);
}
