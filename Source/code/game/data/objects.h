// written by diskodev
// game/data/objects.h
#pragma once

#include <engine/engine_core.h>

#define OBJ_TILESET_W 160
#define OBJ_TILESET_H 160

typedef enum {
    OBJ_PLAYER,
    OBJ_LADDER,
    OBJ_SIGN,
    OBJ_SIGN_TRIGGER,
    OBJ_SPIKE_UP,
    OBJ_SPIKE_DOWN,
    OBJ_SPIKE_RIGHT,
    OBJ_SPIKE_LEFT,
    OBJ_SPRING_A,
    OBJ_SPRING_B,
    OBJ_BUTTON_UP,
    OBJ_BUTTON_DOWN,
    OBJ_PRESSURE_PLATE_UP,
    OBJ_PRESSURE_PLATE_DOWN,
    OBJ_LEVER_OFF,
    OBJ_LEVER_ON,
    OBJ_CRANK_FIXED,
    OBJ_CRANK_BROKEN,
    OBJ_CRATE,
    OBJ_BEAR_TRAP,
    OBJ_VENT,
    OBJ_ROPE,
    OBJ_PLATFORM_ACTIVE,
    OBJ_PLATFORM_INACTIVE,
    OBJ_DOOR_SUB,
    OBJ_DOOR_LOCKED,
    OBJ_DOOR_EXIT,
    OBJ_DOOR_ENTRANCE,
    OBJ_NOTE,
    OBJ_HEART,
    OBJ_SANITY_PILL,
    OBJ_KEY,
    OBJ_BROOM,
    OBJ_CROSS,
    OBJ_KNIFE,
    OBJ_HATCHET,
    OBJ_SMOKE_BOMB,
    OBJ_FLASHLIGHT,
    OBJ_FLASHLIGHT_BATTERY,
    OBJ_POTION,
    OBJ_CRANK_PIECE,
    OBJ_LIGHT_A,
    OBJ_LIGHT_B,
    OBJ_LIGHT_C,
    OBJ_LIGHT_D,

// more to follow later
    OBJ_BASE_COUNT // player is not included in this count
} ObjectType;

