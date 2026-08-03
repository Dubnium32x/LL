// written by diskodev
// engine/edit/screen_state.h
#pragma once
#include <engine/util.h>

#define MAX_SCREEN_FADES 8
#define MAX_REGISTERED_SCREENS 32

typedef enum {
    SS_INIT,
    SS_SPLASH,
    SS_INTRO,
    SS_TITLE,
    SS_MENU,
    SS_OPTIONS,
    SS_GAME,
    SS_PAUSE,
    SS_GAMEOVER,
    SS_CUTSCENE,
    SS_EXIT,
    SS_CREDITS,

    SS_TEST_1,
    SS_TEST_2,
    SS_TEST_3,
    SS_TEST_4,
    SS_TEST_5,
    SS_TEST_6,

    SS_COUNT
} ScreenState;

static inline cstr GetScreenStateName(ScreenState state) {
    switch (state) {
        case SS_INIT: return "INIT";
        case SS_SPLASH: return "SPLASH";
        case SS_INTRO: return "INTRO";
        case SS_TITLE: return "TITLE";
        case SS_MENU: return "MENU";
        case SS_OPTIONS: return "OPTIONS";
        case SS_GAME: return "GAME";
        case SS_PAUSE: return "PAUSE";
        case SS_GAMEOVER: return "GAMEOVER";
        case SS_CUTSCENE: return "CUTSCENE";
        case SS_EXIT: return "EXIT";
        case SS_CREDITS: return "CREDITS";
        case SS_TEST_1: return "TEST_1";
        case SS_TEST_2: return "TEST_2";
        case SS_TEST_3: return "TEST_3";
        case SS_TEST_4: return "TEST_4";
        case SS_TEST_5: return "TEST_5";
        case SS_TEST_6: return "TEST_6";
        default: return "UNKNOWN";
    }
}

static inline void PrintScreenState(ScreenState state) {
    for (i32 i = 0; i < SS_COUNT; i++) {
        if (state == i) {
            LOG("Current Screen State: %s", GetScreenStateName(state));
            return;
        }
    }
    LOG("Current Screen State: UNKNOWN (%d)", state);
}