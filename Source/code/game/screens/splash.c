// written by diskodev
// game/screens/splash.c
#include "splash.h"

extern PlaydateAPI* pd;

typedef enum {
    SPLASH_STATE_FADING_IN,
    SPLASH_STATE_DISPLAY,
    SPLASH_STATE_FADING_OUT,
    SPLASH_STATE_FINISHED
} SplashState;

static SplashState currentState = SPLASH_STATE_FADING_IN;
static f32 stateTimer = 0.0f;
static const f32 FADE_DURATION = 1.5f;
static const f32 DISPLAY_DURATION = 5.5f;

static LCDBitmap* logoBitmap = NULL;
// static LCDBitmap* lincolnBitmap = NULL;
// static f32 lincolnX = 0.0f;
// static f32 lincolnSpeed = 80.0f; // pixels per second
// static bool lincolnVisible = false;

void SplashScreen_Init(void) {
    currentState = SPLASH_STATE_FADING_IN;
    Visual_StartFade(&visualManager, VISUAL_FADE_BLACK, VISUAL_FADE_IN, FADE_DURATION);

    stateTimer = 0.0f;
    
    pd->system->logToConsole("SplashScreen: Init");
    
    // Load logo using AssetManager
    logoBitmap = Asset_LoadBitmap("assets/texture/image/team_logo");
    if (!logoBitmap) {
        pd->system->logToConsole("SplashScreen: Logo not found");
    } else {
        pd->system->logToConsole("SplashScreen: Logo loaded");
    }
}

void SplashScreen_Update(f32 deltatime) {
    switch (currentState) {
        case SPLASH_STATE_FADING_IN:
            stateTimer += deltatime;
            if (stateTimer >= FADE_DURATION) {
                currentState = SPLASH_STATE_DISPLAY;
                stateTimer = 0.0f;
                // lincolnVisible = true; // Start Lincoln running
            }
            break;
            
        case SPLASH_STATE_DISPLAY:
            stateTimer += deltatime;
            
            if (stateTimer >= DISPLAY_DURATION) {
                currentState = SPLASH_STATE_FADING_OUT;
                stateTimer = 0.0f;
                // Start fade to black ONCE when entering FADING_OUT state
                Visual_StartFade(&visualManager, VISUAL_FADE_BLACK, VISUAL_FADE_OUT, FADE_DURATION);
            }
            break;
            
        case SPLASH_STATE_FADING_OUT:
            stateTimer += deltatime;
            if (stateTimer >= FADE_DURATION || !Visual_IsFadeActive(&visualManager)) {
                currentState = SPLASH_STATE_FINISHED;
                stateTimer = 0.0f;
            }
            break;
                
        case SPLASH_STATE_FINISHED:
            // Transition to next screen (e.g., Intro or Title)
            Visual_StopFade(&visualManager); // Ensure fade is stopped before switching
            SwitchScreen(&screenManager, SS_INTRO);
            break;
        default:
            break;
    }
}

void SplashScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);
    
    // Draw logo if loaded
    if (logoBitmap) {
        int w, h;
        pd->graphics->getBitmapData(logoBitmap, &w, &h, NULL, NULL, NULL);
        pd->graphics->drawBitmap(logoBitmap, (400 - w) / 2, (240 - h) / 2 - 30, kBitmapUnflipped);
    }

    cstr developedByText = "DEVELOPED BY";
    cstr disclaimertextA = "2026 DISKODEV, LLC. ALL RIGHTS RESERVED.";
    cstr disclaimertextB = "Modified PocketMod Player by Nikku4211, licensed under MIT.";

    DrawText(developedByText, (400 - GetTextWidth(developedByText, 2)) / 2, 16, 2, kColorWhite);
    // Draw Lincoln sprite if loaded and visible
    // if (lincolnVisible && lincolnBitmap && lincolnX < 420) {
    //     pd->graphics->drawBitmap(lincolnBitmap, (int)lincolnX, 150, kBitmapUnflipped);
    // }

    DrawText(disclaimertextA, (400 - GetTextWidth(disclaimertextA, 2)) / 2, 200, 2, kColorWhite);
    DrawText(disclaimertextB, (400 - GetTextWidth(disclaimertextB, 2)) / 2, 212, 2, kColorWhite);
    
    // Draw fade overlay on top if needed
    // DrawScreenFade(&screenManager);
}

void SplashScreen_Unload(void) {
    pd->system->logToConsole("SplashScreen: Unloading");
    
    if (logoBitmap) {
        Asset_FreeBitmap(logoBitmap);
        logoBitmap = NULL;
    }
    
    // if (lincolnBitmap) {
    //     pd->graphics->freeBitmap(lincolnBitmap);
    //     lincolnBitmap = NULL;
    // }
    
    // Clear any fade effects if needed
    // ClearScreenFades(&screenManager);
}