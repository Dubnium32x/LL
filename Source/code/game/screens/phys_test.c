// written by diskodev
// game/screens/phys_test.c
// Physics + collision showcase. Debug geometry overlay + tenkopd-reference slope physics.
#include "phys_test.h"
#include <engine/engine_core.h>
#include <engine/signage.h>
#include <engine/audio.h>
#include <engine/visual.h>
#include <game/data/world.h>
#include <game/data/objects.h>
#include <math.h>

extern PlaydateAPI*   pd;
extern InputManager   inputManager;
extern AudioManager   audioManager;
extern ScreenManager  screenManager;

#define SFX_JUMP1   "assets/audio/sfx/jump_one.wav"
#define SFX_JUMP2   "assets/audio/sfx/jump_two.wav"
#define SFX_LAND    "assets/audio/sfx/land.wav"
#define SFX_STEP    "assets/audio/sfx/step.wav"
#define SFX_SPRING  "assets/audio/sfx/spring.wav"
#define SFX_CRATE   "assets/audio/sfx/crate_move.wav"

#define PT_STEP_INTERVAL_WALK 0.36f
#define PT_STEP_INTERVAL_RUN  0.20f
#define PT_STEP_FRAME         1  // second frame (0-based) of walk/run triggers step sound

// ---- Physics constants ----
#define PT_GRAVITY        780.0f
#define PT_JUMP_VEL      -260.0f
#define PT_DOUBLE_JUMP_VEL -300.0f
#define PT_MOVE_SPEED      60.0f
#define PT_RUN_SPEED      110.0f
#define PT_DOUBLE_TAP_FRAMES 8   // window to register double-tap direction
#define PT_MAX_FALL       460.0f
#define PT_BODY_W          10.0f
#define PT_BODY_H          14.0f
#define PT_SPAWN_X         32.0f
#define PT_SPAWN_Y        162.0f
#define PT_SLOPE_SNAP       4.0f   // px snap window for slope landing
#define PT_SPRING_VEL    -420.0f
#define PT_SPRING_FRAMES    6    // frames to show compressed spring sprite

typedef struct {
    f32 x, y, prevY;
    f32 vx, vy;
    bool onGround;
    bool facingRight;
    bool dropThrough;
    bool canDoubleJump;
    bool isRunning;
    i32  tapDir;          // last tapped direction: -1/0/1
    i32  tapFrames;       // frames since last tap (counts up)
    i32  tapDownFrames;   // frames since last down-tap
} PhysBody;

// ---- Animation ----
typedef enum { PS_IDLE, PS_WALK, PS_RUN, PS_CROUCH, PS_JUMP, PS_FALL } PlayerAnimState;

static AnimationSet    s_animSet;
static AnimationState  s_animState;
static PlayerAnimState s_animCur;

// ---- Crate physics ----
#define PT_MAX_CRATES    8
#define PT_CRATE_SIZE   ((f32)TILE_SIZE)
#define PT_CRATE_FRICTION 0.97f  // lateral damping when grounded
#define PT_CRATE_PUSH_VEL   22.0f  // velocity transferred to crate on player contact
#define PT_PUSH_SPEED        9.0f   // player max speed while pushing a crate
#define PT_STAMINA_DRAIN      0.6f  // per second while pushing
#define PT_STAMINA_REGEN      0.35f // per second while not pushing
#define PT_STAMINA_EXHAUST_T  0.8f  // seconds of lockout after stamina empties

typedef struct {
    f32 x, y, prevY;
    f32 vx, vy;
    bool onGround;
    bool active;
} CrateBody;

static CrateBody s_crates[PT_MAX_CRATES];
static i32       s_crateCount;
static f32       s_stamina      = 1.0f;  // 0..1
static f32       s_exhaustTimer = 0.0f;  // lockout countdown after depletion
static bool      s_isPushing    = false;
static bool      s_runLocked    = false; // locked out of running until stamina recovers past blink
static bool      s_crateMoving  = false;

// ---- Dust particles ----
#define PT_MAX_DUST 32
typedef struct {
    f32 x, y, vx, vy;
    f32 life;   // 0..1, counts down
    bool active;
} DustParticle;
static DustParticle s_dust[PT_MAX_DUST];
static f32 s_dustSpawnTimer = 0.0f;

static void SpawnDust(f32 x, f32 y) {
    for (i32 i = 0; i < PT_MAX_DUST; i++) {
        if (s_dust[i].active) continue;
        s_dust[i].x       = x + ((f32)(pd->system->getCurrentTimeMilliseconds() % 7) - 3.0f);
        s_dust[i].y       = y;
        s_dust[i].vx      = ((f32)((pd->system->getCurrentTimeMilliseconds()*3) % 11) - 5.0f) * 0.5f;
        s_dust[i].vy      = -8.0f - (f32)((pd->system->getCurrentTimeMilliseconds()*7) % 8);
        s_dust[i].life    = 1.0f;
        s_dust[i].active  = true;
        return;
    }
}

// ---- State ----
static PhysBody        s_body;
static LCDBitmapTable* s_objTable;
static LCDBitmap*      s_uiBorder = NULL;
static PDMenuItem*     s_menuRestart = NULL;

static void PT_RestartCallback(void* ud) {
    (void)ud;
    PhysTestScreen_Unload();
    PhysTestScreen_Init();
}
static i32 s_springTx = -1, s_springTy = -1, s_springFrames = 0;
static bool s_buttonActivated = false;
static bool s_inVent          = false;
static i32  s_ventTx = -1, s_ventTy = -1;
static bool s_wasGrounded     = false;
static u8   s_prevFramePos    = 0xFF;
static f32  s_sanityLevel     = 0.0f;
static f32  s_sanityTime      = 0.0f;
static f32  s_smokeTimer      = 0.0f;

// OBJ_LIGHT_A positions scanned at Init
#define PT_MAX_LIGHTS 8
static i32 s_lightCX[PT_MAX_LIGHTS];
static i32 s_lightCY[PT_MAX_LIGHTS];
static i32 s_lightCount = 0;

// Smoke cloud platform anchors (hand-placed at semi-solid platform tops)
typedef struct { i32 cx, topY, w; } SmokePlatform;
static const SmokePlatform s_smokePlatforms[] = {
    { 104, 80, 48 },   // row-5 semi-solid platform (tx 5-7)
    { 232, 64, 48 },   // row-4 semi-solid platform (tx 13-15)
};
#define SMOKE_PLATFORM_COUNT 2

// ---- Light-rendering helpers (mirrors light_test.c) ----
static const uint8_t kLPat75[16] = {
    0xFF,0xEE,0xFF,0xBB,0xFF,0xEE,0xFF,0xBB,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t kLPat50[16] = {
    0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t kLPat25[16] = {
    0x88,0x00,0x22,0x00,0x88,0x00,0x22,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t kLPat12[16] = {
    0x80,0x00,0x08,0x00,0x80,0x00,0x08,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

static void PT_ScanlineDither(i32 x, i32 y, i32 w, f32 density) {
    if (w <= 0) return;
    LCDColor col;
    if      (density > 0.87f) col = kColorWhite;
    else if (density > 0.62f) col = (LCDColor)(uintptr_t)kLPat75;
    else if (density > 0.37f) col = (LCDColor)(uintptr_t)kLPat50;
    else if (density > 0.18f) col = (LCDColor)(uintptr_t)kLPat25;
    else if (density > 0.06f) col = (LCDColor)(uintptr_t)kLPat12;
    else                      return;
    pd->graphics->fillRect(x, y, w, 1, col);
}

static void PT_DrawRadialLight(i32 cx, i32 cy, i32 radius, f32 time) {
    i32 r = radius + (i32)(sinf(time * 1.8f) * 3.0f);
    i32 r2 = r * r;
    for (i32 y = cy - r; y <= cy + r; y++) {
        i32 dy = y - cy;
        i32 span = (i32)sqrtf((f32)(r2 - dy * dy));
        if (span <= 0) continue;
        f32 distEdge = (f32)dy / (f32)r;
        f32 centerDensity = 1.0f - fabsf(distEdge);
        i32 rInner = (i32)(r * 0.25f), rMid = (i32)(r * 0.55f);
        i32 rInnerSpan = 0, rMidSpan = 0;
        { i32 ri2 = rInner*rInner - dy*dy; if (ri2>0) rInnerSpan=(i32)sqrtf((f32)ri2); }
        { i32 rm2 = rMid*rMid   - dy*dy; if (rm2>0) rMidSpan  =(i32)sqrtf((f32)rm2);  }
        if (span > rMidSpan) {
            PT_ScanlineDither(cx-span,     y, span-rMidSpan,   0.15f*centerDensity+0.05f);
            PT_ScanlineDither(cx+rMidSpan, y, span-rMidSpan,   0.15f*centerDensity+0.05f);
        }
        if (rMidSpan > rInnerSpan) {
            PT_ScanlineDither(cx-rMidSpan,   y, rMidSpan-rInnerSpan, 0.45f);
            PT_ScanlineDither(cx+rInnerSpan, y, rMidSpan-rInnerSpan, 0.45f);
        }
        if (rInnerSpan > 2) {
            f32 shimmer = 0.80f + sinf(time*4.3f+(f32)y*0.7f)*0.08f;
            PT_ScanlineDither(cx-rInnerSpan, y, rInnerSpan*2, shimmer);
        }
        if (dy == 0 || dy == -1) PT_ScanlineDither(cx-2, y, 4, 1.0f);
    }
}

// Matches tenkopd GetSlopeSurfaceY exactly.
static f32 SlopeSurfaceY(CollisionType ct, f32 localX) {
    f32 x    = localX < 0.0f ? 0.0f : (localX > (f32)TILE_SIZE ? (f32)TILE_SIZE : localX);
    f32 half = (f32)TILE_SIZE * 0.5f;
    switch (ct) {
        case COL_SLOPE_UL:    return (f32)TILE_SIZE - x;          // / full
        case COL_SLOPE_UL_LO: return (f32)TILE_SIZE - (x * 0.5f); // / lower half
        case COL_SLOPE_UL_HI: return half - (x * 0.5f);           // / upper half
        case COL_SLOPE_UR_LO: return x * 0.5f;                    // \ lower half
        case COL_SLOPE_UR_HI: return half + (x * 0.5f);           // \ upper half
        case COL_SLOPE_UR:    return x;                            // \ full
        default:               return (f32)TILE_SIZE;
    }
}

static bool IsSlope(CollisionType ct) {
    switch (ct) {
        case COL_SLOPE_UL: case COL_SLOPE_UL_LO: case COL_SLOPE_UL_HI:
        case COL_SLOPE_UR_LO: case COL_SLOPE_UR_HI: case COL_SLOPE_UR:
            return true;
        default: return false;
    }
}

// ---- Collision helpers ----
static CollisionType TileAt(i32 tx, i32 ty) {
    return World_GetCollisionType(&gWorld, tx, ty);
}

static bool IsSolid(CollisionType ct) {
    return ct == COL_SOLID || ct == COL_SOLID_DUP;
}

// Returns true if stepping up tileTop is possible (space above is clear).
static bool TryStepUp(PhysBody* b, f32 tileTop) {
    if (b->vy < -5.0f) return false;  // don't step up while moving upward (e.g. spring launch)
    f32 stepH = (b->y + PT_BODY_H) - tileTop;
    if (stepH <= 0.0f || stepH > (f32)TILE_SIZE) return false;
    f32 newY  = tileTop - PT_BODY_H;
    i32 tTop  = (i32)(newY / (f32)TILE_SIZE);
    i32 tBot  = (i32)((newY + PT_BODY_H - 0.5f) / (f32)TILE_SIZE);
    i32 tL    = (i32)(b->x / (f32)TILE_SIZE);
    i32 tR    = (i32)((b->x + PT_BODY_W - 0.5f) / (f32)TILE_SIZE);
    for (i32 cy = tTop; cy <= tBot; cy++)
        for (i32 cx = tL; cx <= tR; cx++)
            if (IsSolid(TileAt(cx, cy))) return false;
    b->y = newY;
    if (b->vy > 0.0f) b->vy = 0.0f;
    b->onGround = true;
    return true;
}

// Returns walkable surface Y for objects that block lateral movement, or -1 if none.
static f32 ObjectSolidSurface(i32 tx, i32 ty) {
    if (tx < 0 || tx >= WORLD_WIDTH_TILES || ty < 0 || ty >= WORLD_HEIGHT_TILES) return -1.0f;
    i32 obj = gWorld.tiles[LAYER_OBJECTS][ty][tx];
    f32 t   = (f32)(ty * TILE_SIZE);
    if (obj == OBJ_BUTTON_UP)   return t + (f32)TILE_SIZE * 0.50f;
    if (obj == OBJ_BUTTON_DOWN) return t + (f32)TILE_SIZE * 0.75f;
    return -1.0f;
}

static void ResolveX(PhysBody* b) {
    i32 top = (i32)( b->y                  / (f32)TILE_SIZE);
    i32 bot = (i32)((b->y + PT_BODY_H - 0.5f) / (f32)TILE_SIZE);
    if (b->vx > 0.0f) {
        i32 right = (i32)((b->x + PT_BODY_W - 0.5f) / (f32)TILE_SIZE);
        for (i32 ty = top; ty <= bot; ty++) {
            CollisionType ct = TileAt(right, ty);
            f32 objSurf = (ct == COL_EMPTY) ? ObjectSolidSurface(right, ty) : -1.0f;
            // overlap: player body must enter the solid region [objSurf, tileBottom)
            bool objBlock = objSurf >= 0.0f
                && b->y < (f32)(ty * TILE_SIZE + TILE_SIZE)
                && b->y + PT_BODY_H > objSurf + 0.5f;
            if (IsSolid(ct) || ct == COL_HALF_BOTTOM || objBlock) {
                if (!objBlock) {
                    f32 tileTop = (f32)(ty * TILE_SIZE);
                    f32 surface = tileTop + (ct == COL_HALF_BOTTOM ? (f32)TILE_SIZE * 0.5f : 0.0f);
                    if (TryStepUp(b, surface)) return;
                }
                b->x  = (f32)(right * TILE_SIZE) - PT_BODY_W;
                b->vx = 0.0f;
                return;
            }
        }
    } else if (b->vx < 0.0f) {
        i32 left = (i32)(b->x / (f32)TILE_SIZE);
        for (i32 ty = top; ty <= bot; ty++) {
            CollisionType ct = TileAt(left, ty);
            f32 objSurf = (ct == COL_EMPTY) ? ObjectSolidSurface(left, ty) : -1.0f;
            // overlap: player body must enter the solid region [objSurf, tileBottom)
            bool objBlock = objSurf >= 0.0f
                && b->y < (f32)(ty * TILE_SIZE + TILE_SIZE)
                && b->y + PT_BODY_H > objSurf + 0.5f;
            if (IsSolid(ct) || ct == COL_HALF_BOTTOM || objBlock) {
                if (!objBlock) {
                    f32 tileTop = (f32)(ty * TILE_SIZE);
                    f32 surface = tileTop + (ct == COL_HALF_BOTTOM ? (f32)TILE_SIZE * 0.5f : 0.0f);
                    if (TryStepUp(b, surface)) return;
                }
                b->x  = (f32)((left + 1) * TILE_SIZE);
                b->vx = 0.0f;
                return;
            }
        }
    }
}

// Returns true if the player is adjacent to an OBJ_VENT tile.
static bool NearVent(i32* outTx, i32* outTy) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] != OBJ_VENT) continue;
            f32 sx = (f32)(tx * TILE_SIZE), sy = (f32)(ty * TILE_SIZE);
            f32 dx = (s_body.x + PT_BODY_W * 0.5f) - (sx + (f32)TILE_SIZE * 0.5f);
            f32 dy = (s_body.y + PT_BODY_H * 0.5f) - (sy + (f32)TILE_SIZE * 0.5f);
            if (fabsf(dx) < (f32)TILE_SIZE * 0.7f && fabsf(dy) < (f32)TILE_SIZE * 0.7f) {
                if (outTx) *outTx = tx;
                if (outTy) *outTy = ty;
                return true;
            }
        }
    }
    return false;
}

// Returns true if the player AABB is within one tile of an OBJ_SIGN tile.
static bool NearSign(i32* outTx, i32* outTy) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] != OBJ_SIGN) continue;
            f32 sx = (f32)(tx * TILE_SIZE), sy = (f32)(ty * TILE_SIZE);
            f32 dx = (s_body.x + PT_BODY_W * 0.5f) - (sx + (f32)TILE_SIZE * 0.5f);
            f32 dy = (s_body.y + PT_BODY_H * 0.5f) - (sy + (f32)TILE_SIZE * 0.5f);
            if (dx > -(f32)TILE_SIZE * 0.7f && dx < (f32)TILE_SIZE * 0.7f &&
                dy > -(f32)TILE_SIZE * 0.7f && dy < (f32)TILE_SIZE * 0.7f) {
                if (outTx) *outTx = tx;
                if (outTy) *outTy = ty;
                return true;
            }
        }
    }
    return false;
}

static void ResolveY(PhysBody* b) {
    f32 inset  = 1.0f;
    f32 sampleX[3] = {
        b->x + inset,
        b->x + PT_BODY_W * 0.5f,
        b->x + PT_BODY_W - inset,
    };

    if (b->vy >= 0.0f) {
        f32 feet = b->y + PT_BODY_H;
        i32 botMin = (i32)floorf((feet - (f32)TILE_SIZE) / (f32)TILE_SIZE);
        i32 botMax = (i32)floorf((feet + PT_SLOPE_SNAP)   / (f32)TILE_SIZE);
        f32 bestSurface = 1e9f;
        bool found = false;

        for (int s = 0; s < 3; s++) {
          for (i32 bot = botMin; bot <= botMax; bot++) {
            i32 tx = (i32)(sampleX[s] / (f32)TILE_SIZE);
            CollisionType ct = TileAt(tx, bot);

            f32 tileTop = (f32)(bot * TILE_SIZE);
            f32 surface = tileTop;

            if (ct == COL_EMPTY) {
                // buttons provide their own collision surface
                if (bot < 0 || bot >= WORLD_HEIGHT_TILES || tx < 0 || tx >= WORLD_WIDTH_TILES) continue;
                i32 obj = gWorld.tiles[LAYER_OBJECTS][bot][tx];
                if (obj == OBJ_BUTTON_UP)   surface = tileTop + (f32)TILE_SIZE * 0.50f;
                else if (obj == OBJ_BUTTON_DOWN) surface = tileTop + (f32)TILE_SIZE * 0.75f;
                else continue;
                if (b->dropThrough) continue;
                if (b->prevY + PT_BODY_H > surface + 2.0f) continue;
            } else if (IsSolid(ct)) {
                // skip if player is a full tile or more below surface — catches ceilings, allows fast falls
                if (b->y + PT_BODY_H >= surface + (f32)TILE_SIZE) continue;
            } else if (ct == COL_HALF_BOTTOM) {
                surface += (f32)TILE_SIZE * 0.5f;
            } else if (ct == COL_PLATFORM) {
                if (b->dropThrough) continue;
                if (b->prevY + PT_BODY_H > tileTop + 2.0f) continue;
            } else if (IsSlope(ct)) {
                f32 localX = sampleX[s] - (f32)(tx * TILE_SIZE);
                surface += SlopeSurfaceY(ct, localX);
                if (surface > tileTop + (f32)TILE_SIZE) continue;
            } else {
                continue;
            }

            if (b->y + PT_BODY_H < surface - PT_SLOPE_SNAP) continue;
            if (!found || surface < bestSurface) {
                bestSurface = surface;
                found = true;
            }
          }
        }

        if (found) {
            b->y      = bestSurface - PT_BODY_H;
            b->vy     = 0.0f;
            b->onGround = true;
        }
    } else {
        i32 topRow = (i32)(b->y / (f32)TILE_SIZE);
        i32 botRow = (i32)((b->y + PT_BODY_H - 0.5f) / (f32)TILE_SIZE);
        for (i32 ty = botRow; ty >= topRow; ty--) {
            for (int s = 0; s < 3; s++) {
                i32 tx = (i32)(sampleX[s] / (f32)TILE_SIZE);
                if (IsSolid(TileAt(tx, ty))) {
                    b->y  = (f32)((ty + 1) * TILE_SIZE);
                    b->vy = 0.0f;
                    return;
                }
            }
        }
    }
}

// ---- Crate tile collision ----
static void CrateResolveX(CrateBody* c) {
    i32 top = (i32)(c->y / (f32)TILE_SIZE);
    i32 bot = (i32)((c->y + PT_CRATE_SIZE - 0.5f) / (f32)TILE_SIZE);
    if (c->vx > 0.0f) {
        i32 right = (i32)((c->x + PT_CRATE_SIZE - 0.5f) / (f32)TILE_SIZE);
        for (i32 ty = top; ty <= bot; ty++) {
            CollisionType ct = TileAt(right, ty);
            if (IsSlope(ct)) {
                f32 localX = (c->x + PT_CRATE_SIZE) - (f32)(right * TILE_SIZE);
                f32 surfY  = (f32)(ty * TILE_SIZE) + SlopeSurfaceY(ct, localX);
                if (surfY < c->y + PT_CRATE_SIZE - 1.0f) {
                    c->x  = (f32)(right * TILE_SIZE) - PT_CRATE_SIZE;
                    c->vx = 0.0f;
                    return;
                }
                continue;
            }
            if (IsSolid(ct)) {
                c->x  = (f32)(right * TILE_SIZE) - PT_CRATE_SIZE;
                c->vx = 0.0f;
                return;
            }
        }
    } else if (c->vx < 0.0f) {
        i32 left = (i32)(c->x / (f32)TILE_SIZE);
        for (i32 ty = top; ty <= bot; ty++) {
            CollisionType ct = TileAt(left, ty);
            if (IsSlope(ct)) {
                f32 localX = c->x - (f32)(left * TILE_SIZE);
                f32 surfY  = (f32)(ty * TILE_SIZE) + SlopeSurfaceY(ct, localX);
                if (surfY < c->y + PT_CRATE_SIZE - 1.0f) {
                    c->x  = (f32)((left + 1) * TILE_SIZE);
                    c->vx = 0.0f;
                    return;
                }
                continue;
            }
            if (IsSolid(ct)) {
                c->x  = (f32)((left + 1) * TILE_SIZE);
                c->vx = 0.0f;
                return;
            }
        }
    }
}

static void CrateResolveY(CrateBody* c) {
    f32 inset = 1.0f;
    f32 sampleX[3] = {
        c->x + inset,
        c->x + PT_CRATE_SIZE * 0.5f,
        c->x + PT_CRATE_SIZE - inset,
    };

    if (c->vy >= 0.0f) {
        f32 feet   = c->y + PT_CRATE_SIZE;
        i32 botMin = (i32)floorf((feet - (f32)TILE_SIZE) / (f32)TILE_SIZE);
        i32 botMax = (i32)floorf((feet + PT_SLOPE_SNAP)   / (f32)TILE_SIZE);
        f32 bestSurface = 1e9f;
        bool found = false;

        for (i32 s = 0; s < 3; s++) {
            for (i32 bot = botMin; bot <= botMax; bot++) {
                i32 tx = (i32)(sampleX[s] / (f32)TILE_SIZE);
                CollisionType ct = TileAt(tx, bot);
                f32 tileTop = (f32)(bot * TILE_SIZE);
                f32 surface = tileTop;

                if (ct == COL_EMPTY) {
                    if (bot < 0 || bot >= WORLD_HEIGHT_TILES || tx < 0 || tx >= WORLD_WIDTH_TILES) continue;
                    i32 obj = gWorld.tiles[LAYER_OBJECTS][bot][tx];
                    if (obj == OBJ_BUTTON_UP)        surface = tileTop + (f32)TILE_SIZE * 0.50f;
                    else if (obj == OBJ_BUTTON_DOWN) surface = tileTop + (f32)TILE_SIZE * 0.75f;
                    else continue;
                    if (c->prevY + PT_CRATE_SIZE > surface + 2.0f) continue;
                } else if (IsSolid(ct)) {
                    if (c->y + PT_CRATE_SIZE >= surface + (f32)TILE_SIZE) continue;
                } else if (IsSlope(ct)) {
                    // Sample at the crate's leading edge for the slope surface
                    f32 localX = sampleX[s] - (f32)(tx * TILE_SIZE);
                    surface += SlopeSurfaceY(ct, localX);
                    if (surface > tileTop + (f32)TILE_SIZE) continue;
                } else if (ct == COL_HALF_BOTTOM) {
                    surface += (f32)TILE_SIZE * 0.5f;
                } else if (ct == COL_PLATFORM) {
                    continue;
                }

                if (c->y + PT_CRATE_SIZE < surface - PT_SLOPE_SNAP) continue;
                if (!found || surface < bestSurface) {
                    bestSurface = surface;
                    found = true;
                }
            }
        }

        if (found) {
            c->y        = bestSurface - PT_CRATE_SIZE;
            c->vy       = 0.0f;
            c->onGround = true;
        }
    } else {
        i32 topRow = (i32)(c->y / (f32)TILE_SIZE);
        i32 botRow = (i32)((c->y + PT_CRATE_SIZE - 0.5f) / (f32)TILE_SIZE);
        for (i32 ty = botRow; ty >= topRow; ty--) {
            for (i32 s = 0; s < 3; s++) {
                i32 tx = (i32)(sampleX[s] / (f32)TILE_SIZE);
                if (IsSolid(TileAt(tx, ty))) {
                    c->y  = (f32)((ty + 1) * TILE_SIZE);
                    c->vy = 0.0f;
                    return;
                }
            }
        }
    }
}

static void CratePhysStep(CrateBody* c, f32 dt) {
    c->vy += PT_GRAVITY * dt;
    if (c->vy > PT_MAX_FALL) c->vy = PT_MAX_FALL;

    c->x += c->vx * dt;
    CrateResolveX(c);

    c->prevY    = c->y;
    c->onGround = false;
    c->y += c->vy * dt;
    CrateResolveY(c);

    if (c->onGround) {
        c->vx *= PT_CRATE_FRICTION;
        if (fabsf(c->vx) < 20.0f) c->vx = 0.0f;
    }

    // Reset if fallen off screen
    if (c->y > SCR_H + 64) c->active = false;
}

// Crate vs crate horizontal separation (simple push-apart).
static void ResolveCrateVsCrates(i32 idx) {
    CrateBody* a = &s_crates[idx];
    for (i32 j = 0; j < s_crateCount; j++) {
        if (j == idx) continue;
        CrateBody* b = &s_crates[j];
        if (!b->active) continue;
        f32 ox = PT_CRATE_SIZE - fabsf(a->x - b->x);
        f32 oy = PT_CRATE_SIZE - fabsf(a->y - b->y);
        if (ox <= 0 || oy <= 0) continue;
        if (ox < oy) {
            f32 push = ox * 0.5f;
            if (a->x < b->x) { a->x -= push; b->x += push; }
            else             { a->x += push; b->x -= push; }
            f32 avg = (a->vx + b->vx) * 0.5f;
            a->vx = avg; b->vx = avg;
        } else {
            // vertical stack
            if (a->y < b->y) {
                b->y = a->y + PT_CRATE_SIZE;
                b->vy = 0.0f; b->onGround = true;
            } else {
                a->y = b->y + PT_CRATE_SIZE;
                a->vy = 0.0f; a->onGround = true;
            }
        }
    }
}

// Resolve player body against all active crates.
static void ResolvePlayerVsCrates(void) {
    f32 pw = PT_BODY_W, ph = PT_BODY_H;
    f32 cs = PT_CRATE_SIZE;
    s_isPushing = false;
    for (i32 i = 0; i < s_crateCount; i++) {
        CrateBody* c = &s_crates[i];
        if (!c->active) continue;

        f32 pCx = s_body.x + pw * 0.5f, pCy = s_body.y + ph * 0.5f;
        f32 cCx = c->x    + cs * 0.5f,  cCy = c->y    + cs * 0.5f;
        f32 ox  = (pw + cs) * 0.5f - fabsf(pCx - cCx);
        f32 oy  = (ph + cs) * 0.5f - fabsf(pCy - cCy);
        if (ox <= 0 || oy <= 0) continue;

        // Bias toward vertical resolution when falling onto a crate to prevent corner blipping
        bool preferVertical = (s_body.vy > 0.0f && pCy < cCy && oy < ox + 4.0f);
        if (!preferVertical && ox < oy) {
            // Horizontal contact — lock crate to player while pushing
            f32 dir = (pCx < cCx) ? 1.0f : -1.0f;
            c->x  += dir * ox;
            c->vx  = s_body.vx;  // crate moves exactly with player, no independent x drift
            CrateResolveX(c);
            // Push player back by any remaining overlap after crate correction
            f32 remainOx = (pw + cs) * 0.5f - fabsf((s_body.x + pw * 0.5f) - (c->x + cs * 0.5f));
            if (remainOx > 0.0f) {
                s_body.x -= dir * remainOx;
                s_body.vx = 0.0f;
            }
            s_isPushing = true;
        } else {
            // Vertical contact
            if (pCy < cCy) {
                s_body.y        = c->y - ph;
                s_body.vy       = 0.0f;
                s_body.onGround = true;
            } else {
                s_body.y  = c->y + cs;
                s_body.vy = 0.0f;
            }
        }
    }
}

static void PhysStep(PhysBody* b, f32 dt) {
    b->vy += PT_GRAVITY * dt;
    if (b->vy > PT_MAX_FALL) b->vy = PT_MAX_FALL;

    b->x += b->vx * dt;
    ResolveX(b);

    b->prevY    = b->y;
    b->onGround = false;
    b->y       += b->vy * dt;
    ResolveY(b);
    ResolveX(b);  // re-check horizontal after vertical move to catch walls entered from below

    if (b->y > SCR_H + 32) {
        b->x = PT_SPAWN_X; b->y = PT_SPAWN_Y;
        b->vx = 0.0f; b->vy = 0.0f;
    }
}

// ---- Debug collision visualization ----
static void DrawCollisionDebug(void) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            CollisionType ct = TileAt(tx, ty);
            if (ct == COL_EMPTY) continue;

            i32 px = tx * TILE_SIZE;
            i32 py = ty * TILE_SIZE;

            if (IsSolid(ct)) {
                pd->graphics->fillRect(px, py, TILE_SIZE, TILE_SIZE, kColorWhite);
            } else if (ct == COL_PLATFORM) {
                pd->graphics->fillRect(px, py, TILE_SIZE, 3, kColorWhite);
            } else if (ct == COL_HALF_BOTTOM) {
                pd->graphics->fillRect(px, py + TILE_SIZE / 2, TILE_SIZE, TILE_SIZE / 2, kColorWhite);
            } else if (IsSlope(ct)) {
                // Draw slope as a filled triangle using scanlines
                for (i32 sx = 0; sx < TILE_SIZE; sx++) {
                    f32 surfY = SlopeSurfaceY(ct, (f32)sx);
                    i32 fillTop = py + (i32)surfY;
                    i32 fillH   = TILE_SIZE - (i32)surfY;
                    if (fillH > 0) pd->graphics->fillRect(px + sx, fillTop, 1, fillH, kColorWhite);
                }
            }
        }
    }
}

// Lincoln frame indices are 0-based (user counts from 1: idle=1-4, walk=5-10, run=11-14, jump=15-16, fall=17-18)
static void DefineAnimations(void) {
    AnimSequence seq;

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "idle", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback   = ANIM_LOOP;
    seq.frameCount = 4;
    for (int i = 0; i < 4; i++) { seq.frames[i].frameIndex = i; seq.frames[i].duration = 0.18f; }
    AnimSet_AddSequence(&s_animSet, &seq);

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "walk", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback   = ANIM_LOOP;
    seq.frameCount = 6;
    for (int i = 0; i < 6; i++) { seq.frames[i].frameIndex = 4 + i; seq.frames[i].duration = 0.085f; }
    AnimSet_AddSequence(&s_animSet, &seq);

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "run", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback   = ANIM_LOOP;
    seq.frameCount = 4;
    for (int i = 0; i < 4; i++) { seq.frames[i].frameIndex = 10 + i; seq.frames[i].duration = 0.07f; }
    AnimSet_AddSequence(&s_animSet, &seq);

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "jump", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback   = ANIM_LOOP;
    seq.frameCount = 2;
    for (int i = 0; i < 2; i++) { seq.frames[i].frameIndex = 14 + i; seq.frames[i].duration = 0.1f; }
    AnimSet_AddSequence(&s_animSet, &seq);

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "fall", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback   = ANIM_LOOP;
    seq.frameCount = 2;
    for (int i = 0; i < 2; i++) { seq.frames[i].frameIndex = 16 + i; seq.frames[i].duration = 0.1f; }
    AnimSet_AddSequence(&s_animSet, &seq);

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "hurt", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback      = ANIM_ONCE;
    seq.frameCount    = 1;
    seq.frames[0].frameIndex = 18; seq.frames[0].duration = 0.2f;
    AnimSet_AddSequence(&s_animSet, &seq);

    memset(&seq, 0, sizeof(seq));
    strncpy(seq.name, "crouch", MAX_ANIM_NAME_LENGTH - 1);
    seq.playback      = ANIM_ONCE;
    seq.frameCount    = 1;
    seq.frames[0].frameIndex = 19; seq.frames[0].duration = 0.1f;
    AnimSet_AddSequence(&s_animSet, &seq);
}

// ---- Screen callbacks ----

void PhysTestScreen_Init(void) {
    World_Init(pd, &gWorld, "assets/texture/tileset/collision");

    cstr paths[LAYER_COUNT] = {
        NULL,
        NULL,
        "assets/data/csv/test/test_map_Objects.csv",
        NULL,
        "assets/data/csv/test/test_map_Collision.csv"
    };
    if (!World_LoadCSV(pd, &gWorld, paths))
        LOG("PhysTest: failed to load room CSVs");

    s_objTable = Asset_LoadBitmapTable("assets/texture/tileset/BASE_TILES_OBJECTS");
    s_uiBorder = Asset_LoadBitmap("assets/texture/image/UI_Border");
    s_menuRestart = pd->system->addMenuItem("Restart level", PT_RestartCallback, NULL);

    AnimSet_Init(&s_animSet);
    AnimState_Init(&s_animState);
    AnimSet_LoadTable(&s_animSet, "assets/texture/spritesheet/LincolnPlaydate");
    DefineAnimations();
    AnimState_Play(&s_animState, &s_animSet, "idle");
    s_animCur = PS_IDLE;

    // Find OBJ_PLAYER tile in the objects layer to use as spawn position
    f32 spawnX = PT_SPAWN_X, spawnY = PT_SPAWN_Y;
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] == OBJ_PLAYER) {
                spawnX = (f32)(tx * TILE_SIZE);
                spawnY = (f32)(ty * TILE_SIZE);
                goto spawn_found;
            }
        }
    }
    spawn_found:
    s_body = (PhysBody){ .x = spawnX, .y = spawnY, .facingRight = true };
    s_stamina      = 1.0f;
    s_exhaustTimer = 0.0f;
    s_isPushing    = false;
    s_runLocked    = false;
    s_crateMoving  = false;
    s_dustSpawnTimer = 0.0f;
    for (i32 i = 0; i < PT_MAX_DUST; i++) s_dust[i].active = false;
    s_sanityLevel  = 0.0f;
    s_sanityTime   = 0.0f;
    s_smokeTimer   = 0.0f;

    // Scan for OBJ_LIGHT_A to drive DrawRadialLight positions
    s_lightCount = 0;
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES && s_lightCount < PT_MAX_LIGHTS; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES && s_lightCount < PT_MAX_LIGHTS; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] == OBJ_LIGHT_A) {
                s_lightCX[s_lightCount] = tx * TILE_SIZE + TILE_SIZE / 2;
                s_lightCY[s_lightCount] = ty * TILE_SIZE + TILE_SIZE / 2;
                s_lightCount++;
            }
        }
    }

    // Scan object layer for crates; lift them out of the tile map into dynamic bodies
    s_crateCount = 0;
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES && s_crateCount < PT_MAX_CRATES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES && s_crateCount < PT_MAX_CRATES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] != OBJ_CRATE) continue;
            CrateBody* cr  = &s_crates[s_crateCount++];
            cr->x          = (f32)(tx * TILE_SIZE);
            cr->y          = (f32)(ty * TILE_SIZE);
            cr->vx         = 0.0f;
            cr->vy         = 0.0f;
            cr->onGround   = false;
            cr->active     = true;
            gWorld.tiles[LAYER_OBJECTS][ty][tx] = -1;  // remove static tile
        }
    }
}

void PhysTestScreen_Update(f32 deltaTime) {
    if (Signage_IsActive()) { Signage_Update(deltaTime); return; }

    bool left       = Input_IsDown(&inputManager, INPUT_LEFT);
    bool right      = Input_IsDown(&inputManager, INPUT_RIGHT);
    bool leftPress  = Input_IsPressed(&inputManager, INPUT_LEFT);
    bool rightPress = Input_IsPressed(&inputManager, INPUT_RIGHT);
    bool down      = Input_IsDown(&inputManager, INPUT_DOWN);
    bool downPress = Input_IsPressed(&inputManager, INPUT_DOWN);
    bool isCrouching = down;

    // Double-tap DOWN to drop through platforms
    s_body.tapDownFrames++;
    if (downPress) {
        if (s_body.tapDownFrames <= PT_DOUBLE_TAP_FRAMES)
            s_body.dropThrough = true;
        s_body.tapDownFrames = 0;
    }
    if (!down) s_body.dropThrough = false;

    // Double-tap run detection
    s_body.tapFrames++;
    if (leftPress) {
        if (s_body.tapDir == -1 && s_body.tapFrames <= PT_DOUBLE_TAP_FRAMES && s_body.onGround)
            s_body.isRunning = true;
        s_body.tapDir    = -1;
        s_body.tapFrames = 0;
    } else if (rightPress) {
        if (s_body.tapDir == 1 && s_body.tapFrames <= PT_DOUBLE_TAP_FRAMES && s_body.onGround)
            s_body.isRunning = true;
        s_body.tapDir    = 1;
        s_body.tapFrames = 0;
    }
    // cancel run when direction released or in post-depletion lockout
    if (!left && !right) s_body.isRunning = false;
    if (s_stamina >= 0.25f) s_runLocked = false;
    if (s_runLocked) s_body.isRunning = false;

    f32 speed = s_body.isRunning ? PT_RUN_SPEED : PT_MOVE_SPEED;
    if (s_inVent) {
        s_body.vx = 0.0f;
        s_body.vy = 0.0f;
    } else {
        s_body.vx = (!(isCrouching && s_body.onGround) && right) ? speed
                  : (!(isCrouching && s_body.onGround) && left)  ? -speed
                  : 0.0f;
        if (right) s_body.facingRight = true;
        if (left)  s_body.facingRight = false;
    }

    // Clamp to push speed NOW so PhysStep integrates the right velocity
    if (s_isPushing && s_body.onGround) {
        f32 maxPush = (s_exhaustTimer > 0.0f) ? 0.0f : PT_PUSH_SPEED;
        if (s_body.vx >  maxPush) s_body.vx =  maxPush;
        if (s_body.vx < -maxPush) s_body.vx = -maxPush;
    }

    // Stamina drain/regen — pushing crates or running both cost stamina
    bool isExerting = (s_isPushing || s_body.isRunning);
    if (isExerting) {
        s_exhaustTimer -= deltaTime;
        if (s_exhaustTimer < 0.0f) s_exhaustTimer = 0.0f;
        if (s_exhaustTimer == 0.0f && s_stamina > 0.0f) {
            s_stamina -= PT_STAMINA_DRAIN * deltaTime;
            if (s_stamina <= 0.0f) {
                s_stamina      = 0.0f;
                s_exhaustTimer = PT_STAMINA_EXHAUST_T;
                s_runLocked    = true;
            }
        }
    } else {
        s_stamina += PT_STAMINA_REGEN * deltaTime;
        if (s_stamina > 1.0f) s_stamina = 1.0f;
    }

    if (!s_inVent && Input_IsPressed(&inputManager, INPUT_A)) {
        if (s_body.onGround) {
            s_body.vy            = PT_JUMP_VEL;
            s_body.canDoubleJump = true;
            PlaySFX(&audioManager, SFX_JUMP1, 200, false);
        } else if (s_body.canDoubleJump) {
            s_body.vy            = PT_DOUBLE_JUMP_VEL;
            s_body.canDoubleJump = false;
            PlaySFX(&audioManager, SFX_JUMP2, 200, false);
        }
    }
    if (s_body.onGround) s_body.canDoubleJump = true;

    // Step crates first so player resolution can land on them
    for (i32 i = 0; i < s_crateCount; i++) {
        if (s_crates[i].active) CratePhysStep(&s_crates[i], deltaTime);
    }
    for (i32 i = 0; i < s_crateCount; i++) {
        if (s_crates[i].active) ResolveCrateVsCrates(i);
    }

    if (!s_inVent) PhysStep(&s_body, deltaTime);
    ResolvePlayerVsCrates();

    // Crate move sound — loop while any crate has lateral velocity, stop when all still
    bool anyCrateMoving = false;
    for (i32 i = 0; i < s_crateCount; i++) {
        if (s_crates[i].active && fabsf(s_crates[i].vx) > 1.0f)
            anyCrateMoving = true;
    }
    if (anyCrateMoving && !s_crateMoving)
        PlaySFX(&audioManager, SFX_CRATE, 160, true);
    else if (!anyCrateMoving && s_crateMoving)
        StopAllSFX(&audioManager);
    s_crateMoving = anyCrateMoving;

    // Button press — player
    if (s_body.onGround) {
        i32 btx = (i32)((s_body.x + PT_BODY_W * 0.5f) / (f32)TILE_SIZE);
        i32 bty = (i32)((s_body.y + PT_BODY_H - 1.0f)  / (f32)TILE_SIZE);
        if (bty >= 0 && bty < WORLD_HEIGHT_TILES &&
            gWorld.tiles[LAYER_OBJECTS][bty][btx] == OBJ_BUTTON_UP) {
            gWorld.tiles[LAYER_OBJECTS][bty][btx] = OBJ_BUTTON_DOWN;
            s_buttonActivated = true;
            s_sanityLevel = Clamp(s_sanityLevel + 0.25f, 0.0f, 1.0f);
        }
    }

    // Button press — crates
    for (i32 i = 0; i < s_crateCount; i++) {
        if (!s_crates[i].active || !s_crates[i].onGround) continue;
        i32 btx = (i32)((s_crates[i].x + PT_CRATE_SIZE * 0.5f) / (f32)TILE_SIZE);
        i32 bty = (i32)((s_crates[i].y + PT_CRATE_SIZE - 1.0f)  / (f32)TILE_SIZE);
        if (bty >= 0 && bty < WORLD_HEIGHT_TILES &&
            gWorld.tiles[LAYER_OBJECTS][bty][btx] == OBJ_BUTTON_UP) {
            gWorld.tiles[LAYER_OBJECTS][bty][btx] = OBJ_BUTTON_DOWN;
            s_buttonActivated = true;
        }
    }

    // Spring launch
    if (s_body.onGround) {
        i32 ftx = (i32)((s_body.x + PT_BODY_W * 0.5f) / (f32)TILE_SIZE);
        i32 fty = (i32)((s_body.y + PT_BODY_H - 1.0f)  / (f32)TILE_SIZE);
        if (fty < WORLD_HEIGHT_TILES && gWorld.tiles[LAYER_OBJECTS][fty][ftx] == OBJ_SPRING_A) {
            s_body.vy           = PT_SPRING_VEL;
            s_body.onGround     = false;
            s_body.canDoubleJump = true;
            s_springTx = ftx; s_springTy = fty; s_springFrames = PT_SPRING_FRAMES;
            PlaySFX(&audioManager, SFX_SPRING, 220, false);
        }
    }
    if (s_springFrames > 0) s_springFrames--;

    // Landing sound
    if (!s_wasGrounded && s_body.onGround)
        PlaySFX(&audioManager, SFX_LAND, 190, false);
    s_wasGrounded = s_body.onGround;

    // Footstep sound on the second frame of the walk/run cycle
    if (s_body.onGround && !s_inVent && s_body.vx != 0.0f) {
        u8 fp = s_animState.framePos;
        if (fp == PT_STEP_FRAME && s_prevFramePos != PT_STEP_FRAME)
            PlaySFX(&audioManager, SFX_STEP, 160, false);
        s_prevFramePos = fp;
    } else {
        s_prevFramePos = 0xFF;
    }

    PlayerAnimState next = PS_IDLE;
    if (!s_body.onGround)             next = (s_body.vy < 0.0f) ? PS_JUMP : PS_FALL;
    else if (isCrouching)             next = PS_CROUCH;
    else if (s_body.vx != 0.0f)       next = s_body.isRunning ? PS_RUN : PS_WALK;

    if (next != s_animCur) {
        s_animCur = next;
        switch (next) {
            case PS_IDLE:   AnimState_Play(&s_animState, &s_animSet, "idle");   break;
            case PS_WALK:   AnimState_Play(&s_animState, &s_animSet, "walk");   break;
            case PS_RUN:    AnimState_Play(&s_animState, &s_animSet, "run");    break;
            case PS_CROUCH: AnimState_Play(&s_animState, &s_animSet, "crouch"); break;
            case PS_JUMP:   AnimState_Play(&s_animState, &s_animSet, "jump");   break;
            case PS_FALL:   AnimState_Play(&s_animState, &s_animSet, "fall");   break;
        }
    }
    AnimState_Update(&s_animState, &s_animSet, deltaTime);

    s_sanityTime  += deltaTime;
    s_smokeTimer  += deltaTime;

    // Spawn dust from base of moving crates
    s_dustSpawnTimer += deltaTime;
    if (s_dustSpawnTimer >= 0.06f) {
        s_dustSpawnTimer = 0.0f;
        for (i32 i = 0; i < s_crateCount; i++) {
            if (!s_crates[i].active || !s_crates[i].onGround) continue;
            if (fabsf(s_crates[i].vx) < 1.0f) continue;
            f32 edgeX = s_crates[i].vx > 0 ? s_crates[i].x : s_crates[i].x + PT_CRATE_SIZE;
            SpawnDust(edgeX, s_crates[i].y + PT_CRATE_SIZE - 2.0f);
        }
    }
    // Update dust particles
    for (i32 i = 0; i < PT_MAX_DUST; i++) {
        if (!s_dust[i].active) continue;
        s_dust[i].x    += s_dust[i].vx * deltaTime;
        s_dust[i].y    += s_dust[i].vy * deltaTime;
        s_dust[i].vy   += 40.0f * deltaTime;  // gentle fall
        s_dust[i].life -= deltaTime * 2.2f;
        if (s_dust[i].life <= 0.0f) s_dust[i].active = false;
    }

    // Sign interaction — open on UP press near a sign
    i32 stx, sty;
    if (NearSign(&stx, &sty) && Input_IsPressed(&inputManager, INPUT_UP))
        Signage_Show("Notice", "This is a test sign. Press { or } to dismiss.");

    // Vent interaction
    i32 vtx, vty;
    if (Input_IsPressed(&inputManager, INPUT_UP)) {
        if (s_inVent) {
            // Exit vent — pop player out just below the vent tile
            s_body.x    = (f32)(s_ventTx * TILE_SIZE) + (TILE_SIZE - PT_BODY_W) * 0.5f;
            s_body.y    = (f32)((s_ventTy + 1) * TILE_SIZE);
            s_body.vx   = 0.0f;
            s_body.vy   = 0.0f;
            s_inVent    = false;
            s_ventTx    = -1;
            s_ventTy    = -1;
        } else if (!s_inVent && NearVent(&vtx, &vty)) {
            // Enter vent — snap player into vent tile and freeze
            s_body.x    = (f32)(vtx * TILE_SIZE) + (TILE_SIZE - PT_BODY_W) * 0.5f;
            s_body.y    = (f32)(vty * TILE_SIZE) + (TILE_SIZE - PT_BODY_H) * 0.5f;
            s_body.vx   = 0.0f;
            s_body.vy   = 0.0f;
            s_inVent    = true;
            s_ventTx    = vtx;
            s_ventTy    = vty;
        }
    }

    if (Input_IsPressed(&inputManager, INPUT_B))
        SwitchScreen(&screenManager, SS_TEST_WORLD);
    if (Input_IsPressed(&inputManager, INPUT_UP) && !s_inVent)
        SwitchScreen(&screenManager, SS_TEST_6);
}

void PhysTestScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);

    // 1. Debug collision geometry (white shapes on black)
    DrawCollisionDebug();

    // 3. Objects from CSV — skip OBJ_PLAYER (spawn marker), use BlackTransparent so background shows through
    if (s_objTable) {
        pd->graphics->setDrawMode(kDrawModeBlackTransparent);
        for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
            for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
                i32 id = gWorld.tiles[LAYER_OBJECTS][ty][tx];
                if (id <= 0) continue;  // skip empty (-1) and OBJ_PLAYER (0)
                // Show compressed spring while active
                if (id == OBJ_SPRING_A && s_springFrames > 0 && tx == s_springTx && ty == s_springTy)
                    id = OBJ_SPRING_B;
                LCDBitmap* bmp = pd->graphics->getTableBitmap(s_objTable, id);
                if (bmp) pd->graphics->drawBitmap(bmp, tx * TILE_SIZE, ty * TILE_SIZE, kBitmapUnflipped);
            }
        }
        pd->graphics->setDrawMode(kDrawModeCopy);
    }

    // 3b. Dynamic crates (drawn after static objects, before player)
    if (s_objTable) {
        pd->graphics->setDrawMode(kDrawModeBlackTransparent);
        for (i32 i = 0; i < s_crateCount; i++) {
            if (!s_crates[i].active) continue;
            LCDBitmap* bmp = pd->graphics->getTableBitmap(s_objTable, OBJ_CRATE);
            if (bmp) pd->graphics->drawBitmap(bmp,
                (i32)s_crates[i].x, (i32)s_crates[i].y, kBitmapUnflipped);
        }
        pd->graphics->setDrawMode(kDrawModeCopy);
    }

    // 4. Player sprite — hidden while inside a vent or smoke
    bool s_inSmoke = false;
    {
        f32 px = s_body.x + PT_BODY_W * 0.5f, py = s_body.y;
        for (i32 i = 0; i < SMOKE_PLATFORM_COUNT; i++) {
            f32 left = (f32)(s_smokePlatforms[i].cx - s_smokePlatforms[i].w / 2 - 4);
            f32 right= (f32)(s_smokePlatforms[i].cx + s_smokePlatforms[i].w / 2 + 4);
            f32 top  = (f32)(s_smokePlatforms[i].topY - 22);
            f32 bot  = (f32)(s_smokePlatforms[i].topY + 4);
            if (px > left && px < right && py + PT_BODY_H > top && py < bot)
                s_inSmoke = true;
        }
    }
    // 5. Radial lights — drawn before player so player always renders on top
    pd->graphics->setDrawMode(kDrawModeBlackTransparent);
    for (i32 i = 0; i < s_lightCount; i++) {
        PT_DrawRadialLight(s_lightCX[i], s_lightCY[i], 38, s_smokeTimer);
        if (s_objTable) {
            pd->graphics->setDrawMode(kDrawModeInverted);
            LCDBitmap* lbmp = pd->graphics->getTableBitmap(s_objTable, OBJ_LIGHT_A);
            if (lbmp) pd->graphics->drawBitmap(lbmp,
                s_lightCX[i] - TILE_SIZE/2, s_lightCY[i] - TILE_SIZE/2, kBitmapUnflipped);
            pd->graphics->setDrawMode(kDrawModeBlackTransparent);
        }
    }
    pd->graphics->setDrawMode(kDrawModeCopy);

    // 6. Smoke clouds — drawn before player so player always renders on top
    pd->graphics->setDrawMode(kDrawModeBlackTransparent);
    for (i32 i = 0; i < SMOKE_PLATFORM_COUNT; i++) {
        i32 cx   = s_smokePlatforms[i].cx;
        i32 topY = s_smokePlatforms[i].topY;
        i32 pw   = s_smokePlatforms[i].w;
        for (i32 p = 0; p < 3; p++) {
            f32 bob  = sinf(s_smokeTimer * 1.4f + (f32)i * 1.3f + (f32)p * 0.9f) * 2.0f;
            i32 puffW = pw / 2 + 4;
            i32 puffH = 14;
            i32 puffX = cx - pw/2 + (p * pw/3) - puffW/4;
            i32 puffY = topY - puffH + (i32)bob;
            pd->graphics->fillEllipse(puffX, puffY, puffW, puffH,
                                      0.0f, 0.0f, (LCDColor)(uintptr_t)kLPat50);
        }
    }
    pd->graphics->setDrawMode(kDrawModeCopy);

    // 6b. Dust particles from moving crates
    pd->graphics->setDrawMode(kDrawModeBlackTransparent);
    for (i32 i = 0; i < PT_MAX_DUST; i++) {
        if (!s_dust[i].active) continue;
        f32 t = s_dust[i].life;  // 1→0
        i32 sz = 1 + (i32)(t * 3.0f);
        LCDColor col = t > 0.6f ? (LCDColor)(uintptr_t)kLPat75
                     : t > 0.3f ? (LCDColor)(uintptr_t)kLPat50
                                : (LCDColor)(uintptr_t)kLPat25;
        pd->graphics->fillEllipse((i32)s_dust[i].x - sz/2, (i32)s_dust[i].y - sz/2,
                                  sz, sz, 0.0f, 0.0f, col);
    }
    pd->graphics->setDrawMode(kDrawModeCopy);

    if (!s_inVent) {
    LCDBitmap* bmp = AnimState_GetCurrentBitmap(&s_animState, &s_animSet);
    if (bmp) {
        Vec2 off  = AnimState_GetCurrentOffset(&s_animState, &s_animSet);
        LCDBitmapFlip flip = s_body.facingRight ? kBitmapUnflipped : kBitmapFlippedX;
        pd->graphics->setDrawMode(kDrawModeBlackTransparent);
        pd->graphics->drawBitmap(bmp,
            (i32)(s_body.x - 3.0f) + (i32)off.x,
            (i32)(s_body.y - 2.0f) + (i32)off.y,
            flip);
        pd->graphics->setDrawMode(kDrawModeCopy);
    }
    }

    bool s_isHidden = s_inVent || s_inSmoke;

    // UP prompts over nearby interactive objects
    i32 stx, sty;
    if (!Signage_IsActive() && NearSign(&stx, &sty)) {
        i32 px = stx * TILE_SIZE + (TILE_SIZE - 8) / 2;
        i32 py = sty * TILE_SIZE - 10;
        DrawIconText("^", px, py, 2, kColorWhite, 10);
    }
    i32 vtx, vty;
    if (!s_inVent && NearVent(&vtx, &vty)) {
        i32 px = vtx * TILE_SIZE + (TILE_SIZE - 8) / 2;
        i32 py = vty * TILE_SIZE - 10;
        DrawIconText("^", px, py, 2, kColorWhite, 10);
    }
    if (s_inVent) {
        i32 px = s_ventTx * TILE_SIZE + (TILE_SIZE - 8) / 2;
        i32 py = s_ventTy * TILE_SIZE - 10;
        DrawIconText("^", px, py, 2, kColorWhite, 10);
    }

    // Sign box drawn last (on top of everything)
    Signage_Draw();

    // UI border drawn over everything
    if (s_uiBorder) {
        pd->graphics->setDrawMode(kDrawModeCopy);
        pd->graphics->drawBitmap(s_uiBorder, 0, 0, kBitmapUnflipped);
    }

    // ---- HUD drawn on top of border ----

    // Visibility badge — top-right corner of game viewport
    {
        cstr label = s_isHidden ? "HIDDEN" : "VISIBLE";
        i32 lw = GetTextWidth(label, 2);
        i32 bx = 382 - lw - 4, by = 4;
        pd->graphics->fillRect(bx - 3, by - 2, lw + 6, 12, s_isHidden ? kColorBlack : kColorWhite);
        pd->graphics->drawRect(bx - 3, by - 2, lw + 6, 12, s_isHidden ? kColorWhite : kColorBlack);
        DrawText(label, bx, by, 2, s_isHidden ? kColorWhite : kColorBlack);
    }

    if (s_buttonActivated)
        pd->graphics->fillRect(390, 4, 8, 8, kColorWhite);

    // Stat bars in the bottom panel (white on black)
    pd->graphics->setDrawMode(kDrawModeCopy);
    {
        static const f32 s_health = 1.0f;  // placeholder until real health exists
        static const cstr labels[3] = { "HP", "SP", "SN" };
        const f32 vals[3] = { s_health, s_stamina, 1.0f - s_sanityLevel };
        i32 barW = 52, barH = 3;
        for (i32 i = 0; i < 3; i++) {
            i32 rowY = 217 + i * 8;
            DrawText(labels[i], 4, rowY, 2, kColorWhite);
            i32 bx = 22;
            f32 blinkRate = (i == 1) ? 24.0f : 6.0f;
            bool blink = vals[i] < 0.25f && (i32)(s_smokeTimer * blinkRate) % 2 == 1;
            i32 fill = blink ? 0 : (i32)(vals[i] * (f32)barW);
            if (fill > 0) pd->graphics->fillRect(bx, rowY, fill, barH, kColorWhite);
            pd->graphics->drawRect(bx - 1, rowY - 1, barW + 2, barH + 2, kColorWhite);
        }
    }

    // 3 item slots in the bottom panel, right-aligned
    for (i32 i = 0; i < 3; i++) {
        i32 slotX = 310 + i * 20, slotY = 220;
        pd->graphics->drawRect(slotX, slotY, 14, 14, kColorWhite);
    }
}

void PhysTestScreen_Unload(void) {
    if (s_menuRestart) {
        pd->system->removeMenuItem(s_menuRestart);
        s_menuRestart = NULL;
    }
    World_Free(pd, &gWorld);
    AnimSet_Free(&s_animSet);
    if (s_objTable) {
        Asset_FreeBitmapTable(s_objTable);
        s_objTable = NULL;
    }
    if (s_uiBorder) {
        Asset_FreeBitmap(s_uiBorder);
        s_uiBorder = NULL;
    }
}
