// written by diskodev
// engine/util.h
#pragma once

#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <pd_api.h>

extern PlaydateAPI* pd;

#define TRUE 1
#define FALSE 0

#define LOG(...) do { if (pd) pd->system->logToConsole(__VA_ARGS__); } while(0)

#define GAME_NAME "Lincoln's Labyrinthine"
#define GAME_VERSION "0.1.0"

#define TARGET_FPS 30
#define TARGET_FRAME_TIME (1.0f / TARGET_FPS)

#define RESOLUTION 1
#define SCR_W (400 * RESOLUTION)
#define SCR_H (240 * RESOLUTION)

#define TILE_SIZE 16

#define SCREEN_TILE_W (SCR_W / TILE_SIZE) // should be 25
#define SCREEN_TILE_H (SCR_H / TILE_SIZE) // should be 15

#define TILES_PER_SCREEN (SCREEN_TILE_W * SCREEN_TILE_H) // should be 375

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

typedef float f32;

#define U8_MIN 0
#define U8_MAX 255
#define U16_MIN 0
#define U16_MAX 65535
#define U32_MIN 0
#define U32_MAX 4294967295

#define I8_MIN -128
#define I8_MAX 127
#define I16_MIN -32768
#define I16_MAX 32767
#define I32_MIN -2147483648
#define I32_MAX 2147483647

#define F32_MIN -3.402823466e+38F
#define F32_MAX 3.402823466e+38F

// typedef double f64; // no 64bit support for playdate

typedef const char* cstr;

#define BPB 8 // bits per byte

#define DIV2 0.5
#define DIV4 0.25
#define DIV8 0.125
#define DIV16 0.0625
#define DIV32 0.03125
#define DIV64 0.015625
#define DIV128 0.0078125

#define PI 3.14159265358979323846f

typedef struct {
    f32 x;
    f32 y;
} Vec2;

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} Vec3;

typedef struct {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
} Rect;

typedef struct {
    f32 x;
    f32 y;
    f32 radx;
    f32 rady;
    f32 angle;
} Ellipse;

static inline f32 Clamp(f32 value, f32 min, f32 max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static inline f32 Lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

static inline f32 Mag2(f32 x, f32 y) {
    return sqrtf(x * x + y * y);
}

static inline f32 Mag3(f32 x, f32 y, f32 z) {
    return sqrtf(x * x + y * y + z * z);
}

static inline f32 Slope(f32 l, f32 r) {
    return (fabsf(l - r) * 180.0f);
}

static inline f32 Abs(f32 a) {
    return fabsf(a);
}

static f32 ClampedLerp(f32 a, f32 b, f32 t) {
    return Lerp(a, b, Clamp(t, 0.0f, 1.0f));
}

static inline f32 MinF(f32 x, f32 y) {
    return (x < y) ? x : y;
}

static inline f32 MaxF(f32 x, f32 y) {
    return (x > y) ? x : y;
}

static inline i32 MinI(i32 x, i32 y) {
    return (x < y) ? x : y;
}

static inline i32 MaxI(i32 x, i32 y) {
    return (x > y) ? x : y;
}

static inline u32 MinU(u32 x, u32 y) {
    return (x < y) ? x : y;
}

static inline u32 MaxU(u32 x, u32 y) {
    return (x > y) ? x : y;
}

static inline i32 SignOf8b(i8 a) {
    return (a & 0b10000000) ? -1 : 1;
}

static inline i32 SignOf16b(i16 a) {
    return (a & 0b1000000000000000) ? -1 : 1;
}

static inline i32 SignOf32b(i32 a) {
    return (a & 0b10000000000000000000000000000000) ? -1 : 1;
}

static inline Vec2 Vec2Add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

static inline Vec2 Vec2Sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

static inline Vec2 Vec2Mul(Vec2 a, f32 scalar) {
    return (Vec2){a.x * scalar, a.y * scalar};
}

static inline Vec2 Vec2Div(Vec2 a, f32 scalar) {
    return (Vec2){a.x / scalar, a.y / scalar};
}

static inline f32 Vec2Mag(Vec2 v) {
    return Mag2(v.x, v.y);
}

static inline Vec2 Vec2Normalize(Vec2 v) {
    f32 mag = Vec2Mag(v);
    if (mag == 0) return (Vec2){0, 0};
    return Vec2Div(v, mag);
}

static inline f32 Vec2Dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static inline f32 Vec2Cross(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}

static inline Vec2 Vec2Rotate(Vec2 v, f32 angle) {
    f32 rad = angle * (PI / 180.0f);
    f32 cosA = cosf(rad);
    f32 sinA = sinf(rad);
    return (Vec2){v.x * cosA - v.y * sinA, v.x * sinA + v.y * cosA};
}

#define DEBUG_OFF 0b00000000
#define DEBUG_HITBOX 0b00000001
#define DEBUG_PHYS 0b00000010
#define DEBUG_RENDER 0b00000100
#define DEBUG_AI 0b00001000
#define DEBUG_CARIC 0b00010000
#define DEBUG_ANIM  0b00100000
// add more later
#define DEBUG_ALL 0b11111111

#define DEBUG_ENABLED 1
#define DEBUG_DISABLED 0

static u8 GlobalDebug = DEBUG_CARIC|DEBUG_ANIM|DEBUG_PHYS;//DEBUG_COM|DEBUG_CRIT|DEBUG_VIS|DEBUG_PHYS|DEBUG_SOLE|DEBUG_CARIC|DEBUG_MOME|DEBUG_GAME;
static inline void printd(u8 priority, const char *message, ...)
{
  if(GlobalDebug & priority)
  {
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
  }
}

#define CARIC_DEPTH_MAX                 128

#define CARIC_FLAG_ENABLED              0b00000001
#define CARIC_FLAG_PERSISTENT           0b10000000

extern LCDFont* fontFamily[5];

extern LCDFont octoFont;
extern LCDFont resmontFont;
extern LCDFont sonicFont;
extern LCDFont storiesFont;
extern LCDFont billJillyFont;

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
#define GAME_VERSION_STR STRINGIFY(GAME_VERSION)
static inline u16 String_GetLength(cstr str) {
    return (u16)strlen(str);
}

static inline u16 String_Copy(cstr src, char* dest, u16 maxLength) {
    strncpy(dest, src, maxLength);
    dest[maxLength - 1] = '\0'; // Ensure null-termination
    return (u16)strlen(dest);
}

static inline u16 String_Compare(cstr str1, cstr str2) {
    return (u16)strcmp(str1, str2);
}

static inline u16 String_CompareIgnoreCase(cstr str1, cstr str2) {
    return (u16)strcasecmp(str1, str2);
}

#define ASSETS_PATH "assets/"

#define DATA_PATH ASSETS_PATH "data/"
#define BIN_PATH DATA_PATH "bin/"
#define CSV_PATH DATA_PATH "csv/"
#define SYNTH_PATH DATA_PATH "synth/"

#define AUDIO_PATH ASSETS_PATH "audio/"
#define AUDIO_MOD_PATH AUDIO_PATH "mod/"
#define AUDIO_SFX_PATH AUDIO_PATH "sfx/"
#define AUDIO_VOX_PATH AUDIO_PATH "vox/"
#define AUDIO_AMB_PATH AUDIO_PATH "amb/"

#define TEXTURE_PATH ASSETS_PATH "texture/"
#define SPRITE_PATH TEXTURE_PATH "sprite/"
#define TILESET_PATH TEXTURE_PATH "tileset/"
#define IMAGE_PATH TEXTURE_PATH "image/"

#define FONT_PATH ASSETS_PATH "font/"

#define FMV_PATH ASSETS_PATH "fmv/"
