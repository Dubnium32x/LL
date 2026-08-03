// written by diskodev
// engine/edit/csv_read.h
#pragma once

#include <engine/util.h>

#define MAX_CSV_ROWS U8_MAX
#define MAX_CSV_COLS U8_MAX

typedef struct {
    i32** data;
    u8 rows;
    u8 cols;
} CSVData;

extern CSVData csvData;

CSVData* ReadCSVFile(PlaydateAPI* pd, cstr filepath);
void FreeCSVData(PlaydateAPI* pd, CSVData* csvData);

i32 GetCSVValue(CSVData* csv, u8 row, u8 col);