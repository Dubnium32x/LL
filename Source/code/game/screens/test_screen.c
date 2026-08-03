// written by diskodev
// game/screens/test_screen.c
#include "test_screen.h"

#include <engine/engine_core.h>

extern PlaydateAPI* pd;

static f32 testX = SCR_W * 0.5f;
static f32 testY = SCR_H * 0.5f;
static f32 speed = 120.0f;
static u8 fadeMode = 0;

static void StepAudioVolumes(void) {
    if (Input_IsPressed(&inputManager, INPUT_UP)) {
        u8 v = GetMasterVol(&audioManager);
        SetMasterVol(&audioManager, (u8)MinU(v + 5, 100));
    }

    if (Input_IsPressed(&inputManager, INPUT_DOWN)) {
        u8 v = GetMasterVol(&audioManager);
        SetMasterVol(&audioManager, (u8)MaxI((i32)v - 5, 0));
    }

    if (Input_IsPressed(&inputManager, INPUT_LEFT)) {
        u8 v = GetSFXVol(&audioManager);
        SetSFXVol(&audioManager, (u8)MaxI((i32)v - 5, 0));
    }

    if (Input_IsPressed(&inputManager, INPUT_RIGHT)) {
        u8 v = GetSFXVol(&audioManager);
        SetSFXVol(&audioManager, (u8)MinU(v + 5, 100));
    }
}

void TestScreen_Init(void) {
    testX = SCR_W * 0.5f;
    testY = SCR_H * 0.5f;
    speed = 120.0f;
    fadeMode = 0;
    pd->system->logToConsole("TestScreen: init");
}

void TestScreen_Update(f32 deltaTime) {
    if (Input_IsDown(&inputManager, INPUT_A)) {
        speed = 220.0f;
    } else {
        speed = 120.0f;
    }

    f32 dx = 0.0f;
    f32 dy = 0.0f;

    if (Input_IsDown(&inputManager, INPUT_LEFT)) dx -= 1.0f;
    if (Input_IsDown(&inputManager, INPUT_RIGHT)) dx += 1.0f;
    if (Input_IsDown(&inputManager, INPUT_UP)) dy -= 1.0f;
    if (Input_IsDown(&inputManager, INPUT_DOWN)) dy += 1.0f;

    testX += dx * speed * deltaTime;
    testY += dy * speed * deltaTime;

    testX = Clamp(testX, 8.0f, (f32)(SCR_W - 8));
    testY = Clamp(testY, 8.0f, (f32)(SCR_H - 8));

    StepAudioVolumes();

    if (Input_IsPressed(&inputManager, INPUT_B)) {
        fadeMode = (u8)((fadeMode + 1) % 4);
        if (fadeMode == 0) Visual_FadeInBlack(&visualManager, 0.4f);
        if (fadeMode == 1) Visual_FadeOutBlack(&visualManager, 0.4f);
        if (fadeMode == 2) Visual_FadeInWhite(&visualManager, 0.4f);
        if (fadeMode == 3) Visual_FadeOutWhite(&visualManager, 0.4f);
    }
}

void TestScreen_Draw(void) {
    pd->graphics->clear(kColorWhite);

    pd->graphics->fillRect((i32)testX - 8, (i32)testY - 8, 16, 16, kColorBlack);

    char line1[96];
    char line2[96];
    char line3[96];

    snprintf(line1, sizeof(line1), "Engine Test (SS_TEST_1)");
    snprintf(line2, sizeof(line2), "A=run  B=cycle fades  DPad=move");
    pd->graphics->drawText(line1, strlen(line1), kASCIIEncoding, 12, 18);
    pd->graphics->drawText(line2, strlen(line2), kASCIIEncoding, 12, 34);
}

void TestScreen_Unload(void) {
    pd->system->logToConsole("TestScreen: unload");
}
