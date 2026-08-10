// written by diskodev
// game/data/data.c

#include "data.h"

PlayerData playerData;
SaveData   saveData;
u32        notesFoundBitmask = 0;

static const char* s_areaNames[AREA_COUNT] = {
    "Mansion",
    "Caves",
    "Lab",
    "Circus",
    "Factory",
    "Hospital",
    "Asylum",
    "Aftermath",
};

const char* AreaType_Name(AreaType area) {
    if (area < 0 || area >= AREA_COUNT) return "Unknown";
    return s_areaNames[area];
}

static const char* s_itemNames[ITEM_COUNT] = {
    "None",
    "Key",
    "Keycard",
    "Crank Piece",
    "Broom",
    "Cross",
    "Knife",
    "Hatchet",
    "Smoke Bomb",
    "Potion",
    "Sanity Pill",
};

const char* ItemType_Name(ItemType item) {
    if (item < 0 || item >= ITEM_COUNT) return "None";
    return s_itemNames[item];
}

int PlayerData_NoteCount(const PlayerData* p) {
    int count = 0;
    for (int i = 0; i < MAX_NOTES; i++) {
        if (p->notes[i]) count++;
    }
    return count;
}
