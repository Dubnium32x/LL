// written by diskodev
// engine/audio.h
#pragma once

#include "util.h"
#include "pocketmod_player.h"

#define MAX_SOUNDS 255
#define MAX_MUSIC_TRACKS 16

typedef enum {
    AUDIO_MOD, // aka music
    AUDIO_SFX,
    AUDIO_VOX,
    AUDIO_AMB,

    AUDIO_COUNT
} AudioType;

// Legacy path alias kept for portability from TENKOPD manager code.
#ifndef AUDIO_MUSIC_PATH
#define AUDIO_MUSIC_PATH AUDIO_MOD_PATH
#endif

/*
    NOTE:
    We're gonna start this game's development without
    synthesizer sound effects. 

    This makes it easier to focus on the core gameplay
    and mechanics without the added complexity of synthesizer
    audio.
*/

typedef struct {
    cstr path;
    u8 volume; // out of 100
    AudioType type;
    SamplePlayer* player;
    AudioSample* sample;
    bool isPlaying;
    bool isLooping;
} AudioSource; // applies to SFX, VOX and AMB

typedef struct {
    PocketModPlayer* player;
    u8 volume; // out of 100
    bool isPlaying;
    bool isLooping;
    cstr path;
} ModuleSource; // applies to MOD (music)

typedef struct {
    ModuleSource mods[MAX_MUSIC_TRACKS];
    i32 currentModIndex;
    u8 modCount;

    AudioSource sfx[MAX_SOUNDS];
    u8 sfxCount;

    AudioSource vox[MAX_SOUNDS];
    u8 voxCount;
    
    AudioSource amb[MAX_SOUNDS];
    u8 ambCount;
    
    // note: all volumes are out of 100
    u8 masterVol;
    u8 modVol;
    u8 sfxVol;
    u8 voxVol;
    u8 ambVol;

    bool isMusicPlaying;
    bool isMusicEnabled;
    bool isSoundEnabled; // covers sfx, vox, amb, and synth

    bool isFadingOut;
    f32 fadeDuration;
    f32 fadeTimer;
    u8 fadeStartVolume;

    cstr pendingMusicPath;
    u8 pendingMusicVolume;
    bool pendingMusicLoop;
    f32 pendingMusicDelay;
    f32 pendingMusicDelayTimer;
    bool isPendingMusicDelayed;
} AudioManager;

extern AudioManager audioManager;

void Audio_InitManager(AudioManager* manager);
void Audio_UpdateManager(AudioManager* manager, f32 deltaTime);
void Audio_UnloadManager(AudioManager* manager);

void PlayMusic(AudioManager* manager, cstr path, u8 volume, bool loop);
void StopMusic(AudioManager* manager);
void FadeOutMusic(AudioManager* manager, f32 dur);
void FadeInMusic(AudioManager* manager, cstr path, u8 volume, bool loop, f32 dur);
bool IsMusicPlaying(AudioManager* manager);

void PlaySFX(AudioManager* manager, cstr path, u8 volume, bool loop);
void PlayVox(AudioManager* manager, cstr path, u8 volume, bool loop);
void PlayAmb(AudioManager* manager, cstr path, u8 volume, bool loop);

void StopSFX(AudioManager* manager, cstr path);
void StopAllSFX(AudioManager* manager);
void StopAllVox(AudioManager* manager);
void StopAllAmb(AudioManager* manager);

void SetMasterVol(AudioManager* manager, u8 volume);
void SetMusicVol(AudioManager* manager, u8 volume);
void SetSFXVol(AudioManager* manager, u8 volume);
void SetVoxVol(AudioManager* manager, u8 volume);  
void SetAmbVol(AudioManager* manager, u8 volume);

u8 GetMasterVol(AudioManager* manager);
u8 GetMusicVol(AudioManager* manager);
u8 GetSFXVol(AudioManager* manager);
u8 GetVoxVol(AudioManager* manager);
u8 GetAmbVol(AudioManager* manager);

void EnableMusic(AudioManager* manager, bool enabled);
void EnableSound(AudioManager* manager, bool enabled);

bool IsMusicEnabled(AudioManager* manager);
bool IsSoundEnabled(AudioManager* manager);

