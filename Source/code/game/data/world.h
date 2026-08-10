// written by diskodev
// game/data/world.h
#pragma once

#include <engine/engine_core.h>

#define USE_CSV TRUE
#define USE_BIN FALSE

typedef enum {
    LAYER_BG,
    LAYER_CENTER,
    LAYER_OBJECTS,
    LAYER_FG,
    LAYER_COLLISION,
    LAYER_COUNT
} WorldLayer;

typedef enum {
    COL_NULL        = 0,  // passable (matches PHYSICS_TILE_NULL)
    COL_HALF_BOTTOM = 1,  // solid lower-half tile
    COL_SLOPE_UL    = 2,  // 1x1 \ slope, surfY = x
    COL_SLOPE_UL_LO = 3,  // 1x2 \ lower half
    COL_SLOPE_UL_HI = 4,  // 1x2 \ upper half
    COL_SOLID       = 5,  // full block
    COL_SOLID_DUP   = 6,  // full block (visual duplicate)
    COL_SLOPE_UR_LO = 7,  // 1x2 / lower half
    COL_SLOPE_UR_HI = 8,  // 1x2 / upper half
    COL_SLOPE_UR    = 9,  // 1x1 / slope, surfY = tileSize - x
    COL_SLOPE_DR_LO = 10, // ceiling slopes — not used for floor physics
    COL_SLOPE_DR_HI = 11,
    COL_SLOPE_DR    = 12,
    COL_SLOPE_DL    = 13,
    COL_SLOPE_DL_LO = 14,
    COL_SLOPE_DL_HI = 15,
    COL_PLATFORM    = 16, // one-way semi-solid, solid from above only
    COL_EMPTY       = 17, // passable (matches PHYSICS_TILE_EMPTY)
    COL_TYPE_COUNT
} CollisionType;

#define WORLD_WIDTH_TILES  24
#define WORLD_HEIGHT_TILES 13

// remember, TILE_SIZE = 16, so WORLD_WIDTH = 384, WORLD_HEIGHT = 208

#define WORLD_WIDTH_PIXELS (WORLD_WIDTH_TILES*TILE_SIZE)
#define WORLD_HEIGHT_PIXELS (WORLD_HEIGHT_TILES*TILE_SIZE)

#define EMPTY_TILE -1 // start of range
#define MAX_TILE 255+EMPTY_TILE // end of range

#define LAYER_1_BITMASK 0xFF // bits 1 - 8
#define LAYER_2_BITMASK 0xFF00 // bits 9 - 16
#define LAYER_3_BITMASK 0xFF0000 // bits 17 - 24
#define LAYER_4_BITMASK 0xFF000000 // bits 25 - 32

#define LAYER_1_SHIFT 0
#define LAYER_2_SHIFT 8
#define LAYER_3_SHIFT 16
#define LAYER_4_SHIFT 24

// everything up to this point wont matter if USE_BIN is false.
// if USE_BIN is true, then we will use a binary file to load the world data
// otherwise, use CSV load for level design.

typedef struct {
    i32 tiles[LAYER_COUNT][WORLD_HEIGHT_TILES][WORLD_WIDTH_TILES];
    LCDBitmapTable* tileTable;
    u8 loadedRoom;
    bool isLoaded;
} World;

extern World gWorld;

void World_Init(PlaydateAPI* pd, World* world, cstr tileTablePath);
bool World_LoadCSV(PlaydateAPI* pd, World* world, cstr layerPaths[LAYER_COUNT]);
void World_Draw(PlaydateAPI* pd, World* world, i32 cameraX, i32 cameraY);
void World_DrawLayer(PlaydateAPI* pd, World* world, WorldLayer layer, i32 cameraX, i32 cameraY);
CollisionType World_GetCollisionType(World* world, i32 tileX, i32 tileY);
i32 World_GetTile(World* world, WorldLayer layer, u8 x, u8 y);
void World_Free(PlaydateAPI* pd, World* world);

