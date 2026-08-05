// written by diskodev
// engine/asset.h
#pragma once

#include "util.h"

#define MAX_CACHED_BITMAPS 64
#define MAX_CACHED_BITMAP_TABLES 32
#define MAX_ASSET_PATH_LEN 128

typedef struct {
    char path[MAX_ASSET_PATH_LEN];
    LCDBitmap* bitmap;
    u16 refCount;
} CachedBitmap;

typedef struct {
    char path[MAX_ASSET_PATH_LEN];
    LCDBitmapTable* table;
    u16 refCount;
} CachedBitmapTable;

typedef struct {
    CachedBitmap bitmaps[MAX_CACHED_BITMAPS];
    u16 bitmapCount;

    CachedBitmapTable tables[MAX_CACHED_BITMAP_TABLES];
    u16 tableCount;
} AssetManager;

extern AssetManager assetManager;

void Asset_InitManager(void);
void Asset_FreeManager(void);

LCDBitmap* Asset_LoadBitmap(cstr path);
void Asset_FreeBitmap(LCDBitmap* bitmap);

LCDBitmapTable* Asset_LoadBitmapTable(cstr path);
void Asset_FreeBitmapTable(LCDBitmapTable* table);

void Asset_UnloadUnused(void);
void Asset_ClearCache(void);
void Asset_PrintStats(void);
