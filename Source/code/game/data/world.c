// written by diskodev
// game/data/world.c
#include "world.h"
#include <engine/edit/csv_read.h>

World gWorld = {0};

void World_Init(PlaydateAPI* pd, World* world, cstr tileTablePath) {
    if (pd == NULL || world == NULL) return;

    memset(world, 0, sizeof(World));

    if (tileTablePath != NULL) {
        world->tileTable = Asset_LoadBitmapTable(tileTablePath);
        if (world->tileTable == NULL) {
            LOG("World_Init: failed to load tiletable '%s'", tileTablePath);
        }
    }
}

bool World_LoadCSV(PlaydateAPI* pd, World* world, cstr layerPaths[LAYER_COUNT]) {
    if (pd == NULL || world == NULL || layerPaths == NULL) return false;

    // Reset current tile array
    memset(world->tiles, 0, sizeof(world->tiles));

    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        // Initialize layer with empty tiles
        for (u8 y = 0; y < WORLD_HEIGHT_TILES; y++) {
            for (u8 x = 0; x < WORLD_WIDTH_TILES; x++) {
                world->tiles[layer][y][x] = EMPTY_TILE;
            }
        }

        cstr path = layerPaths[layer];
        if (path == NULL || strlen(path) == 0) {
            continue;
        }

        CSVData* csv = ReadCSVFile(pd, path);
        if (csv == NULL) {
            LOG("World_LoadCSV: failed to read CSV layer %d at path '%s'", layer, path);
            continue;
        }

        u8 targetRows = csv->rows < WORLD_HEIGHT_TILES ? csv->rows : WORLD_HEIGHT_TILES;
        u8 targetCols = csv->cols < WORLD_WIDTH_TILES ? csv->cols : WORLD_WIDTH_TILES;

        for (u8 r = 0; r < targetRows; r++) {
            for (u8 c = 0; c < targetCols; c++) {
                world->tiles[layer][r][c] = GetCSVValue(csv, r, c);
            }
        }

        FreeCSVData(pd, csv);
    }

    world->isLoaded = true;
    return true;
}

void World_Draw(PlaydateAPI* pd, World* world, i32 cameraX, i32 cameraY) {
    if (pd == NULL || world == NULL || !world->isLoaded || world->tileTable == NULL) return;

    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        if (layer == LAYER_COLLISION) continue;
        for (u8 y = 0; y < WORLD_HEIGHT_TILES; y++) {
            for (u8 x = 0; x < WORLD_WIDTH_TILES; x++) {
                i32 tileId = world->tiles[layer][y][x];
                if (tileId < 0) continue;
                LCDBitmap* tileImg = pd->graphics->getTableBitmap(world->tileTable, tileId);
                if (tileImg != NULL) {
                    i32 drawX = (x * TILE_SIZE) - cameraX;
                    i32 drawY = (y * TILE_SIZE) - cameraY;
                    pd->graphics->drawBitmap(tileImg, drawX, drawY, kBitmapUnflipped);
                }
            }
        }
    }
}

void World_DrawLayer(PlaydateAPI* pd, World* world, WorldLayer layer, i32 cameraX, i32 cameraY) {
    if (pd == NULL || world == NULL || !world->isLoaded || world->tileTable == NULL) return;
    if (layer >= LAYER_COUNT || layer == LAYER_COLLISION) return;
    for (u8 y = 0; y < WORLD_HEIGHT_TILES; y++) {
        for (u8 x = 0; x < WORLD_WIDTH_TILES; x++) {
            i32 tileId = world->tiles[layer][y][x];
            if (tileId < 0) continue;
            LCDBitmap* tileImg = pd->graphics->getTableBitmap(world->tileTable, tileId);
            if (tileImg != NULL) {
                pd->graphics->drawBitmap(tileImg, x * TILE_SIZE - cameraX, y * TILE_SIZE - cameraY, kBitmapUnflipped);
            }
        }
    }
}

CollisionType World_GetCollisionType(World* world, i32 tileX, i32 tileY) {
    if (world == NULL || tileX < 0 || tileY < 0
        || tileX >= WORLD_WIDTH_TILES || tileY >= WORLD_HEIGHT_TILES)
        return COL_SOLID;
    i32 val = world->tiles[LAYER_COLLISION][tileY][tileX];
    if (val < 0 || val >= COL_TYPE_COUNT) return COL_EMPTY;
    CollisionType ct = (CollisionType)val;
    if (ct == COL_NULL) return COL_EMPTY;
    return ct;
}

i32 World_GetTile(World* world, WorldLayer layer, u8 x, u8 y) {
    if (world == NULL || layer >= LAYER_COUNT) return EMPTY_TILE;
    if (x >= WORLD_WIDTH_TILES || y >= WORLD_HEIGHT_TILES) return EMPTY_TILE;

    return world->tiles[layer][y][x];
}

void World_Free(PlaydateAPI* pd, World* world) {
    if (pd == NULL || world == NULL) return;

    if (world->tileTable != NULL) {
        Asset_FreeBitmapTable(world->tileTable);
        world->tileTable = NULL;
    }

    world->isLoaded = false;
}
