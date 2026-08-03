// PocketMod-based Module Player for Playdate
#ifndef POCKETMOD_PLAYER_H
#define POCKETMOD_PLAYER_H

#include "util.h"

// Forward declare pocketmod_context; full definition comes from pocketmod_player.c
typedef struct pocketmod_context pocketmod_context;

typedef struct PocketModPlayer PocketModPlayer;

typedef enum {
    POCKETMOD_STATE_STOPPED,
    POCKETMOD_STATE_PLAYING,
    POCKETMOD_STATE_PAUSED
} PocketModState;

bool InitPocketModPlayer(PocketModPlayer* player);
void UnloadPocketModPlayer(PocketModPlayer* player);
bool LoadModFromPath(PocketModPlayer* player, const char* path);
void PlayMod(PocketModPlayer* player);
void PauseMod(PocketModPlayer* player);
void ResumeMod(PocketModPlayer* player);
void RestartMod(PocketModPlayer* player);
void UpdatePocketModPlayer(PocketModPlayer* player);

bool IsModLoaded(PocketModPlayer* player);
bool IsModPlaying(PocketModPlayer* player);
bool IsModPaused(PocketModPlayer* player);
PocketModState GetModState(PocketModPlayer* player);

void SetModVolume(PocketModPlayer* player, float volume);
float GetModVolume(PocketModPlayer* player);

const char* GetModName(PocketModPlayer* player);
const char* GetModType(PocketModPlayer* player);
i32 GetModChannelCount(PocketModPlayer* player);
i32 GetModCurrentTime(PocketModPlayer* player);
i32 GetModTotalTime(PocketModPlayer* player);
i32 GetModPosition(PocketModPlayer* player);
i32 GetModRow(PocketModPlayer* player);
const char* GetModLoadStage(PocketModPlayer* player);

struct PocketModPlayer {
    void* modData;
    void* pmodContext;
    SoundSource* soundSource;
    bool loaded;
    bool playing;
    PocketModState state;
    float volume;
    char moduleName[32];
    char loadStage[32];
    i32 sampleRate;
    i32 channels;
    i32 position;
    i32 row;
    u32 timeMs;
    u32 totalTimeMs;
};

#endif // POCKETMOD_PLAYER_H
