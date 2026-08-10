// written by diskodev
// game/data/data.h
#pragma once

#include <engine/engine_core.h>
#include "world.h"
#include "objects.h"

#define SAVES 3
#define SAVE_NAME_LEN 16

#define USE_TXT TRUE
#define USE_JSON FALSE

#define SAVE_POINTS_PER_AREA 4

#define MAX_NOTES 32

#define SAVE_FILE_NAME "save.txt"

typedef enum {
    AREA_MANSION,
    AREA_CAVES,
    AREA_LAB,
    AREA_CIRCUS,
    AREA_FACTORY,
    AREA_HOSPITAL,
    AREA_ASYLUM,

    AREA_AFTERMATH,
    AREA_COUNT
} AreaType;

/*
    We don't know what the best course of action is for our game's
    saves will be. Binary? Text? JSON?

    For now, the best suitable option should be text.
    Able to store the save data in a human-readable format,
    and we can easily edit it if we need to.
*/

typedef enum {
    ITEM_NONE,

    // items that are not used with B
    ITEM_KEY,
    ITEM_KEYCARD,
    ITEM_CRANK_PIECE,
    
    // items that are used with B
    ITEM_BROOM,
    ITEM_CROSS,
    ITEM_KNIFE,
    ITEM_HATCHET,
    ITEM_SMOKE_BOMB,
    
    // items that are used with B and UP
    ITEM_POTION,
    ITEM_SANITY_PILL,
    
    ITEM_COUNT
} ItemType;
// these are items that the player can pick up 
// each category of items are used in a different way

typedef struct {
    u16 currentRoom;
    AreaType currentArea;
    ItemType* currentItems[3];

    bool notes[MAX_NOTES];

    u8 savePoint[AREA_COUNT][SAVE_POINTS_PER_AREA];

    // quick save
    bool hasQuicksaved;
    u16 quicksavedRoom; // for quicksave feature
    u8 quicksavedHealth;
    u8 quicksavedSanity;
    ItemType* quicksavedItems[3];
    AreaType quicksavedArea;
} PlayerData;

typedef struct {
    PlayerData* playerData[3]; // out of 3

    u8 masterVol;
    u8 musicVol;
    u8 sfxVol;
    u8 voxVol;
    u8 ambVol;
   
    bool flashingLights;
    bool goreEnabled;

    bool hasSeenIntro;

    u8   textSpeed;        // 0=Slow 1=Normal 2=Fast
    bool screenFxEnabled;  // visual effects toggle
} SaveData;

static inline void SaveData_Init(SaveData* saveData) {
    saveData->playerData[0] = NULL;
    saveData->playerData[1] = NULL;
    saveData->playerData[2] = NULL;

    saveData->masterVol = 100;
    saveData->musicVol = 100;
    saveData->sfxVol = 100;
    saveData->voxVol = 100;
    saveData->ambVol = 100;

    saveData->flashingLights = true;
    saveData->goreEnabled = true;

    saveData->hasSeenIntro = false;

    saveData->textSpeed       = 1;
    saveData->screenFxEnabled = true;
} // default values for a new save data

static inline void PlayerData_Init(PlayerData* playerData) {
    playerData->currentRoom = 0;
    playerData->currentArea = AREA_MANSION;

    playerData->currentItems[0] = NULL;
    playerData->currentItems[1] = NULL;
    playerData->currentItems[2] = NULL;

    for (int i = 0; i < MAX_NOTES; i++) {
        playerData->notes[i] = false;
    }

    for (int i = 0; i < AREA_COUNT; i++) {
        for (int j = 0; j < SAVE_POINTS_PER_AREA; j++) {
            playerData->savePoint[i][j] = 0;
        }
    }

    // quick save
    playerData->quicksavedRoom = 0;
    playerData->quicksavedHealth = 100;
    playerData->quicksavedSanity = 100;
    playerData->quicksavedItems[0] = NULL;
    playerData->quicksavedItems[1] = NULL;
    playerData->quicksavedItems[2] = NULL;
    playerData->quicksavedArea = AREA_MANSION;
}

/*
    SAVE STRUCTURE 
    --------------

SaveData

masterVol=100
musicVol=100
sfxVol=100
voxVol=100
ambVol=100
flashingLights=true
goreEnabled=true
hasSeenIntro=false
textSpeed=1
screenFxEnabled=true

Slot 1

playername=Player 1
currentRoom=0
savepoint=1,0
quicksavedRoom=0
quicksavedHealth=100
quicksavedSanity=100
hasQuicksaved=false
quicksavedItems[0]=NULL
quicksavedItems[1]=NULL
quicksavedItems[2]=NULL
quicksavedArea=AREA_MANSION
savedItems[0]=NULL
savedItems[1]=NULL
savedItems[2]=NULL
notes=00000000000000000000000000000000 // 32 notes, 0 = not found, 1 = found

...

*/
#define DATA_VERSION 1
#define DATA_HEADER "SaveData"
#define DATA_MASTER_VOL "masterVol"
#define DATA_MUSIC_VOL "musicVol"
#define DATA_SFX_VOL "sfxVol"
#define DATA_VOX_VOL "voxVol"
#define DATA_AMB_VOL "ambVol"
#define DATA_FLASHING_LIGHTS "flashingLights"
#define DATA_GORE_ENABLED "goreEnabled"
#define DATA_HAS_SEEN_INTRO "hasSeenIntro"
#define DATA_TEXT_SPEED     "textSpeed"
#define DATA_SCREEN_FX      "screenFxEnabled"

#define DATA_PLAYER_HEADER "Slot %d"
#define DATA_PLAYER_NAME "playername"
#define DATA_PLAYER_CURRENT_ROOM "currentRoom"
#define DATA_PLAYER_CURRENT_AREA "currentArea"
#define DATA_PLAYER_SAVEPOINT "savepoint"
#define DATA_PLAYER_QUICKSAVED_ROOM "quicksavedRoom"
#define DATA_PLAYER_QUICKSAVED_HEALTH "quicksavedHealth"
#define DATA_PLAYER_QUICKSAVED_SANITY "quicksavedSanity"
#define DATA_PLAYER_HAS_QUICKSAVED "hasQuicksaved"
#define DATA_PLAYER_QUICKSAVED_ITEMS "quicksavedItems"
#define DATA_PLAYER_SAVED_ITEMS "savedItems"
#define DATA_PLAYER_NOTES "notes"

#define DATA_PLAYER_DEFAULT_NOTES 0b00000000000000000000000000000000

extern u32        notesFoundBitmask;
extern PlayerData playerData;
extern SaveData   saveData;

const char* AreaType_Name(AreaType area);
const char* ItemType_Name(ItemType item);
int         PlayerData_NoteCount(const PlayerData* p);

static inline void SaveData_Free(SaveData* saveData) {
    for (int i = 0; i < 3; i++) {
        if (saveData->playerData[i]) {
            free(saveData->playerData[i]);
            saveData->playerData[i] = NULL;
        }
    }
}

static inline void PlayerData_Free(PlayerData* playerData) {
    if (playerData) {
        free(playerData);
        playerData = NULL;
    }
}

static inline void PlayerData_SaveNote(PlayerData* playerData, u8 noteIndex) {
    if (noteIndex >= MAX_NOTES) return;
    playerData->notes[noteIndex] = true;
}

