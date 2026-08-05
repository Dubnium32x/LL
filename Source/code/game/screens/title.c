// written by diskodev
// game/screens/title.c
#include "title.h"
#include <engine/engine_core.h>
#include <engine/caric.h>

extern PlaydateAPI* pd;

#define TITLE_CLOUD_COUNT 16
#define TITLE_RAIN_COUNT 40
#define TITLE_MIST_COUNT 6

#define SAVE_SLOTS 3
#define TITLE_PANEL_W 210
#define TITLE_PANEL_H 130

typedef enum {
    TITLE_PHASE_PROMPT,
    TITLE_PHASE_MENU,
    TITLE_PHASE_SAVE_SELECT,
    TITLE_PHASE_OPTIONS
} TitlePhase;

typedef enum {
    TITLE_MENU_PLAY_GAME,
    TITLE_MENU_OPTIONS,
    TITLE_MENU_CREDITS,
    TITLE_MENU_COUNT
} TitleMenuItem;

typedef enum {
    TITLE_OPT_MASTER_VOL,
    TITLE_OPT_MUSIC_VOL,
    TITLE_OPT_SFX_VOL,
    TITLE_OPT_BACK,
    TITLE_OPT_COUNT
} TitleOptionItem;

typedef struct {
    f32 x;
    f32 y;
    f32 speed;
    f32 width;
    f32 height;
} TitleCloud;

typedef struct {
    f32 x;
    f32 y;
    f32 speed;
    f32 length;
    f32 drift;
} TitleRaindrop;

typedef struct {
    f32 x;
    f32 y;
    f32 speed;
    f32 width;
    f32 height;
    f32 phase;
} TitleMist;

static LCDBitmap* titleLogo = NULL;
static LCDBitmap* titleMoon = NULL;
static LCDBitmap* iconCursorBit = NULL;
static LCDBitmap* iconPlusBit = NULL;
static LCDBitmap* iconMinusBit = NULL;

static Caricature logoCaric;
static Caricature moonCaric;
static Caricature cursorCaric;
static Caricature plusCaric;
static Caricature minusCaric;

static i32 titleLogoWidth = 0;
static i32 titleLogoHeight = 0;
static i32 titleMoonWidth = 0;
static i32 titleMoonHeight = 0;
static i32 iconCursorWidth = 0;
static i32 iconCursorHeight = 0;
static i32 iconPlusWidth = 0;
static i32 iconPlusHeight = 0;
static i32 iconMinusWidth = 0;
static i32 iconMinusHeight = 0;
static f32 titleTimer = 0.0f;
static bool introSfxFinished = false;

static TitleCloud clouds[TITLE_CLOUD_COUNT];
static TitleRaindrop raindrops[TITLE_RAIN_COUNT];
static TitleMist mistLayers[TITLE_MIST_COUNT];

// State & slide animations
static TitlePhase titlePhase = TITLE_PHASE_PROMPT;
static i32 titleMenuSelection = TITLE_MENU_PLAY_GAME;
static i32 titleOptionsSelection = TITLE_OPT_MASTER_VOL;
static i32 selectedSaveSlot = 0;

static f32 optionsSlide = 0.0f;  // 0.0 = offscreen right, 1.0 = in view
static f32 saveSelectSlide = 0.0f; // 0.0 = offscreen bottom, 1.0 = in view

static f32 cursorBumpScale = 1.0f;
static f32 plusBumpScale = 1.0f;
static f32 minusBumpScale = 1.0f;

// Thunder SFX duration ~1.5 seconds
static const f32 THUNDER_SFX_DURATION = 1.5f;

static void InitBackgroundElements(void) {
    // Top clouds
    for (i32 i = 0; i < TITLE_CLOUD_COUNT; i++) {
        clouds[i].x = (f32)((i * 35) % (SCR_W + 60)) - 20.0f;
        clouds[i].y = -12.0f + (f32)((i * 11) % 36);
        clouds[i].speed = 4.0f + (f32)(i % 4) * 2.5f;
        clouds[i].width = 60.0f + (f32)(i % 5) * 20.0f;
        clouds[i].height = 20.0f + (f32)(i % 3) * 8.0f;
    }

    // Rain
    for (i32 i = 0; i < TITLE_RAIN_COUNT; i++) {
        raindrops[i].x = (f32)((i * 29) % SCR_W);
        raindrops[i].y = (f32)((i * 17) % SCR_H);
        raindrops[i].speed = 170.0f + (f32)((i * 13) % 80);
        raindrops[i].length = 7.0f + (f32)(i % 4) * 3.0f;
        raindrops[i].drift = -20.0f - (f32)(i % 5) * 3.0f;
    }

    // Bottom mist puffs
    for (i32 i = 0; i < TITLE_MIST_COUNT; i++) {
        mistLayers[i].x = (f32)(i * 75) - 30.0f;
        mistLayers[i].y = (f32)(SCR_H - 35 + (i % 3) * 8);
        mistLayers[i].speed = 8.0f + (f32)(i % 3) * 4.0f;
        mistLayers[i].width = 90.0f + (f32)(i % 2) * 30.0f;
        mistLayers[i].height = 22.0f + (f32)(i % 3) * 6.0f;
        mistLayers[i].phase = (f32)i * 1.5f;
    }
}

static void UpdateBackgroundElements(f32 deltaTime) {
    // Update clouds
    for (i32 i = 0; i < TITLE_CLOUD_COUNT; i++) {
        clouds[i].x -= clouds[i].speed * deltaTime;
        if (clouds[i].x + clouds[i].width < -20.0f) {
            clouds[i].x = (f32)SCR_W + 20.0f;
        }
    }

    // Update rain
    for (i32 i = 0; i < TITLE_RAIN_COUNT; i++) {
        raindrops[i].x += raindrops[i].drift * deltaTime;
        raindrops[i].y += raindrops[i].speed * deltaTime;

        if (raindrops[i].y > SCR_H + 10.0f) {
            raindrops[i].y = -raindrops[i].length;
            raindrops[i].x = (f32)((i * 31 + (i32)(titleTimer * 50.0f)) % SCR_W);
        }
        if (raindrops[i].x < -20.0f) {
            raindrops[i].x += (f32)SCR_W + 30.0f;
        }
    }

    // Update mist
    for (i32 i = 0; i < TITLE_MIST_COUNT; i++) {
        mistLayers[i].x += mistLayers[i].speed * deltaTime;
        if (mistLayers[i].x > SCR_W + 20.0f) {
            mistLayers[i].x = -mistLayers[i].width - 10.0f;
        }
    }
}

static void DrawBackgroundElements(void) {
    // 0. Draw Crescent Moon caric or procedural fallback
    if (titleMoon == NULL) {
        i32 moonX = SCR_W - 80;
        i32 moonY = 32;
        i32 moonRadius = 24;

        // Outer moon body (white circle)
        pd->graphics->fillEllipse(moonX, moonY, moonRadius * 2, moonRadius * 2, 0.0f, 0.0f, kColorWhite);

        // Crescent shadow cutout (black circle offset to create crescent shape)
        pd->graphics->fillEllipse(moonX - 10, moonY - 4, moonRadius * 2 - 2, moonRadius * 2 - 2, 0.0f, 0.0f, kColorBlack);
    } else {
        Caric_Draw(&moonCaric);
    }

    // 1. Draw top clouds (white ellipses/puffs)
    pd->graphics->setDrawMode(kDrawModeCopy);
    for (i32 i = 0; i < TITLE_CLOUD_COUNT; i++) {
        pd->graphics->fillEllipse((i32)clouds[i].x, (i32)clouds[i].y,
                                  (i32)clouds[i].width, (i32)clouds[i].height,
                                  0.0f, 0.0f, kColorWhite);
    }

    // 2. Draw rain lines
    for (i32 i = 0; i < TITLE_RAIN_COUNT; i++) {
        i32 x1 = (i32)raindrops[i].x;
        i32 y1 = (i32)raindrops[i].y;
        i32 x2 = (i32)(raindrops[i].x + (raindrops[i].drift * 0.05f));
        i32 y2 = (i32)(raindrops[i].y + raindrops[i].length);

        pd->graphics->drawLine(x1, y1, x2, y2, 1, kColorWhite);
    }

    // 3. Draw bottom mist (white translucent/patterned ellipses near floor)
    static const u8 mistPattern[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
    for (i32 i = 0; i < TITLE_MIST_COUNT; i++) {
        f32 hoverY = mistLayers[i].y + sinf(titleTimer * 2.0f + mistLayers[i].phase) * 3.0f;
        pd->graphics->fillEllipse((i32)mistLayers[i].x, (i32)hoverY,
                                  (i32)mistLayers[i].width, (i32)mistLayers[i].height,
                                  0.0f, 0.0f, (LCDColor)mistPattern);
    }
}

static bool HasSaveInSlot(i32 slotIndex) {
    if (slotIndex < 0 || slotIndex >= SAVE_SLOTS) return false;
    char filename[64];
    snprintf(filename, sizeof(filename), "save_slot_%d.dat", slotIndex);
    SDFile* file = pd->file->open(filename, kFileRead);
    if (file) {
        pd->file->close(file);
        return true;
    }
    return false;
}

static bool DeleteSaveSlot(i32 slotIndex) {
    if (slotIndex < 0 || slotIndex >= SAVE_SLOTS) return false;
    char filename[64];
    snprintf(filename, sizeof(filename), "save_slot_%d.dat", slotIndex);
    return (pd->file->unlink(filename, 0) == 0);
}

void TitleScreen_Init(void) {
    pd->system->logToConsole("TitleScreen: Init");
    titleTimer = 0.0f;
    introSfxFinished = false;

    titlePhase = TITLE_PHASE_PROMPT;
    titleMenuSelection = TITLE_MENU_PLAY_GAME;
    titleOptionsSelection = TITLE_OPT_MASTER_VOL;
    selectedSaveSlot = 0;
    optionsSlide = 0.0f;
    saveSelectSlide = 0.0f;

    InitBackgroundElements();

    // Start fade-in from white (1.2 second duration)
    Visual_StartFade(&visualManager, VISUAL_FADE_WHITE, VISUAL_FADE_IN, 1.2f);

    // Load title logo & UI icons via AssetManager
    titleLogo = Asset_LoadBitmap("assets/texture/image/title_logo_pd");
    titleMoon = Asset_LoadBitmap("assets/texture/image/title_moon");
    iconCursorBit = Asset_LoadBitmap("assets/texture/image/icon_cursor");
    iconPlusBit = Asset_LoadBitmap("assets/texture/image/icon_plus");
    iconMinusBit = Asset_LoadBitmap("assets/texture/image/icon_minus");

    if (titleLogo != NULL) {
        pd->graphics->getBitmapData(titleLogo, &titleLogoWidth, &titleLogoHeight, NULL, NULL, NULL);
        Caric_Init(&logoCaric, "TitleLogo", titleLogo, (Vec2){SCR_W * 0.5f, 100.0f}, (Vec2){1.0f, 1.0f}, CARIC_TYPE_UI);
        Caric_SetOrigin(&logoCaric, (Vec2){0.5f, 0.5f});
    } else {
        pd->system->logToConsole("TitleScreen: failed to load title logo");
    }

    if (titleMoon != NULL) {
        pd->graphics->getBitmapData(titleMoon, &titleMoonWidth, &titleMoonHeight, NULL, NULL, NULL);
        Caric_Init(&moonCaric, "TitleMoon", titleMoon, (Vec2){SCR_W - 60.0f, 60.0f}, (Vec2){1.0f, 1.0f}, CARIC_TYPE_BACKGROUND);
        Caric_SetOrigin(&moonCaric, (Vec2){0.5f, 0.5f});
    }

    if (iconCursorBit != NULL) {
        pd->graphics->getBitmapData(iconCursorBit, &iconCursorWidth, &iconCursorHeight, NULL, NULL, NULL);
        Caric_Init(&cursorCaric, "IconCursor", iconCursorBit, (Vec2){0.0f, 0.0f}, (Vec2){1.0f, 1.0f}, CARIC_TYPE_UI);
        Caric_SetOrigin(&cursorCaric, (Vec2){0.5f, 0.5f});
    }
    if (iconPlusBit != NULL) {
        pd->graphics->getBitmapData(iconPlusBit, &iconPlusWidth, &iconPlusHeight, NULL, NULL, NULL);
        Caric_Init(&plusCaric, "IconPlus", iconPlusBit, (Vec2){0.0f, 0.0f}, (Vec2){1.0f, 1.0f}, CARIC_TYPE_UI);
        Caric_SetOrigin(&plusCaric, (Vec2){0.5f, 0.5f});
    }
    if (iconMinusBit != NULL) {
        pd->graphics->getBitmapData(iconMinusBit, &iconMinusWidth, &iconMinusHeight, NULL, NULL, NULL);
        Caric_Init(&minusCaric, "IconMinus", iconMinusBit, (Vec2){0.0f, 0.0f}, (Vec2){1.0f, 1.0f}, CARIC_TYPE_UI);
        Caric_SetOrigin(&minusCaric, (Vec2){0.5f, 0.5f});
    }

    // Play thunder SFX when title screen starts
    PlaySFX(&audioManager, "assets/audio/sfx/thunder", 100, false);
}

void TitleScreen_Update(f32 deltaTime) {
    titleTimer += deltaTime;

    UpdateBackgroundElements(deltaTime);

    // Update window slide animations (smooth interpolation towards target)
    f32 targetOptionsSlide = (titlePhase == TITLE_PHASE_OPTIONS) ? 1.0f : 0.0f;
    f32 targetSaveSlide = (titlePhase == TITLE_PHASE_SAVE_SELECT) ? 1.0f : 0.0f;

    optionsSlide += (targetOptionsSlide - optionsSlide) * 12.0f * deltaTime;
    saveSelectSlide += (targetSaveSlide - saveSelectSlide) * 12.0f * deltaTime;

    // Smoothly spring bump scales back to 1.0f
    cursorBumpScale += (1.0f - cursorBumpScale) * 15.0f * deltaTime;
    plusBumpScale += (1.0f - plusBumpScale) * 15.0f * deltaTime;
    minusBumpScale += (1.0f - minusBumpScale) * 15.0f * deltaTime;

    // Snap tiny leftover fractions to exactly 1.0f so the icons stop
    // continuously re-scaling (each frame's rounding of the scaled bitmap's
    // pixel dimensions can shift the drawn image by a pixel, which reads as
    // "drifting"/"moving" while the icon settles back down to its resting size)
    if (fabsf(cursorBumpScale - 1.0f) < 0.01f) cursorBumpScale = 1.0f;
    if (fabsf(plusBumpScale - 1.0f) < 0.01f) plusBumpScale = 1.0f;
    if (fabsf(minusBumpScale - 1.0f) < 0.01f) minusBumpScale = 1.0f;

    // Check when thunder SFX finishes to start playing spooky.mod
    if (!introSfxFinished && titleTimer >= THUNDER_SFX_DURATION) {
        introSfxFinished = true;
        PlayMusic(&audioManager, "assets/audio/mod/spooky.mod", 100, true);
    }

    if (!introSfxFinished) return;

    // State machine input processing
    if (titlePhase == TITLE_PHASE_PROMPT) {
        if (Input_IsPressed(&inputManager, INPUT_A)) {
            titlePhase = TITLE_PHASE_MENU;
            titleMenuSelection = TITLE_MENU_PLAY_GAME;
            PlaySFX(&audioManager, "assets/audio/sfx/textbox_appear", 90, false);
        }
    }
    else if (titlePhase == TITLE_PHASE_MENU) {
        if (Input_IsPressed(&inputManager, INPUT_UP)) {
            titleMenuSelection = (titleMenuSelection + TITLE_MENU_COUNT - 1) % TITLE_MENU_COUNT;
            cursorBumpScale = 1.4f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 80, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_DOWN)) {
            titleMenuSelection = (titleMenuSelection + 1) % TITLE_MENU_COUNT;
            cursorBumpScale = 1.4f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 80, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_A)) {
            cursorBumpScale = 1.6f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_confirm", 90, false);
            if (titleMenuSelection == TITLE_MENU_PLAY_GAME) {
                titlePhase = TITLE_PHASE_SAVE_SELECT;
                selectedSaveSlot = 0;
            } else if (titleMenuSelection == TITLE_MENU_OPTIONS) {
                titlePhase = TITLE_PHASE_OPTIONS;
                titleOptionsSelection = TITLE_OPT_MASTER_VOL;
                plusBumpScale = 1.0f;
                minusBumpScale = 1.0f;
            } else if (titleMenuSelection == TITLE_MENU_CREDITS) {
                PlaySFX(&audioManager, "assets/audio/sfx/thunder", 80, false);
                SwitchScreen(&screenManager, SS_TEST_WORLD);
            }
        }
        if (Input_IsPressed(&inputManager, INPUT_B)) {
            titlePhase = TITLE_PHASE_PROMPT;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_back", 80, false);
        }
    }
    else if (titlePhase == TITLE_PHASE_OPTIONS) {
        if (Input_IsPressed(&inputManager, INPUT_UP)) {
            titleOptionsSelection = (titleOptionsSelection + TITLE_OPT_COUNT - 1) % TITLE_OPT_COUNT;
            cursorBumpScale = 1.4f;
            plusBumpScale = 1.0f;
            minusBumpScale = 1.0f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 80, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_DOWN)) {
            titleOptionsSelection = (titleOptionsSelection + 1) % TITLE_OPT_COUNT;
            cursorBumpScale = 1.4f;
            plusBumpScale = 1.0f;
            minusBumpScale = 1.0f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 80, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_LEFT)) {
            minusBumpScale = 1.5f;
            u8 vol = 0;
            if (titleOptionsSelection == TITLE_OPT_MASTER_VOL) {
                vol = GetMasterVol(&audioManager);
                SetMasterVol(&audioManager, vol >= 10 ? vol - 10 : 0);
            } else if (titleOptionsSelection == TITLE_OPT_MUSIC_VOL) {
                vol = GetMusicVol(&audioManager);
                SetMusicVol(&audioManager, vol >= 10 ? vol - 10 : 0);
            } else if (titleOptionsSelection == TITLE_OPT_SFX_VOL) {
                vol = GetSFXVol(&audioManager);
                SetSFXVol(&audioManager, vol >= 10 ? vol - 10 : 0);
            }
            PlaySFX(&audioManager, "assets/audio/sfx/text_letter_blipwav", 70, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_RIGHT)) {
            plusBumpScale = 1.5f;
            u8 vol = 0;
            if (titleOptionsSelection == TITLE_OPT_MASTER_VOL) {
                vol = GetMasterVol(&audioManager);
                SetMasterVol(&audioManager, vol <= 90 ? vol + 10 : 100);
            } else if (titleOptionsSelection == TITLE_OPT_MUSIC_VOL) {
                vol = GetMusicVol(&audioManager);
                SetMusicVol(&audioManager, vol <= 90 ? vol + 10 : 100);
            } else if (titleOptionsSelection == TITLE_OPT_SFX_VOL) {
                vol = GetSFXVol(&audioManager);
                SetSFXVol(&audioManager, vol <= 90 ? vol + 10 : 100);
            }
            PlaySFX(&audioManager, "assets/audio/sfx/text_letter_blipwav", 70, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_B) ||
           (titleOptionsSelection == TITLE_OPT_BACK && Input_IsPressed(&inputManager, INPUT_A))) {
            titlePhase = TITLE_PHASE_MENU;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_back", 80, false);
        }
    }
    else if (titlePhase == TITLE_PHASE_SAVE_SELECT) {
        if (Input_IsPressed(&inputManager, INPUT_UP)) {
            selectedSaveSlot = (selectedSaveSlot + SAVE_SLOTS - 1) % SAVE_SLOTS;
            cursorBumpScale = 1.4f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 80, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_DOWN)) {
            selectedSaveSlot = (selectedSaveSlot + 1) % SAVE_SLOTS;
            cursorBumpScale = 1.4f;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_select", 80, false);
        }
        if (Input_IsPressed(&inputManager, INPUT_A)) {
            PlaySFX(&audioManager, "assets/audio/sfx/menu_confirm", 90, false);
            SwitchScreen(&screenManager, SS_TEST_WORLD);
        }
        if (Input_IsPressed(&inputManager, INPUT_LEFT)) {
            if (HasSaveInSlot(selectedSaveSlot)) {
                if (DeleteSaveSlot(selectedSaveSlot)) {
                    PlaySFX(&audioManager, "assets/audio/sfx/scratch", 80, false);
                }
            }
        }
        if (Input_IsPressed(&inputManager, INPUT_B)) {
            titlePhase = TITLE_PHASE_MENU;
            PlaySFX(&audioManager, "assets/audio/sfx/menu_back", 80, false);
        }
    }
}

void TitleScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);

    // Draw background rain, top clouds, bottom mist, and moon
    DrawBackgroundElements();

    // Calculate squash, stretch, and breathing scale for the title logo
    f32 scaleX = 1.0f;
    f32 scaleY = 1.0f;

    f32 fadeDuration = 1.2f;
    if (titleTimer < fadeDuration) {
        // Initial Squash & Stretch during fade-in (1.2s)
        f32 fadeProgress = titleTimer / fadeDuration; // 0.0 to 1.0
        f32 dampening = 1.0f - fadeProgress;
        f32 bounce = sinf(titleTimer * 18.0f) * dampening * 0.28f;
        scaleX = 1.0f + bounce;
        scaleY = 1.0f - bounce;
    } else {
        // Idle gentle breathing scale (subtle grow & shrink afterwards)
        f32 idleTime = titleTimer - fadeDuration;
        f32 pulse = sinf(idleTime * 2.5f) * 0.035f;
        scaleX = 1.0f + pulse;
        scaleY = 1.0f + pulse;
    }

    // Ensure draw mode is Copy (normal bitmap blending)
    pd->graphics->setDrawMode(kDrawModeCopy);

    // Draw title logo caricature if loaded
    if (titleLogo != NULL) {
        f32 centerX = SCR_W * 0.5f;
        f32 centerY = 100.0f;

        // Draw shadow offset behind logo using caric
        Caric_SetScale(&logoCaric, (Vec2){scaleX, scaleY});

        // Draw shadow caric offset
        Caric_SetPosition(&logoCaric, (Vec2){centerX + 2.0f, centerY + 3.0f});
        Caric_Draw(&logoCaric);

        // Draw main logo caric centered
        Caric_SetPosition(&logoCaric, (Vec2){centerX, centerY});
        Caric_Draw(&logoCaric);
    } else {
        DrawText("LINCOLN'S LABYRINTHINE", (SCR_W - GetTextWidth("LINCOLN'S LABYRINTHINE", 0)) / 2, 40, 0, kColorWhite);
    }

    // Show "Press A to Begin" prompt or options/save windows
    if (introSfxFinished) {
        if (titlePhase == TITLE_PHASE_PROMPT) {
            bool blink = ((i32)(titleTimer * 2.2f) % 2) == 0;
            if (blink) {
                cstr beginPrompt = "Press A to Begin";
                DrawText(beginPrompt, (SCR_W - GetTextWidth(beginPrompt, 2)) / 2, 180, 2, kColorWhite);
            }
        }
        else if (titlePhase == TITLE_PHASE_MENU || titlePhase == TITLE_PHASE_OPTIONS) {
            // Draw Main Menu options using cleaner font (index 2: sonic-hud-life)
            cstr menuItems[TITLE_MENU_COUNT] = { "Play Game", "Options", "Test World" };
            i32 menuStartY = 158;
            i32 menuX = 35;

            for (i32 i = 0; i < TITLE_MENU_COUNT; i++) {
                i32 rowY = menuStartY + (i * 24);
                bool isSel = (i == titleMenuSelection && titlePhase == TITLE_PHASE_MENU);

                if (isSel && iconCursorBit != NULL) {
                    Caric_SetPosition(&cursorCaric, (Vec2){(f32)menuX + iconCursorWidth * 0.5f, (f32)rowY + (iconCursorHeight * 0.5f)});
                    Caric_SetScale(&cursorCaric, (Vec2){cursorBumpScale, cursorBumpScale});
                    Caric_Draw(&cursorCaric);
                    DrawText(menuItems[i], menuX + iconCursorWidth + 6, rowY, 2, kColorWhite);
                } else {
                    char line[32];
                    snprintf(line, sizeof(line), "%s%s", isSel ? "> " : "  ", menuItems[i]);
                    DrawText(line, menuX, rowY, 2, kColorWhite);
                }
            }

            // Draw Options Panel sliding in from the right side
            if (optionsSlide > 0.01f) {
                i32 winW = TITLE_PANEL_W + 10;
                i32 winH = TITLE_PANEL_H + 10;
                i32 targetX = SCR_W - winW - 15;
                i32 hiddenX = SCR_W + 20;
                i32 winX = hiddenX + (i32)((targetX - hiddenX) * optionsSlide);
                i32 winY = 80;

                // Draw sliding window frame
                pd->graphics->fillRect(winX, winY, winW, winH, kColorWhite);
                pd->graphics->fillRect(winX + 2, winY + 2, winW - 4, winH - 4, kColorBlack);

                DrawText("OPTIONS", winX + 12, winY + 8, 2, kColorWhite);

                cstr optLabels[TITLE_OPT_COUNT] = { "Master Vol", "Music Vol", "SFX Vol", "Back" };
                for (i32 i = 0; i < TITLE_OPT_COUNT; i++) {
                    i32 rowY = winY + 32 + (i * 24);
                    bool isOptSel = (i == titleOptionsSelection && titlePhase == TITLE_PHASE_OPTIONS);

                    i32 cursorX = winX + 18;
                    i32 labelX = winX + 32;

                    if (isOptSel && iconCursorBit != NULL) {
                        Caric_SetPosition(&cursorCaric, (Vec2){(f32)cursorX, (f32)rowY + (iconCursorHeight * 0.5f)});
                        Caric_SetScale(&cursorCaric, (Vec2){cursorBumpScale, cursorBumpScale});
                        Caric_Draw(&cursorCaric);
                    } else if (isOptSel) {
                        DrawText(">", winX + 12, rowY, 2, kColorWhite);
                    }

                    DrawText(optLabels[i], labelX, rowY, 2, kColorWhite);

                    // Volume adjusters with fixed position plus/minus caricatures
                    if (i == TITLE_OPT_MASTER_VOL || i == TITLE_OPT_MUSIC_VOL || i == TITLE_OPT_SFX_VOL) {
                        u8 volVal = (i == TITLE_OPT_MASTER_VOL) ? GetMasterVol(&audioManager) :
                                    (i == TITLE_OPT_MUSIC_VOL)  ? GetMusicVol(&audioManager) :
                                                                  GetSFXVol(&audioManager);

                        // Fixed stationary column positions inside options window
                        i32 minusX = winX + winW - 68;
                        i32 textCenterX = winX + winW - 42;
                        i32 plusX = winX + winW - 16;

                        if (iconMinusBit != NULL) {
                            Caric_SetPosition(&minusCaric, (Vec2){(f32)minusX, (f32)rowY + (iconMinusHeight * 0.5f)});
                            Caric_SetScale(&minusCaric, (Vec2){(isOptSel ? minusBumpScale : 1.0f), (isOptSel ? minusBumpScale : 1.0f)});
                            Caric_Draw(&minusCaric);
                        }

                        char valBuf[16];
                        snprintf(valBuf, sizeof(valBuf), "%d%%", volVal);
                        i32 textW = GetTextWidth(valBuf, 2);
                        DrawText(valBuf, textCenterX - (textW / 2), rowY, 2, kColorWhite);

                        if (iconPlusBit != NULL) {
                            Caric_SetPosition(&plusCaric, (Vec2){(f32)plusX, (f32)rowY + (iconPlusHeight * 0.5f)});
                            Caric_SetScale(&plusCaric, (Vec2){(isOptSel ? plusBumpScale : 1.0f), (isOptSel ? plusBumpScale : 1.0f)});
                            Caric_Draw(&plusCaric);
                        }
                    }
                }
            }
        }

        // Draw Save Data Window sliding in from the right/bottom
        if (saveSelectSlide > 0.01f) {
            i32 winW = SCR_W - 50;
            i32 winH = 145;
            i32 targetX = 25;
            i32 hiddenX = SCR_W + 30;
            i32 winX = hiddenX + (i32)((targetX - hiddenX) * saveSelectSlide);
            i32 winY = 80;

            pd->graphics->fillRect(winX, winY, winW, winH, kColorWhite);
            pd->graphics->fillRect(winX + 2, winY + 2, winW - 4, winH - 4, kColorBlack);

            DrawText("SELECT DATA SLOT", winX + 16, winY + 8, 2, kColorWhite);

            for (i32 i = 0; i < SAVE_SLOTS; i++) {
                i32 slotY = winY + 32 + (i * 34);
                bool isSelected = (i == selectedSaveSlot && titlePhase == TITLE_PHASE_SAVE_SELECT);
                bool hasSave = HasSaveInSlot(i);

                pd->graphics->fillRect(winX + 12, slotY, winW - 24, 28, kColorWhite);
                pd->graphics->fillRect(winX + (isSelected ? 15 : 13), slotY + (isSelected ? 3 : 1),
                                       winW - (isSelected ? 30 : 26), 28 - (isSelected ? 6 : 2), kColorBlack);

                i32 textX = winX + 20;
                if (isSelected && iconCursorBit != NULL) {
                    Caric_SetPosition(&cursorCaric, (Vec2){(f32)textX + iconCursorWidth * 0.5f, (f32)slotY + 14.0f});
                    Caric_SetScale(&cursorCaric, (Vec2){cursorBumpScale, cursorBumpScale});
                    Caric_Draw(&cursorCaric);
                    textX += iconCursorWidth + 6;
                }

                char slotLabel[32];
                snprintf(slotLabel, sizeof(slotLabel), "%sSLOT %d: %s", (isSelected && iconCursorBit == NULL) ? "> " : "", i + 1, hasSave ? "SAVED GAME" : "EMPTY");
                DrawText(slotLabel, textX, slotY + 5, 2, kColorWhite);
            }
        }
    }
}

void TitleScreen_Unload(void) {
    pd->system->logToConsole("TitleScreen: Unload");

    if (titleLogo != NULL) {
        Asset_FreeBitmap(titleLogo);
        titleLogo = NULL;
    }
    if (titleMoon != NULL) {
        Asset_FreeBitmap(titleMoon);
        titleMoon = NULL;
    }
    if (iconCursorBit != NULL) {
        Asset_FreeBitmap(iconCursorBit);
        iconCursorBit = NULL;
    }
    if (iconPlusBit != NULL) {
        Asset_FreeBitmap(iconPlusBit);
        iconPlusBit = NULL;
    }
    if (iconMinusBit != NULL) {
        Asset_FreeBitmap(iconMinusBit);
        iconMinusBit = NULL;
    }
} 