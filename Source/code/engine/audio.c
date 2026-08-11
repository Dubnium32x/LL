// written by diskodev
// engine/audio.c
#include "audio.h"

AudioManager audioManager = {0};

static PocketModPlayer modPlayer;

static u8 ClampVolume100(u8 volume) {
	return (u8)Clamp((f32)volume, 0.0f, 100.0f);
}

static f32 ComputeEffectiveVolume(const AudioManager* manager, u8 sourceVolume, u8 categoryVolume) {
	if (manager == NULL) return 0.0f;
	return (sourceVolume / 100.0f) * (categoryVolume / 100.0f) * (manager->masterVol / 100.0f);
}

static AudioSource* FindSampleSource(AudioSource* sources, u8 count, cstr path) {
	if (sources == NULL || path == NULL) return NULL;
	for (u8 i = 0; i < count; i++) {
		if (sources[i].path != NULL && strcmp(sources[i].path, path) == 0) {
			return &sources[i];
		}
	}
	return NULL;
}

static AudioSource* GetOrCreateSampleSource(AudioSource* sources, u8* count, i32 capacity, AudioType type, cstr path) {
	if (sources == NULL || count == NULL || path == NULL) return NULL;

	AudioSource* existing = FindSampleSource(sources, *count, path);
	if (existing != NULL) return existing;

	if ((i32)*count >= capacity) return NULL;

	AudioSource* source = &sources[*count];
	memset(source, 0, sizeof(AudioSource));
	source->path = path;
	source->type = type;
	source->volume = 100;
	(*count)++;
	return source;
}

static void RefreshSourceVolumes(AudioManager* manager, AudioSource* sources, u8 count, u8 categoryVolume) {
	if (manager == NULL || sources == NULL) return;

	for (u8 i = 0; i < count; i++) {
		AudioSource* source = &sources[i];
		if (source->player == NULL) continue;

		f32 vol = ComputeEffectiveVolume(manager, source->volume, categoryVolume);
		pd->sound->sampleplayer->setVolume(source->player, vol, vol);
	}
}

static void PlaySample(AudioManager* manager,
					   AudioSource* sources,
					   u8* count,
					   AudioType type,
					   cstr path,
					   u8 volume,
					   bool loop,
					   u8 categoryVolume) {
	if (manager == NULL || pd == NULL || path == NULL) return;
	if (!manager->isSoundEnabled) return;

	AudioSource* source = GetOrCreateSampleSource(sources, count, MAX_SOUNDS, type, path);
	if (source == NULL) return;

	source->volume = ClampVolume100(volume);
	source->isLooping = loop;

	if (source->sample == NULL) {
		source->sample = pd->sound->sample->load(path);
		if (source->sample == NULL) {
			LOG("Audio: failed to load sample: %s", path);
			return;
		}
	}

	if (source->player == NULL) {
		source->player = pd->sound->sampleplayer->newPlayer();
		if (source->player == NULL) {
			LOG("Audio: failed to create sample player: %s", path);
			return;
		}
		pd->sound->sampleplayer->setSample(source->player, source->sample);
	}

	f32 vol = ComputeEffectiveVolume(manager, source->volume, categoryVolume);
	pd->sound->sampleplayer->setVolume(source->player, vol, vol);
	pd->sound->sampleplayer->play(source->player, loop ? 0 : 1, 1.0f);
	source->isPlaying = true;
}

static void StopSampleSource(AudioSource* sources, u8 count, cstr path) {
	if (sources == NULL || path == NULL) return;
	AudioSource* source = FindSampleSource(sources, count, path);
	if (source == NULL || source->player == NULL) return;
	pd->sound->sampleplayer->stop(source->player);
	source->isPlaying = false;
}

static void StopAllSampleSources(AudioSource* sources, u8 count) {
	if (sources == NULL || pd == NULL) return;

	for (u8 i = 0; i < count; i++) {
		AudioSource* source = &sources[i];
		if (source->player != NULL) {
			pd->sound->sampleplayer->stop(source->player);
		}
		source->isPlaying = false;
	}
}

static void UnloadSampleSources(AudioSource* sources, u8 count) {
	if (sources == NULL || pd == NULL) return;

	for (u8 i = 0; i < count; i++) {
		AudioSource* source = &sources[i];
		if (source->player != NULL) {
			pd->sound->sampleplayer->stop(source->player);
			pd->sound->sampleplayer->freePlayer(source->player);
			source->player = NULL;
		}
		if (source->sample != NULL) {
			pd->sound->sample->freeSample(source->sample);
			source->sample = NULL;
		}
		source->isPlaying = false;
	}
}

static i32 FindModIndex(AudioManager* manager, cstr path) {
	if (manager == NULL || path == NULL) return -1;

	for (u8 i = 0; i < manager->modCount; i++) {
		if (manager->mods[i].path != NULL && strcmp(manager->mods[i].path, path) == 0) {
			return i;
		}
	}

	return -1;
}

static i32 GetOrCreateModIndex(AudioManager* manager, cstr path) {
	if (manager == NULL || path == NULL) return -1;

	i32 index = FindModIndex(manager, path);
	if (index >= 0) return index;

	if (manager->modCount < MAX_MUSIC_TRACKS) {
		i32 newIndex = manager->modCount;
		ModuleSource* mod = &manager->mods[newIndex];
		memset(mod, 0, sizeof(ModuleSource));
		mod->path = path;
		manager->modCount++;
		return newIndex;
	}

	manager->mods[0].path = path;
	manager->mods[0].isPlaying = false;
	manager->mods[0].isLooping = false;
	return 0;
}

void Audio_InitManager(AudioManager* manager) {
	if (manager == NULL || pd == NULL) return;

	memset(manager, 0, sizeof(AudioManager));
	manager->currentModIndex = -1;

	manager->masterVol = 80;
	manager->modVol = 100;
	manager->sfxVol = 100;
	manager->voxVol = 100;
	manager->ambVol = 100;

	manager->isMusicEnabled = true;
	manager->isSoundEnabled = true;

	InitPocketModPlayer(&modPlayer);
}

void Audio_UpdateManager(AudioManager* manager, f32 deltaTime) {
	if (manager == NULL || pd == NULL) return;

	UpdatePocketModPlayer(&modPlayer);

	if (manager->isFadingOut && manager->isMusicPlaying) {
		manager->fadeTimer += deltaTime;
		f32 t = (manager->fadeDuration > 0.0f) ? (manager->fadeTimer / manager->fadeDuration) : 1.0f;
		t = Clamp(t, 0.0f, 1.0f);

		u8 sourceVol = (u8)Lerp((f32)manager->fadeStartVolume, 0.0f, t);
		SetModVolume(&modPlayer, ComputeEffectiveVolume(manager, sourceVol, manager->modVol));

		if (t >= 1.0f) {
			StopMusic(manager);
			manager->isFadingOut = false;
			manager->fadeTimer = 0.0f;

			if (manager->pendingMusicPath != NULL) {
				cstr path = manager->pendingMusicPath;
				u8 volume = manager->pendingMusicVolume;
				bool loop = manager->pendingMusicLoop;

				manager->pendingMusicPath = NULL;
				PlayMusic(manager, path, volume, loop);
			}
		}
	}

	if (manager->isPendingMusicDelayed && manager->pendingMusicPath != NULL) {
		manager->pendingMusicDelayTimer += deltaTime;
		if (manager->pendingMusicDelayTimer >= manager->pendingMusicDelay) {
			cstr path = manager->pendingMusicPath;
			u8 volume = manager->pendingMusicVolume;
			bool loop = manager->pendingMusicLoop;

			manager->pendingMusicPath = NULL;
			manager->isPendingMusicDelayed = false;
			manager->pendingMusicDelay = 0.0f;
			manager->pendingMusicDelayTimer = 0.0f;

			PlayMusic(manager, path, volume, loop);
		}
	}
}

void Audio_UnloadManager(AudioManager* manager) {
	if (manager == NULL || pd == NULL) return;

	StopMusic(manager);
	UnloadPocketModPlayer(&modPlayer);

	UnloadSampleSources(manager->sfx, manager->sfxCount);
	UnloadSampleSources(manager->vox, manager->voxCount);
	UnloadSampleSources(manager->amb, manager->ambCount);

	manager->modCount = 0;
	manager->sfxCount = 0;
	manager->voxCount = 0;
	manager->ambCount = 0;
	manager->currentModIndex = -1;
}

void PlayMusic(AudioManager* manager, cstr path, u8 volume, bool loop) {
	if (manager == NULL || pd == NULL || path == NULL) return;
	if (!manager->isMusicEnabled) return;

	if (manager->isMusicPlaying) {
		StopMusic(manager);
	}

	i32 index = GetOrCreateModIndex(manager, path);
	if (index < 0) return;

	bool isSameTrack = (manager->currentModIndex >= 0 &&
						manager->mods[manager->currentModIndex].path != NULL &&
						strcmp(manager->mods[manager->currentModIndex].path, path) == 0);

	if (isSameTrack && IsModLoaded(&modPlayer)) {
		RestartMod(&modPlayer);
	} else {
		if (!LoadModFromPath(&modPlayer, path)) {
			LOG("Audio: failed to load module: %s", path);
			return;
		}
	}

	ModuleSource* mod = &manager->mods[index];
	mod->path = path;
	mod->volume = ClampVolume100(volume);
	mod->isLooping = loop;
	mod->isPlaying = true;
	mod->player = &modPlayer;

	SetModVolume(&modPlayer, ComputeEffectiveVolume(manager, mod->volume, manager->modVol));
	PlayMod(&modPlayer);

	manager->currentModIndex = index;
	manager->isMusicPlaying = true;
	manager->isFadingOut = false;
}

void StopMusic(AudioManager* manager) {
	if (manager == NULL) return;

	if (IsModLoaded(&modPlayer)) {
		PauseMod(&modPlayer);
		RestartMod(&modPlayer);
	}

	if (manager->currentModIndex >= 0 && manager->currentModIndex < manager->modCount) {
		manager->mods[manager->currentModIndex].isPlaying = false;
	}

	manager->isMusicPlaying = false;
	manager->isFadingOut = false;
}

void FadeOutMusic(AudioManager* manager, f32 dur) {
	if (manager == NULL || !manager->isMusicPlaying) return;

	u8 startVolume = 100;
	if (manager->currentModIndex >= 0 && manager->currentModIndex < manager->modCount) {
		startVolume = manager->mods[manager->currentModIndex].volume;
	}

	manager->isFadingOut = true;
	manager->fadeDuration = MaxF(0.01f, dur);
	manager->fadeTimer = 0.0f;
	manager->fadeStartVolume = startVolume;
}

void FadeInMusic(AudioManager* manager, cstr path, u8 volume, bool loop, f32 dur) {
	(void)dur;
	if (manager == NULL || path == NULL) return;

	if (manager->isMusicPlaying) {
		manager->pendingMusicPath = path;
		manager->pendingMusicVolume = ClampVolume100(volume);
		manager->pendingMusicLoop = loop;
		manager->isPendingMusicDelayed = false;
		manager->pendingMusicDelay = 0.0f;
		manager->pendingMusicDelayTimer = 0.0f;
		FadeOutMusic(manager, MaxF(0.01f, dur));
		return;
	}

	PlayMusic(manager, path, volume, loop);
}

bool IsMusicPlaying(AudioManager* manager) {
	return manager ? manager->isMusicPlaying : false;
}

void PlaySFX(AudioManager* manager, cstr path, u8 volume, bool loop) {
	if (manager == NULL) return;
	PlaySample(manager, manager->sfx, &manager->sfxCount, AUDIO_SFX, path, volume, loop, manager->sfxVol);
}

void PlayVox(AudioManager* manager, cstr path, u8 volume, bool loop) {
	if (manager == NULL) return;
	PlaySample(manager, manager->vox, &manager->voxCount, AUDIO_VOX, path, volume, loop, manager->voxVol);
}

void PlayAmb(AudioManager* manager, cstr path, u8 volume, bool loop) {
	if (manager == NULL) return;
	PlaySample(manager, manager->amb, &manager->ambCount, AUDIO_AMB, path, volume, loop, manager->ambVol);
}

void StopSFX(AudioManager* manager, cstr path) {
	if (manager == NULL) return;
	StopSampleSource(manager->sfx, manager->sfxCount, path);
}

void StopAllSFX(AudioManager* manager) {
	if (manager == NULL) return;
	StopAllSampleSources(manager->sfx, manager->sfxCount);
}

void StopAllVox(AudioManager* manager) {
	if (manager == NULL) return;
	StopAllSampleSources(manager->vox, manager->voxCount);
}

void StopAllAmb(AudioManager* manager) {
	if (manager == NULL) return;
	StopAllSampleSources(manager->amb, manager->ambCount);
}

void SetMasterVol(AudioManager* manager, u8 volume) {
	if (manager == NULL) return;
	manager->masterVol = ClampVolume100(volume);

	if (manager->currentModIndex >= 0 && manager->currentModIndex < manager->modCount && IsModLoaded(&modPlayer)) {
		u8 srcVol = manager->mods[manager->currentModIndex].volume;
		SetModVolume(&modPlayer, ComputeEffectiveVolume(manager, srcVol, manager->modVol));
	}

	RefreshSourceVolumes(manager, manager->sfx, manager->sfxCount, manager->sfxVol);
	RefreshSourceVolumes(manager, manager->vox, manager->voxCount, manager->voxVol);
	RefreshSourceVolumes(manager, manager->amb, manager->ambCount, manager->ambVol);
}

void SetMusicVol(AudioManager* manager, u8 volume) {
	if (manager == NULL) return;
	manager->modVol = ClampVolume100(volume);

	if (manager->currentModIndex >= 0 && manager->currentModIndex < manager->modCount && IsModLoaded(&modPlayer)) {
		u8 srcVol = manager->mods[manager->currentModIndex].volume;
		SetModVolume(&modPlayer, ComputeEffectiveVolume(manager, srcVol, manager->modVol));
	}
}

void SetSFXVol(AudioManager* manager, u8 volume) {
	if (manager == NULL) return;
	manager->sfxVol = ClampVolume100(volume);
	RefreshSourceVolumes(manager, manager->sfx, manager->sfxCount, manager->sfxVol);
}

void SetVoxVol(AudioManager* manager, u8 volume) {
	if (manager == NULL) return;
	manager->voxVol = ClampVolume100(volume);
	RefreshSourceVolumes(manager, manager->vox, manager->voxCount, manager->voxVol);
}

void SetAmbVol(AudioManager* manager, u8 volume) {
	if (manager == NULL) return;
	manager->ambVol = ClampVolume100(volume);
	RefreshSourceVolumes(manager, manager->amb, manager->ambCount, manager->ambVol);
}

u8 GetMasterVol(AudioManager* manager) {
	return manager ? manager->masterVol : 0;
}

u8 GetMusicVol(AudioManager* manager) {
	return manager ? manager->modVol : 0;
}

u8 GetSFXVol(AudioManager* manager) {
	return manager ? manager->sfxVol : 0;
}

u8 GetVoxVol(AudioManager* manager) {
	return manager ? manager->voxVol : 0;
}

u8 GetAmbVol(AudioManager* manager) {
	return manager ? manager->ambVol : 0;
}

void EnableMusic(AudioManager* manager, bool enabled) {
	if (manager == NULL) return;
	manager->isMusicEnabled = enabled;
	if (!enabled) {
		StopMusic(manager);
	}
}

void EnableSound(AudioManager* manager, bool enabled) {
	if (manager == NULL) return;
	manager->isSoundEnabled = enabled;

	if (!enabled) {
		StopAllSFX(manager);
		StopAllVox(manager);
		StopAllAmb(manager);
	}
}

bool IsMusicEnabled(AudioManager* manager) {
	return manager ? manager->isMusicEnabled : false;
}

bool IsSoundEnabled(AudioManager* manager) {
	return manager ? manager->isSoundEnabled : false;
}

