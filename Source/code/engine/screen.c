// written by diskodev
// engine/screen.c
#include "screen.h"
#include "audio.h"

ScreenManager screenManager = {0};

static bool IsValidScreenType(ScreenState type) {
	return type >= 0 && type < MAX_REGISTERED_SCREENS;
}

static void ClearScreenSlot(IScreen* screen) {
	if (screen == NULL) return;
	screen->Init = NULL;
	screen->Update = NULL;
	screen->Draw = NULL;
	screen->Unload = NULL;
}

void ScreenManager_Init(ScreenManager* manager) {
	if (manager == NULL) return;

	memset(manager, 0, sizeof(ScreenManager));
	manager->currentScreen = SS_INIT;
	manager->nextScreen = SS_INIT;
	manager->isPaused = false;
	manager->isTransitioning = false;

	Visual_InitManager(&visualManager);
}

void ScreenManager_Update(ScreenManager* manager, f32 deltaTime) {
	if (manager == NULL) return;

	Visual_UpdateManager(&visualManager, deltaTime);
	manager->isTransitioning = Visual_IsFadeActive(&visualManager);

	if (manager->isPaused) return;

	IScreen* activeScreen = GetActiveScreen(manager);
	if (activeScreen != NULL && activeScreen->Update != NULL) {
		activeScreen->Update(deltaTime);
	}
}

void ScreenManager_Draw(ScreenManager* manager) {
	if (manager == NULL) return;

	IScreen* activeScreen = GetActiveScreen(manager);
	if (activeScreen != NULL && activeScreen->Draw != NULL) {
		activeScreen->Draw();
	}

	if (!visualManager.autoFadeDrawSuppressed) {
		Visual_DrawFade(&visualManager);
	}
	Visual_DrawStatic(&visualManager);
	Visual_DrawTVSnow(&visualManager);
}

void ScreenManager_Unload(ScreenManager* manager) {
	if (manager == NULL) return;

	IScreen* activeScreen = GetActiveScreen(manager);
	if (activeScreen != NULL && activeScreen->Unload != NULL) {
		activeScreen->Unload();
	}

	for (i32 i = 0; i < MAX_REGISTERED_SCREENS; i++) {
		ClearScreenSlot(&manager->screens[i]);
	}

	manager->currentScreen = SS_INIT;
	manager->nextScreen = SS_INIT;
	manager->isPaused = false;
	manager->isTransitioning = false;
	Visual_StopFade(&visualManager);
}

void RegisterScreen(ScreenManager* manager, ScreenState type,
					void (*Init)(void),
					void (*Update)(f32),
					void (*Draw)(void),
					void (*Unload)(void)) {
	if (manager == NULL || !IsValidScreenType(type)) {
		LOG("RegisterScreen: invalid type %d", type);
		return;
	}

	manager->screens[type].Init = Init;
	manager->screens[type].Update = Update;
	manager->screens[type].Draw = Draw;
	manager->screens[type].Unload = Unload;
}

void SwitchScreen(ScreenManager* manager, ScreenState type) {
	if (manager == NULL || !IsValidScreenType(type)) {
		LOG("SwitchScreen: invalid type %d", type);
		return;
	}

	if (manager->currentScreen == type) return;

	FadeOutMusic(&audioManager, 0.6f);

	if (IsValidScreenType(manager->currentScreen)) {
		IScreen* current = &manager->screens[manager->currentScreen];
		if (current->Unload != NULL) {
			current->Unload();
		}
	}

	manager->nextScreen = type;
	manager->currentScreen = type;

	if (IsValidScreenType(type)) {
		IScreen* next = &manager->screens[type];
		if (next->Init != NULL) {
			next->Init();
		}
	}

	manager->isTransitioning = Visual_IsFadeActive(&visualManager);
}

ScreenState GetCurrentScreenType(ScreenManager* manager) {
	if (manager == NULL) return SS_INIT;
	return manager->currentScreen;
}

IScreen* GetActiveScreen(ScreenManager* manager) {
	if (manager == NULL || !IsValidScreenType(manager->currentScreen)) return NULL;
	return &manager->screens[manager->currentScreen];
}
