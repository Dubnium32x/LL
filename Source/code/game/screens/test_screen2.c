// written by diskodev
// game/screens/test_screen2.c
#include "test_screen2.h"

extern PlaydateAPI* pd;

static LCDBitmapTable* clawTable = NULL;
static i32 clawFrame = -1; // -1 means effect inactive
static f32 clawTimer = 0.0f;
static const f32 FRAME_DURATION = 0.06f; // ~16 fps animation for claw scratch

void TestScreen2_Init(void) {
    pd->system->logToConsole("TestScreen2: Init");
    
    // Load claw scratch bitmap table from correct spritesheet path
    cstr tablePath = "assets/texture/spritesheet/claw_scratch";
    cstr outErr = NULL;
    clawTable = pd->graphics->loadBitmapTable(tablePath, &outErr);
    if (!clawTable) {
        pd->system->logToConsole("Failed to load claw_scratch table: %s", outErr ? outErr : "unknown");
    } else {
        pd->system->logToConsole("Loaded claw_scratch bitmap table successfully from %s", tablePath);
    }
    
    clawFrame = -1;
    clawTimer = 0.0f;
}

void TestScreen2_Update(f32 deltaTime) {
    // Update visual manager (shake, fades)
    Visual_UpdateManager(&visualManager, deltaTime);
    
    // Press A: Trigger Claw Scratch Effect + Light Shake
    if (Input_IsPressed(&inputManager, INPUT_A)) {
        clawFrame = 0;
        clawTimer = 0.0f;
        Visual_StartShake(&visualManager, 6.0f, 0.4f);
    }
    
    // Press B: Trigger Heavy Screen Shake
    if (Input_IsPressed(&inputManager, INPUT_B)) {
        Visual_StartShake(&visualManager, 14.0f, 0.7f);
    }
    
    // Press UP: Switch back to Cutscene
    if (Input_IsPressed(&inputManager, INPUT_UP)) {
        SwitchScreen(&screenManager, SS_CUTSCENE);
    }
    
    // Update claw animation frame
    if (clawFrame >= 0 && clawTable) {
        clawTimer += deltaTime;
        if (clawTimer >= FRAME_DURATION) {
            clawTimer -= FRAME_DURATION;
            clawFrame++;
            // Check if animation finished
            LCDBitmap* nextFrame = pd->graphics->getTableBitmap(clawTable, clawFrame);
            if (!nextFrame) {
                clawFrame = -1; // End effect
            }
        }
    }
}

void TestScreen2_Draw(void) {
    // Apply screen shake offset before drawing scene elements
    Visual_ApplyShakeOffset(&visualManager);
    
    // Clear screen to black so white claw effect pops
    pd->graphics->clear(kColorBlack);
    
    // Draw background/test patterns in white for visibility on black
    pd->graphics->drawRect(20, 20, SCR_W - 40, SCR_H - 40, kColorWhite);
    pd->graphics->fillRect(180, 100, 40, 40, kColorWhite);
    
    // Draw Claw Scratch effect overlay if active
    if (clawFrame >= 0 && clawTable) {
        LCDBitmap* frameBmp = pd->graphics->getTableBitmap(clawTable, clawFrame);
        if (frameBmp) {
            pd->graphics->drawBitmap(frameBmp, 0, 0, kBitmapUnflipped);
        }
    }
    
    // Reset draw offset for UI text overlay
    Visual_ClearShakeOffset();
    
    // Set text draw mode so text is visible over black background
    pd->graphics->setDrawMode(kDrawModeFillWhite);
    
    // Draw UI overlay
    char info1[96];
    char info2[96];
    snprintf(info1, sizeof(info1), "Screen Shake & Claw Scratch Test (SS_TEST_2)");
    snprintf(info2, sizeof(info2), "A=Claw Scratch+Shake  B=Heavy Shake  UP=Cutscene");
    pd->graphics->drawText(info1, strlen(info1), kASCIIEncoding, 10, 10);
    pd->graphics->drawText(info2, strlen(info2), kASCIIEncoding, 10, 220);
    
    // Reset draw mode back to normal
    pd->graphics->setDrawMode(kDrawModeCopy);
    
    // Draw fade overlay if active
    Visual_DrawFade(&visualManager);
}

void TestScreen2_Unload(void) {
    pd->system->logToConsole("TestScreen2: Unload");
    if (clawTable) {
        pd->graphics->freeBitmapTable(clawTable);
        clawTable = NULL;
    }
    Visual_ClearShakeOffset();
}
