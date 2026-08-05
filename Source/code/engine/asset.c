// written by diskodev
// engine/asset.c
#include "asset.h"

AssetManager assetManager = {0};

void Asset_InitManager(void) {
    memset(&assetManager, 0, sizeof(AssetManager));
}

LCDBitmap* Asset_LoadBitmap(cstr path) {
    if (pd == NULL || path == NULL || strlen(path) == 0) return NULL;

    // Check if bitmap is already loaded in cache
    for (u16 i = 0; i < assetManager.bitmapCount; i++) {
        if (assetManager.bitmaps[i].bitmap != NULL &&
            strncmp(assetManager.bitmaps[i].path, path, MAX_ASSET_PATH_LEN) == 0) {
            assetManager.bitmaps[i].refCount++;
            return assetManager.bitmaps[i].bitmap;
        }
    }

    // Load new bitmap from file
    cstr error = NULL;
    LCDBitmap* bitmap = pd->graphics->loadBitmap(path, &error);
    if (bitmap == NULL) {
        LOG("Asset_LoadBitmap: failed to load '%s': %s", path, error ? error : "unknown error");
        return NULL;
    }

    // Find empty slot or use new slot
    i16 slotIndex = -1;
    for (u16 i = 0; i < assetManager.bitmapCount; i++) {
        if (assetManager.bitmaps[i].bitmap == NULL) {
            slotIndex = (i16)i;
            break;
        }
    }

    if (slotIndex == -1) {
        if (assetManager.bitmapCount >= MAX_CACHED_BITMAPS) {
            LOG("Asset_LoadBitmap: cache full (max %d)", MAX_CACHED_BITMAPS);
            pd->graphics->freeBitmap(bitmap);
            return NULL;
        }
        slotIndex = (i16)assetManager.bitmapCount++;
    }

    strncpy(assetManager.bitmaps[slotIndex].path, path, MAX_ASSET_PATH_LEN - 1);
    assetManager.bitmaps[slotIndex].path[MAX_ASSET_PATH_LEN - 1] = '\0';
    assetManager.bitmaps[slotIndex].bitmap = bitmap;
    assetManager.bitmaps[slotIndex].refCount = 1;

    return bitmap;
}

void Asset_FreeBitmap(LCDBitmap* bitmap) {
    if (bitmap == NULL) return;

    for (u16 i = 0; i < assetManager.bitmapCount; i++) {
        if (assetManager.bitmaps[i].bitmap == bitmap) {
            if (assetManager.bitmaps[i].refCount > 0) {
                assetManager.bitmaps[i].refCount--;
            }
            if (assetManager.bitmaps[i].refCount == 0) {
                if (pd != NULL && pd->graphics != NULL) {
                    pd->graphics->freeBitmap(assetManager.bitmaps[i].bitmap);
                }
                assetManager.bitmaps[i].bitmap = NULL;
                assetManager.bitmaps[i].path[0] = '\0';
            }
            return;
        }
    }
}

LCDBitmapTable* Asset_LoadBitmapTable(cstr path) {
    if (pd == NULL || path == NULL || strlen(path) == 0) return NULL;

    // Check if bitmap table is already loaded in cache
    for (u16 i = 0; i < assetManager.tableCount; i++) {
        if (assetManager.tables[i].table != NULL &&
            strncmp(assetManager.tables[i].path, path, MAX_ASSET_PATH_LEN) == 0) {
            assetManager.tables[i].refCount++;
            return assetManager.tables[i].table;
        }
    }

    // Load new bitmap table from file
    cstr error = NULL;
    LCDBitmapTable* table = pd->graphics->loadBitmapTable(path, &error);
    if (table == NULL) {
        LOG("Asset_LoadBitmapTable: failed to load '%s': %s", path, error ? error : "unknown error");
        return NULL;
    }

    // Find empty slot or use new slot
    i16 slotIndex = -1;
    for (u16 i = 0; i < assetManager.tableCount; i++) {
        if (assetManager.tables[i].table == NULL) {
            slotIndex = (i16)i;
            break;
        }
    }

    if (slotIndex == -1) {
        if (assetManager.tableCount >= MAX_CACHED_BITMAP_TABLES) {
            LOG("Asset_LoadBitmapTable: cache full (max %d)", MAX_CACHED_BITMAP_TABLES);
            pd->graphics->freeBitmapTable(table);
            return NULL;
        }
        slotIndex = (i16)assetManager.tableCount++;
    }

    strncpy(assetManager.tables[slotIndex].path, path, MAX_ASSET_PATH_LEN - 1);
    assetManager.tables[slotIndex].path[MAX_ASSET_PATH_LEN - 1] = '\0';
    assetManager.tables[slotIndex].table = table;
    assetManager.tables[slotIndex].refCount = 1;

    return table;
}

void Asset_FreeBitmapTable(LCDBitmapTable* table) {
    if (table == NULL) return;

    for (u16 i = 0; i < assetManager.tableCount; i++) {
        if (assetManager.tables[i].table == table) {
            if (assetManager.tables[i].refCount > 0) {
                assetManager.tables[i].refCount--;
            }
            if (assetManager.tables[i].refCount == 0) {
                if (pd != NULL && pd->graphics != NULL) {
                    pd->graphics->freeBitmapTable(assetManager.tables[i].table);
                }
                assetManager.tables[i].table = NULL;
                assetManager.tables[i].path[0] = '\0';
            }
            return;
        }
    }
}

void Asset_UnloadUnused(void) {
    if (pd == NULL || pd->graphics == NULL) return;

    // Free all zero refCount bitmaps
    for (u16 i = 0; i < assetManager.bitmapCount; i++) {
        if (assetManager.bitmaps[i].bitmap != NULL && assetManager.bitmaps[i].refCount == 0) {
            pd->graphics->freeBitmap(assetManager.bitmaps[i].bitmap);
            assetManager.bitmaps[i].bitmap = NULL;
            assetManager.bitmaps[i].path[0] = '\0';
        }
    }

    // Free all zero refCount bitmap tables
    for (u16 i = 0; i < assetManager.tableCount; i++) {
        if (assetManager.tables[i].table != NULL && assetManager.tables[i].refCount == 0) {
            pd->graphics->freeBitmapTable(assetManager.tables[i].table);
            assetManager.tables[i].table = NULL;
            assetManager.tables[i].path[0] = '\0';
        }
    }
}

void Asset_ClearCache(void) {
    if (pd == NULL || pd->graphics == NULL) return;

    for (u16 i = 0; i < assetManager.bitmapCount; i++) {
        if (assetManager.bitmaps[i].bitmap != NULL) {
            pd->graphics->freeBitmap(assetManager.bitmaps[i].bitmap);
        }
    }

    for (u16 i = 0; i < assetManager.tableCount; i++) {
        if (assetManager.tables[i].table != NULL) {
            pd->graphics->freeBitmapTable(assetManager.tables[i].table);
        }
    }

    memset(&assetManager, 0, sizeof(AssetManager));
}

void Asset_PrintStats(void) {
    u16 activeBitmaps = 0;
    u16 activeTables = 0;

    for (u16 i = 0; i < assetManager.bitmapCount; i++) {
        if (assetManager.bitmaps[i].bitmap != NULL) activeBitmaps++;
    }

    for (u16 i = 0; i < assetManager.tableCount; i++) {
        if (assetManager.tables[i].table != NULL) activeTables++;
    }

    LOG("AssetManager Stats: %d active bitmaps, %d active bitmap tables", activeBitmaps, activeTables);
}

void Asset_FreeManager(void) {
    Asset_ClearCache();
}
