// written by diskodev
// engine/edit/csv_read.c
#include "csv_read.h"

CSVData csvData = {0};

static bool ReadNextLine(PlaydateAPI* pd, SDFile* file, char* buffer, i32 bufferSize) {
    if (pd == NULL || file == NULL || buffer == NULL || bufferSize <= 1) return false;

    i32 index = 0;
    char ch = '\0';
    i32 bytesRead = 0;

    while (index < bufferSize - 1) {
        bytesRead = pd->file->read(file, &ch, 1);
        if (bytesRead <= 0) break;
        if (ch == '\r') continue;
        if (ch == '\n') break;
        buffer[index++] = ch;
    }

    buffer[index] = '\0';

    if (bytesRead <= 0 && index == 0) return false;
    return true;
}

CSVData* ReadCSVFile(PlaydateAPI* pd, cstr filepath) {
    if (pd == NULL || filepath == NULL) return NULL;

    // Reset CSV data
    FreeCSVData(pd, &csvData);

    // Open the file
    SDFile* file = pd->file->open(filepath, kFileRead);
    if (file == NULL) {
        LOG("ReadCSVData: failed to open '%s'", filepath);
        return NULL;
    }

    csvData.data = pd->system->realloc(NULL, sizeof(i32*) * MAX_CSV_ROWS);
    if (csvData.data == NULL) {
        pd->file->close(file);
        return NULL;
    }
    memset(csvData.data, 0, sizeof(i32*) * MAX_CSV_ROWS);

    // Read the file line by line
    char lineBuffer[256];
    while (ReadNextLine(pd, file, lineBuffer, (i32)sizeof(lineBuffer))) {
        if (lineBuffer[0] == '\0') continue;

        if (csvData.rows >= MAX_CSV_ROWS) {
            LOG("ReadCSVData: exceeded max rows (%d)", MAX_CSV_ROWS);
            break;
        }

        csvData.data[csvData.rows] = pd->system->realloc(NULL, sizeof(i32) * MAX_CSV_COLS);
        if (csvData.data[csvData.rows] == NULL) {
            pd->file->close(file);
            FreeCSVData(pd, &csvData);
            return NULL;
        }

        // Split the line into columns
        u8 colCount = 0;
        char* token = strtok(lineBuffer, ",");
        while (token != NULL && colCount < MAX_CSV_COLS) {
            csvData.data[csvData.rows][colCount] = atoi(token);
            colCount++;
            token = strtok(NULL, ",");
        }

        if (colCount > csvData.cols) {
            csvData.cols = colCount;
        }
        csvData.rows++;
    }

    pd->file->close(file);
    return &csvData;
}

void FreeCSVData(PlaydateAPI* pd, CSVData* csvData) {
    if (pd == NULL || csvData == NULL) return;

    for (u8 i = 0; i < csvData->rows; i++) {
        if (csvData->data[i] != NULL) {
            pd->system->realloc(csvData->data[i], 0);
        }
    }
    pd->system->realloc(csvData->data, 0);
    csvData->data = NULL;
    csvData->rows = 0;
    csvData->cols = 0;
}

i32 GetCSVValue(CSVData* csv, u8 row, u8 col) {
    if (csv == NULL || csv->data == NULL) return 0;
    if (row >= csv->rows || col >= csv->cols) return 0;
    return csv->data[row][col];
}