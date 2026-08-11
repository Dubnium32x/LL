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
#define SFX_BUTTON  "assets/audio/sfx/button_press.wav"
#define SFX_HURT    "assets/audio/sfx/hurt.wav"
#define SFX_VENT_OPEN  "assets/audio/sfx/vent_open.wav"
#define SFX_VENT_CLOSE "assets/audio/sfx/vent_close.wav"
#define SFX_DOOR_OPEN  "assets/audio/sfx/door_open.wav"
#define SFX_DOOR_CLOSE "assets/audio/sfx/door_close.wav"
#define SFX_PLATFORM_RUMBLE "assets/audio/sfx/rumble.wav"
#define SFX_KEY_PICKUP      "assets/audio/sfx/key_pickup.wav"
#define SFX_DOOR_LOCKED     "assets/audio/sfx/locked.wav"
#define SFX_METER_LOW  "assets/audio/sfx/meter_low.wav"
#define PT_METER_LOW_THRESHOLD 0.2f

// Paused while entrance/exit doors are being built out — flip back to 1 to resume. Gates the
// low-sanity glitch roll, the blank-sign roll, and (via PhysTestScreen_IsSanityLow) the
// "Are you okay?" pause-menu roll.
#define PT_SANITY_EFFECTS_ENABLED 0
// The two semi-solid platforms that hosted these are now the entrance/exit door spots.
#define PT_SMOKE_CLOUDS_ENABLED 0
#define AMB_DRONE   "assets/audio/amb/basic_droning.wav"
#define SFX_STATIC  "assets/audio/sfx/static.wav"
#define DEATH_FMV_PATH "assets/fmv/10 sec 2D Test animation"
#define PT_DEATH_STATIC_DURATION 0.5f  // seconds the static filter holds before the death FMV starts

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
#define PT_HURT_DURATION    0.35f // seconds of reeling/knockback after touching a spike
#define PT_HURT_KNOCK_VX    90.0f
#define PT_HURT_KNOCK_VY  -140.0f
#define PT_HURT_HP_LOSS     0.15f
#define PT_HURT_SHAKE_INTENSITY 10.0f
#define PT_HURT_SHAKE_DURATION  0.3f

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
typedef enum { PS_IDLE, PS_WALK, PS_RUN, PS_CROUCH, PS_JUMP, PS_FALL, PS_HURT } PlayerAnimState;

// ---- Death sequence: static filter, then FMV, then restart the level ----
typedef enum { PT_LIFE_ALIVE, PT_LIFE_STATIC, PT_LIFE_FMV } PTLifecycle;
static PTLifecycle s_lifecycle = PT_LIFE_ALIVE;
static FMVPlayer    s_deathFMV;
// Tracked independently of visualManager.staticTimer: the death pause must hold for its full
// duration even when screenFxEnabled is off and the static noise itself doesn't render.
static f32 s_deathStaticTimer = 0.0f;

// ---- Door transition: pause everything (same idea as the death freeze), fade to black, swap
// the room, fade back in from black (still paused), then resume with a door-close sound. ----
typedef enum { PT_DOOR_NONE, PT_DOOR_FADE_OUT, PT_DOOR_FADE_IN } PTDoorTransition;
static PTDoorTransition s_doorTransition = PT_DOOR_NONE;
#define PT_DOOR_FADE_DURATION 0.5f

static AnimationSet    s_animSet;
static AnimationState  s_animState;
static PlayerAnimState s_animCur;

// ---- Crate physics ----
#define PT_MAX_CRATES    8
#define PT_CRATE_SIZE   ((f32)TILE_SIZE)
#define PT_CRATE_FRICTION 0.97f  // lateral damping when grounded
#define PT_CRATE_PUSH_VEL   22.0f  // velocity transferred to crate on player contact
#define PT_PUSH_SPEED        14.0f   // player max speed while pushing a crate
#define PT_STAMINA_DRAIN      0.3f  // per second while pushing
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
static i32       s_riddenCrate  = -1;    // index of crate player is currently standing on, or -1
static f32       s_hurtTimer    = 0.0f;  // >0 while reeling from a spike hit; locks out input
static f32       s_health       = 1.0f;  // 0..1
static bool      s_meterLowPlaying = false; // meter_low loop active (health or sanity < 20%)

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

// ---- Push-impact lines ----
#define PT_MAX_PUSH_LINES 12
typedef struct {
    f32 x, y, vx;   // origin + outward velocity
    f32 len;         // current length (shrinks with life)
    f32 life;        // 0..1, counts down
    bool active;
} PushLine;
static PushLine s_pushLines[PT_MAX_PUSH_LINES];

static void SpawnPushEffect(f32 contactX, f32 contactY, f32 dir) {
    i32 spawned = 0;
    for (i32 i = 0; i < PT_MAX_PUSH_LINES && spawned < 4; i++) {
        if (s_pushLines[i].active) continue;
        f32 spread = ((f32)(spawned) - 1.5f) * 5.0f;
        s_pushLines[i].x    = contactX;
        s_pushLines[i].y    = contactY + spread;
        s_pushLines[i].vx   = dir * (28.0f + spawned * 8.0f);
        s_pushLines[i].len  = 6.0f + spawned * 2.0f;
        s_pushLines[i].life = 1.0f;
        s_pushLines[i].active = true;
        spawned++;
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
static bool s_lightsOff = false;      // true while the dim glitch (GLITCH_DIM) is running (Idea #3: "all lights in a level are off")
static bool s_sanityWasLow = false;   // edge-detects entering the low-sanity zone

// Adaptive "pity" chance for a sanity glitch firing on a low-sanity dip: starts (and caps) at
// 40%. Every dip that doesn't fire raises the odds by 10%; a dip that does fire drops the odds
// back down to 10% so glitches don't chain back-to-back. Deliberately NOT reset on level
// restart — the cooldown has to survive the very restart a glitch itself causes.
#define PT_SANITY_GLITCH_CHANCE_START 40
#define PT_SANITY_GLITCH_CHANCE_MIN   10
#define PT_SANITY_GLITCH_CHANCE_STEP  10
static i32 s_glitchChancePct = PT_SANITY_GLITCH_CHANCE_START;

// Which glitch fired last (0=DIM, 1=CRASH, 2=OFF, 3=BLOOD, 4=INVERT_CONTROLS, 5=INVERT_VISUALS,
// 6=MISSING_OBJECTS; -1 = none yet). Also NOT reset on level restart, so the effect that just
// finished (and caused this restart) can't immediately repeat.
static i32 s_lastGlitchPick = -1;

// Sanity glitch sequence: a low-sanity dip has a chance to fire one random glitch — the
// dim-lights effect, a fake Playdate crash screen, a fake "turned off" blackout, blood
// dripping down the screen, inverted controls, inverted visuals, or vanished world objects —
// and all of them end by routing through a TV-snow burst before restarting the level.
typedef enum {
    GLITCH_NONE,
    GLITCH_DIM,              // lights dimmed for a random few seconds
    GLITCH_SNOW,              // TV-snow burst that ends any other glitch phase, then restarts
    GLITCH_CRASH_BOOT,       // fake crash: boot_logo.png held
    GLITCH_CRASH_ANIM,       // fake crash: dump_out -> dump_out2 -> dump_out3 frames
    GLITCH_CRASH_HOLD,       // fake crash: holding on dump_out4 ("press A to restart"), auto-advances
    GLITCH_OFF,              // fake "turned off" — plain black screen, then hands off to GLITCH_SNOW
    GLITCH_BLOOD,            // blood drips down from the top of the screen for a random few seconds
    GLITCH_INVERT_CONTROLS,  // dpad and A/B all swapped (left/right, up/down, A/B) for a random few seconds
    GLITCH_INVERT_VISUALS,   // whole screen color-inverted for a random few seconds
    GLITCH_MISSING_OBJECTS,  // objects layer stops drawing (collision unaffected) for a random few seconds
} SanityGlitchPhase;
static SanityGlitchPhase s_glitchPhase = GLITCH_NONE;
static f32 s_glitchTimer = 0.0f;
static const f32 kGlitchDimDurations[4] = { 3.0f, 4.0f, 6.0f, 8.0f };
static const f32 kGlitchBloodDurations[4] = { 4.0f, 5.0f, 7.0f, 9.0f };
static const f32 kGlitchInvertControlsDurations[4] = { 4.0f, 5.0f, 7.0f, 9.0f };
static const f32 kGlitchInvertVisualsDurations[4]  = { 3.0f, 4.0f, 6.0f, 8.0f };
static const f32 kGlitchMissingObjDurations[4]     = { 4.0f, 5.0f, 7.0f, 9.0f };

#define PT_CRASH_BOOT_DURATION   1.2f
#define PT_CRASH_ANIM_FRAME_TIME 0.18f
#define PT_CRASH_HOLD_DURATION   1.3f
#define PT_TURNOFF_DURATION      5.0f  // plain black "off" screen, held before the TV-snow burst
#define PT_BLOOD_SPAWN_INTERVAL 0.5f   // trickle of new drips while GLITCH_BLOOD is active
#define PT_BLOOD_SPAWN_COUNT    3      // drips spawned per trickle
static f32 s_bloodSpawnTimer = 0.0f;

// Glitched-sign "soft" glitch — not part of the SanityGlitchPhase state machine at all. Rolled
// fresh every time a sign is read, independent of the DIM/CRASH/OFF/BLOOD/etc. roll+cooldown.
#define PT_BLANK_SIGN_CHANCE_PCT    33   // out of 100, only rolled while sanityLow is true
#define PT_BLANK_SIGN_HOLD_DURATION 1.5f // seconds the glitched box sits before auto-closing
#define PT_BLANK_SIGN_PHRASE_COUNT  7
static const char* const kBlankSignPhrases[PT_BLANK_SIGN_PHRASE_COUNT] = {
    "Did I have a choice in any of this?",
    "This is all my fault.",
    "Where the hell am I?",
    "Maybe I deserve this.",
    "Why do I even try?",
    "I'm not going to make it, am I?",
    "I miss my wife...",
    "I want to go home...",
    "Is this all a dream?",
    "I can't even remember what's real anymore...",
};
#define IMG_CRASH_BOOT "assets/texture/image/sanity_effects/fake_crash/boot_logo"
#define IMG_CRASH_1    "assets/texture/image/sanity_effects/fake_crash/dump_out"
#define IMG_CRASH_2    "assets/texture/image/sanity_effects/fake_crash/dump_out2"
#define IMG_CRASH_3    "assets/texture/image/sanity_effects/fake_crash/dump_out3"
#define IMG_CRASH_4    "assets/texture/image/sanity_effects/fake_crash/dump_out4"
static LCDBitmap* s_crashBootImg = NULL;
static LCDBitmap* s_crashFrames[4] = { NULL, NULL, NULL, NULL };

// While GLITCH_INVERT_CONTROLS is active, every dpad direction and both face buttons read as
// their opposite. Every player-input read in this file goes through PT_IsDown/PT_IsPressed
// (instead of calling Input_IsDown/Input_IsPressed directly) so none of them are missed.
static InputBit PT_MapInput(InputBit bit) {
    if (s_glitchPhase != GLITCH_INVERT_CONTROLS) return bit;
    switch (bit) {
        case INPUT_LEFT:  return INPUT_RIGHT;
        case INPUT_RIGHT: return INPUT_LEFT;
        case INPUT_UP:    return INPUT_DOWN;
        case INPUT_DOWN:  return INPUT_UP;
        case INPUT_A:     return INPUT_B;
        case INPUT_B:     return INPUT_A;
        default:          return bit;
    }
}
static bool PT_IsDown(InputBit bit)    { return Input_IsDown(&inputManager, PT_MapInput(bit)); }
static bool PT_IsPressed(InputBit bit) { return Input_IsPressed(&inputManager, PT_MapInput(bit)); }
static i32 s_crashFrameIndex = 0;

// Smoke cloud platform anchors (hand-placed at semi-solid platform tops)
typedef struct { i32 cx, topY, w; } SmokePlatform;
static const SmokePlatform s_smokePlatforms[] = {
    { 104, 80, 48 },   // row-5 semi-solid platform (tx 5-7)
    { 232, 64, 48 },   // row-4 semi-solid platform (tx 13-15)
};
#define SMOKE_PLATFORM_COUNT 2

// ---- Pressure-plate-activated platforms (OBJ_PLATFORM_ACTIVE / OBJ_PLATFORM_INACTIVE) ----
// Tiles are pre-placed in the objects CSV as OBJ_PLATFORM_ACTIVE purely to mark which tiles this
// system should track; PhysTestScreen_Init immediately forces them to their real starting state
// (inactive, no collision). They only become solid + visible while a pressure plate is actively
// weighted (see UpdatePressurePlates/UpdateTogglePlatforms) — step off (or move the crate off)
// and they drop back out from under you.
typedef struct {
    i32 tx, ty;
    bool active;
} TogglePlatform;
#define PT_MAX_TOGGLE_PLATFORMS 8
static TogglePlatform s_togglePlatforms[PT_MAX_TOGGLE_PLATFORMS];
static i32 s_togglePlatformCount = 0;

// ---- Item pickup (local test-only inventory, not wired to the real PlayerData system) ----
// Stores the OBJ_ id of whatever was picked up, doubling as both the HUD icon to draw and the
// "this slot is filled" flag (-1 = empty).
#define PT_ITEM_SLOT_COUNT 3
static i32 s_pickedItemObj[PT_ITEM_SLOT_COUNT];

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

// Returns true if the player is adjacent to a lever tile (either state).
static bool NearLever(i32* outTx, i32* outTy) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            i32 obj = gWorld.tiles[LAYER_OBJECTS][ty][tx];
            if (obj != OBJ_LEVER_OFF && obj != OBJ_LEVER_ON) continue;
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

// Checks every pressure plate tile each frame: presses (with sound) when the player or a
// crate comes to rest on top, releases silently once the weight is gone.
static void UpdatePressurePlates(void) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            i32 obj = gWorld.tiles[LAYER_OBJECTS][ty][tx];
            if (obj != OBJ_PRESSURE_PLATE_UP && obj != OBJ_PRESSURE_PLATE_DOWN) continue;

            bool weighted = false;
            if (s_body.onGround) {
                i32 btx = (i32)((s_body.x + PT_BODY_W * 0.5f) / (f32)TILE_SIZE);
                i32 bty = (i32)((s_body.y + PT_BODY_H - 1.0f)  / (f32)TILE_SIZE);
                if (btx == tx && bty == ty) weighted = true;
            }
            if (!weighted) {
                for (i32 i = 0; i < s_crateCount; i++) {
                    CrateBody* c = &s_crates[i];
                    if (!c->active || !c->onGround) continue;
                    i32 ctx = (i32)((c->x + PT_CRATE_SIZE * 0.5f) / (f32)TILE_SIZE);
                    i32 cty = (i32)((c->y + PT_CRATE_SIZE - 1.0f)  / (f32)TILE_SIZE);
                    if (ctx == tx && cty == ty) { weighted = true; break; }
                }
            }

            if (weighted && obj == OBJ_PRESSURE_PLATE_UP) {
                gWorld.tiles[LAYER_OBJECTS][ty][tx] = OBJ_PRESSURE_PLATE_DOWN;
                PlaySFX(&audioManager, SFX_BUTTON, 180, false);
            } else if (!weighted && obj == OBJ_PRESSURE_PLATE_DOWN) {
                gWorld.tiles[LAYER_OBJECTS][ty][tx] = OBJ_PRESSURE_PLATE_UP;
            }
        }
    }
}

// Platforms mirror whatever pressure plate is currently held down (player or crate weight —
// UpdatePressurePlates already resolves that into the plate's own UP/DOWN tile state, so this
// just reads that back rather than duplicating the weight check). Must run after
// UpdatePressurePlates each frame so it sees this frame's plate state, not last frame's.
static void UpdateTogglePlatforms(void) {
    bool anyPlateDown = false;
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES && !anyPlateDown; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] == OBJ_PRESSURE_PLATE_DOWN) {
                anyPlateDown = true;
                break;
            }
        }
    }

    for (i32 i = 0; i < s_togglePlatformCount; i++) {
        TogglePlatform* p = &s_togglePlatforms[i];
        if (p->active == anyPlateDown) continue;  // no state change

        p->active = anyPlateDown;
        gWorld.tiles[LAYER_OBJECTS][p->ty][p->tx]   = anyPlateDown ? OBJ_PLATFORM_ACTIVE : OBJ_PLATFORM_INACTIVE;
        gWorld.tiles[LAYER_COLLISION][p->ty][p->tx] = anyPlateDown ? COL_PLATFORM       : COL_EMPTY;
        if (anyPlateDown) PlaySFX(&audioManager, SFX_PLATFORM_RUMBLE, 180, false);
    }
}

// Automatic pickup on overlap — walk over a collectible tile (currently just OBJ_KEY) and it's
// stored into the first empty HUD item slot.
// Swaps every OBJ_DOOR_LOCKED tile to OBJ_DOOR_EXIT — called the moment the key is picked up, so
// the door visibly unlocks immediately rather than waiting until the player walks up to it.
static void UnlockExitDoors(void) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] == OBJ_DOOR_LOCKED)
                gWorld.tiles[LAYER_OBJECTS][ty][tx] = OBJ_DOOR_EXIT;
        }
    }
}

static void TryPickupItems(void) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            i32 obj = gWorld.tiles[LAYER_OBJECTS][ty][tx];
            if (obj != OBJ_KEY) continue;

            f32 ix = (f32)(tx * TILE_SIZE), iy = (f32)(ty * TILE_SIZE);
            bool overlap = s_body.x < ix + (f32)TILE_SIZE && s_body.x + PT_BODY_W > ix &&
                           s_body.y < iy + (f32)TILE_SIZE && s_body.y + PT_BODY_H > iy;
            if (!overlap) continue;

            i32 slot = -1;
            for (i32 i = 0; i < PT_ITEM_SLOT_COUNT; i++) {
                if (s_pickedItemObj[i] == -1) { slot = i; break; }
            }
            if (slot == -1) continue;  // inventory full

            s_pickedItemObj[slot] = obj;
            gWorld.tiles[LAYER_OBJECTS][ty][tx] = -1;
            PlaySFX(&audioManager, SFX_KEY_PICKUP, 200, false);
            Notify_Show("Key obtained.", 2.5f);
            UnlockExitDoors();  // only OBJ_KEY reaches here (see the continue above)
        }
    }
}

// Returns true if the player is adjacent to the exit door tile, locked or unlocked.
static bool NearExitDoor(i32* outTx, i32* outTy) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            i32 obj = gWorld.tiles[LAYER_OBJECTS][ty][tx];
            if (obj != OBJ_DOOR_EXIT && obj != OBJ_DOOR_LOCKED) continue;
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

static bool IsSpikeObj(i32 obj) {
    return obj == OBJ_SPIKE_UP || obj == OBJ_SPIKE_DOWN
        || obj == OBJ_SPIKE_RIGHT || obj == OBJ_SPIKE_LEFT;
}

// Returns true if the player's AABB overlaps a spike tile in the object layer.
static bool PlayerTouchingSpike(void) {
    i32 l = (i32)(s_body.x / (f32)TILE_SIZE);
    i32 r = (i32)((s_body.x + PT_BODY_W - 0.5f) / (f32)TILE_SIZE);
    i32 t = (i32)(s_body.y / (f32)TILE_SIZE);
    i32 b = (i32)((s_body.y + PT_BODY_H - 0.5f) / (f32)TILE_SIZE);
    for (i32 ty = t; ty <= b; ty++) {
        if (ty < 0 || ty >= WORLD_HEIGHT_TILES) continue;
        for (i32 tx = l; tx <= r; tx++) {
            if (tx < 0 || tx >= WORLD_WIDTH_TILES) continue;
            if (IsSpikeObj(gWorld.tiles[LAYER_OBJECTS][ty][tx])) return true;
        }
    }
    return false;
}

// Player hit 0 HP — freeze gameplay, flash a static filter with static.wav, then play the
// death FMV. PhysTestScreen_Update/Draw check s_lifecycle first and short-circuit while this
// plays out; the FMV finishing restarts the level.
static void TriggerPlayerDeath(void) {
    if (s_lifecycle != PT_LIFE_ALIVE) return;
    s_lifecycle      = PT_LIFE_STATIC;
    s_deathStaticTimer = PT_DEATH_STATIC_DURATION;
    s_body.vx        = 0.0f;
    s_body.vy        = 0.0f;
    StopSFX(&audioManager, SFX_CRATE);
    StopSFX(&audioManager, SFX_METER_LOW);
    StopAllAmb(&audioManager);
    Visual_TriggerTVSnow(&visualManager, PT_DEATH_STATIC_DURATION);
    PlaySFX(&audioManager, SFX_STATIC, 200, false);
}

// Spike hit: deplete HP, play the hurt sound + screenshake, and knock the player away while
// the hurt animation plays and input is locked out.
static void TriggerPlayerHurt(f32 knockDir) {
    s_hurtTimer      = PT_HURT_DURATION;
    s_health         = Clamp(s_health - PT_HURT_HP_LOSS, 0.0f, 1.0f);
    PlaySFX(&audioManager, SFX_HURT, 200, false);
    Visual_StartShake(&visualManager, PT_HURT_SHAKE_INTENSITY, PT_HURT_SHAKE_DURATION);
    s_body.vx        = knockDir * PT_HURT_KNOCK_VX;
    s_body.vy        = PT_HURT_KNOCK_VY;
    s_body.onGround  = false;
    if (s_animCur != PS_HURT) {
        s_animCur = PS_HURT;
        AnimState_Play(&s_animState, &s_animSet, "hurt");
    }
    if (s_health <= 0.0f) TriggerPlayerDeath();
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
                if (obj == OBJ_BUTTON_UP)        surface = tileTop + (f32)TILE_SIZE * 0.50f;
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
            // Slopes never block horizontal movement (matches player ResolveX) — CrateResolveY
            // snaps the crate onto the surface every tick as it crosses, so it climbs smoothly.
            if (IsSlope(ct)) continue;
            if (IsSolid(ct)) {
                // Step-up: only if low enough and the column above the blocking tile is clear
                f32 tileTop = (f32)(ty * TILE_SIZE);
                f32 stepH   = (c->y + PT_CRATE_SIZE) - tileTop;
                bool clearAbove = !IsSolid(TileAt(right, ty - 1));
                if (stepH > 0.0f && stepH <= (f32)TILE_SIZE && c->vy >= 0.0f && clearAbove) {
                    c->y = tileTop - PT_CRATE_SIZE;
                    if (c->vy > 0.0f) c->vy = 0.0f;
                    return;
                }
                c->x  = (f32)(right * TILE_SIZE) - PT_CRATE_SIZE;
                c->vx = 0.0f;
                return;
            }
        }
    } else if (c->vx < 0.0f) {
        i32 left = (i32)(c->x / (f32)TILE_SIZE);
        for (i32 ty = top; ty <= bot; ty++) {
            CollisionType ct = TileAt(left, ty);
            if (IsSlope(ct)) continue;
            if (IsSolid(ct)) {
                f32 tileTop = (f32)(ty * TILE_SIZE);
                f32 stepH   = (c->y + PT_CRATE_SIZE) - tileTop;
                bool clearAbove = !IsSolid(TileAt(left, ty - 1));
                if (stepH > 0.0f && stepH <= (f32)TILE_SIZE && c->vy >= 0.0f && clearAbove) {
                    c->y = tileTop - PT_CRATE_SIZE;
                    if (c->vy > 0.0f) c->vy = 0.0f;
                    return;
                }
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
    CrateResolveX(c);  // re-check walls after vertical move

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

// True if the player's body fits here without overlapping solid tiles or an active crate.
static bool TileIsFreeForPlayer(i32 tx, i32 ty) {
    f32 testX = (f32)(tx * TILE_SIZE) + ((f32)TILE_SIZE - PT_BODY_W) * 0.5f;
    f32 testY = (f32)(ty * TILE_SIZE) + ((f32)TILE_SIZE - PT_BODY_H) * 0.5f;
    i32 l = (i32)(testX / (f32)TILE_SIZE);
    i32 r = (i32)((testX + PT_BODY_W - 0.5f) / (f32)TILE_SIZE);
    i32 t = (i32)(testY / (f32)TILE_SIZE);
    i32 b = (i32)((testY + PT_BODY_H - 0.5f) / (f32)TILE_SIZE);
    for (i32 cy = t; cy <= b; cy++)
        for (i32 cx = l; cx <= r; cx++)
            if (IsSolid(TileAt(cx, cy))) return false;
    for (i32 i = 0; i < s_crateCount; i++) {
        CrateBody* c = &s_crates[i];
        if (!c->active) continue;
        if (testX + PT_BODY_W > c->x && testX < c->x + PT_CRATE_SIZE &&
            testY + PT_BODY_H > c->y && testY < c->y + PT_CRATE_SIZE)
            return false;
    }
    return true;
}

// Searches outward ring-by-ring from (tx0, ty0) for the closest tile the player fits in.
static bool FindNearestFreeTile(i32 tx0, i32 ty0, i32* outTx, i32* outTy) {
    if (TileIsFreeForPlayer(tx0, ty0)) { *outTx = tx0; *outTy = ty0; return true; }
    for (i32 radius = 1; radius <= 10; radius++) {
        for (i32 dy = -radius; dy <= radius; dy++) {
            for (i32 dx = -radius; dx <= radius; dx++) {
                if (dx != -radius && dx != radius && dy != -radius && dy != radius) continue;
                i32 tx = tx0 + dx, ty = ty0 + dy;
                if (tx < 0 || tx >= WORLD_WIDTH_TILES || ty < 0 || ty >= WORLD_HEIGHT_TILES) continue;
                if (TileIsFreeForPlayer(tx, ty)) { *outTx = tx; *outTy = ty; return true; }
            }
        }
    }
    return false;
}

// A crate crushed the player against solid geometry — warp them out to the nearest open tile.
static void WarpPlayerToNearestFreeTile(void) {
    i32 tx0 = (i32)((s_body.x + PT_BODY_W * 0.5f) / (f32)TILE_SIZE);
    i32 ty0 = (i32)((s_body.y + PT_BODY_H * 0.5f) / (f32)TILE_SIZE);
    i32 fx, fy;
    if (FindNearestFreeTile(tx0, ty0, &fx, &fy)) {
        s_body.x        = (f32)(fx * TILE_SIZE) + ((f32)TILE_SIZE - PT_BODY_W) * 0.5f;
        s_body.y        = (f32)(fy * TILE_SIZE) + ((f32)TILE_SIZE - PT_BODY_H) * 0.5f;
        s_body.vx       = 0.0f;
        s_body.vy       = 0.0f;
        s_body.onGround = false;
    }
}

// Resolve player body against all active crates.
static void ResolvePlayerVsCrates(void) {
    f32 pw = PT_BODY_W, ph = PT_BODY_H;
    f32 cs = PT_CRATE_SIZE;
    s_isPushing = false;
    i32 newRiddenCrate = -1;
    bool left  = PT_IsDown(INPUT_LEFT);
    bool right = PT_IsDown(INPUT_RIGHT);
    for (i32 i = 0; i < s_crateCount; i++) {
        CrateBody* c = &s_crates[i];
        if (!c->active) continue;

        f32 pCx = s_body.x + pw * 0.5f, pCy = s_body.y + ph * 0.5f;
        f32 cCx = c->x    + cs * 0.5f,  cCy = c->y    + cs * 0.5f;
        f32 ox  = (pw + cs) * 0.5f - fabsf(pCx - cCx);
        f32 oy  = (ph + cs) * 0.5f - fabsf(pCy - cCy);

        // Treat as pushing if dpad is held toward crate within 2px contact range
        bool nearH = ox > -2.0f && oy > 0.0f;
        bool holdingToward = (pCx < cCx && right) || (pCx > cCx && left);
        if (nearH && holdingToward && ox <= 0.0f)
            s_isPushing = true;

        if (ox <= 0 || oy <= 0) continue;

        // Bias toward vertical resolution when falling onto a crate to prevent corner blipping
        bool preferVertical = (s_body.vy > 0.0f && pCy < cCy && oy < ox + 4.0f);
        if (!preferVertical && ox < oy) {
            f32 dir = (pCx < cCx) ? 1.0f : -1.0f;
            if (holdingToward) {
                // Horizontal contact — actively pushing the crate
                c->x += dir * ox;
                if (fabsf(s_body.vx) > PT_PUSH_SPEED + 2.0f) {
                    s_stamina -= 0.18f;
                    if (s_stamina < 0.0f) { s_stamina = 0.0f; s_exhaustTimer = PT_STAMINA_EXHAUST_T; s_runLocked = true; }
                    SpawnPushEffect(c->x + (dir > 0 ? 0.0f : PT_CRATE_SIZE),
                                    s_body.y + PT_BODY_H * 0.5f, dir);
                }
                c->vx = s_body.vx;
                CrateResolveX(c);
                // If CrateResolveX stopped the crate (hit a wall), push player back
                if (c->vx == 0.0f) {
                    f32 remainOx = (pw + cs) * 0.5f - fabsf((s_body.x + pw * 0.5f) - (c->x + cs * 0.5f));
                    if (remainOx > 0.0f) {
                        s_body.x -= dir * (remainOx + 0.5f);
                        s_body.vx = 0.0f;
                    }
                }
                s_isPushing = true;
            } else {
                // Passive contact (e.g. brushing past) — separate without moving the crate,
                // so it can't get shoved into a wall with nothing to push it back out.
                s_body.x -= dir * ox;
                s_body.vx = 0.0f;
            }
        } else {
            // Vertical contact
            if (pCy < cCy) {
                s_body.y        = c->y - ph;
                s_body.vy       = 0.0f;
                s_body.onGround = true;
                newRiddenCrate  = i;
            } else {
                f32 newY = c->y + cs;
                // Crush check: if there's solid geometry too close beneath, the player doesn't
                // fit in the gap — warp them out to the nearest open tile instead of clipping.
                bool crushed = false;
                i32 topRow = (i32)(newY / (f32)TILE_SIZE);
                i32 botRow = (i32)((newY + ph - 0.5f) / (f32)TILE_SIZE);
                i32 tL     = (i32)(s_body.x / (f32)TILE_SIZE);
                i32 tR     = (i32)((s_body.x + pw - 0.5f) / (f32)TILE_SIZE);
                for (i32 ty = topRow; ty <= botRow && !crushed; ty++)
                    for (i32 tx = tL; tx <= tR && !crushed; tx++)
                        if (IsSolid(TileAt(tx, ty))) crushed = true;
                if (crushed) {
                    WarpPlayerToNearestFreeTile();
                } else {
                    s_body.y  = newY;
                    s_body.vy = 0.0f;
                }
            }
        }
    }
    s_riddenCrate = newRiddenCrate;
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
    // This screen draws its own fade (behind the HUD/border) instead of letting
    // ScreenManager_Draw paint it over everything — see PhysTestScreen_Draw and _Unload.
    Visual_SetAutoFadeDrawSuppressed(&visualManager, true);

    Drip_Clear();  // wipe blood stains from the previous room on every (re)load
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
    s_menuRestart = pd->system->addMenuItem("Restart room", PT_RestartCallback, NULL);

    PlayAmb(&audioManager, AMB_DRONE, 100, true);

    AnimSet_Init(&s_animSet);
    AnimState_Init(&s_animState);
    AnimSet_LoadTable(&s_animSet, "assets/texture/spritesheet/LincolnPlaydate");
    DefineAnimations();
    AnimState_Play(&s_animState, &s_animSet, "idle");
    s_animCur = PS_IDLE;

    // Find the entrance door tile in the objects layer to use as spawn position — the player
    // spawns standing at the door they "walked out of". Falls back to OBJ_PLAYER (legacy spawn
    // marker), then the PT_SPAWN_X/Y constants if neither is present.
    f32 spawnX = PT_SPAWN_X, spawnY = PT_SPAWN_Y;
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] == OBJ_DOOR_ENTRANCE) {
                spawnX = (f32)(tx * TILE_SIZE);
                spawnY = (f32)(ty * TILE_SIZE);
                goto spawn_found;
            }
        }
    }
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
    s_riddenCrate  = -1;
    s_hurtTimer    = 0.0f;
    s_health       = 1.0f;
    s_meterLowPlaying = false;
    s_lightsOff    = false;
    s_sanityWasLow = false;
    s_glitchPhase  = GLITCH_NONE;
    s_glitchTimer  = 0.0f;
    s_lifecycle    = PT_LIFE_ALIVE;
    s_deathStaticTimer = 0.0f;
    s_doorTransition = PT_DOOR_NONE;
    s_dustSpawnTimer = 0.0f;
    for (i32 i = 0; i < PT_MAX_DUST; i++) s_dust[i].active = false;
    for (i32 i = 0; i < PT_MAX_PUSH_LINES; i++) s_pushLines[i].active = false;
    s_sanityLevel  = 0.0f;  // full sanity (SN bar 100%) at spawn
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

    // Scan for OBJ_PLATFORM_ACTIVE tiles to track — the CSV marks them ACTIVE purely as a
    // placement marker; force them to their real starting state here (inactive, no collision)
    // since they should only turn solid once a pressure plate is weighted.
    s_togglePlatformCount = 0;
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES && s_togglePlatformCount < PT_MAX_TOGGLE_PLATFORMS; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES && s_togglePlatformCount < PT_MAX_TOGGLE_PLATFORMS; tx++) {
            if (gWorld.tiles[LAYER_OBJECTS][ty][tx] != OBJ_PLATFORM_ACTIVE) continue;
            TogglePlatform* p = &s_togglePlatforms[s_togglePlatformCount++];
            p->tx = tx;
            p->ty = ty;
            p->active = false;
            gWorld.tiles[LAYER_OBJECTS][ty][tx]   = OBJ_PLATFORM_INACTIVE;
            gWorld.tiles[LAYER_COLLISION][ty][tx] = COL_EMPTY;
        }
    }

    // Local test-only inventory — cleared on every (re)load, same as the crate/platform state.
    for (i32 i = 0; i < PT_ITEM_SLOT_COUNT; i++) s_pickedItemObj[i] = -1;

    // Every (re)load starts covered in black and fades in, rather than cutting straight to a
    // fully-lit room. Stays paused (see PhysTestScreen_Update) until the fade finishes, so
    // nothing moves before the player can actually see the room.
    s_doorTransition = PT_DOOR_FADE_IN;
    Visual_FadeInBlack(&visualManager, PT_DOOR_FADE_DURATION);
}

void PhysTestScreen_Update(f32 deltaTime) {
    // Ticks independently of glitch phase so a drip still falling when GLITCH_BLOOD ends
    // (or the level restarts) finishes its fall instead of freezing mid-air.
    Drip_Update(deltaTime);
    Notify_Update(deltaTime);

    if (s_doorTransition == PT_DOOR_FADE_OUT) {
        // Pause everything (same idea as the death freeze) until the fade-to-black finishes,
        // then swap the room. PhysTestScreen_Init fades back in from black on its own.
        if (!Visual_IsFadeActive(&visualManager)) {
            s_doorTransition = PT_DOOR_NONE;
            PhysTestScreen_Unload();
            PhysTestScreen_Init();
        }
        return;
    }
    if (s_doorTransition == PT_DOOR_FADE_IN) {
        // Still paused through the fade-in — only once the room is fully visible does the door
        // behind the player swing shut and gameplay actually resume.
        if (!Visual_IsFadeActive(&visualManager)) {
            s_doorTransition = PT_DOOR_NONE;
            PlaySFX(&audioManager, SFX_DOOR_CLOSE, 200, false);
        }
        return;
    }
    if (s_glitchPhase == GLITCH_SNOW) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            PhysTestScreen_Unload();
            PhysTestScreen_Init();
        }
        return;
    }
    if (s_glitchPhase == GLITCH_OFF) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            s_glitchPhase = GLITCH_SNOW;
            s_glitchTimer = PT_DEATH_STATIC_DURATION;
            Visual_TriggerTVSnow(&visualManager, PT_DEATH_STATIC_DURATION);
            PlaySFX(&audioManager, SFX_STATIC, 200, false);
        }
        return;
    }
    if (s_glitchPhase == GLITCH_CRASH_BOOT) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            Asset_FreeBitmap(s_crashBootImg);
            s_crashBootImg = NULL;
            s_crashFrames[0] = Asset_LoadBitmap(IMG_CRASH_1);
            s_crashFrames[1] = Asset_LoadBitmap(IMG_CRASH_2);
            s_crashFrames[2] = Asset_LoadBitmap(IMG_CRASH_3);
            s_crashFrames[3] = Asset_LoadBitmap(IMG_CRASH_4);
            s_crashFrameIndex = 0;
            s_glitchPhase = GLITCH_CRASH_ANIM;
            s_glitchTimer = PT_CRASH_ANIM_FRAME_TIME;
        }
        return;
    }
    if (s_glitchPhase == GLITCH_CRASH_ANIM) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            s_crashFrameIndex++;
            if (s_crashFrameIndex >= 3) {
                // Landed on dump_out4 ("You need to restart your Playdate...") — hold, then
                // auto-advance straight into the level restart, no extra fanfare.
                s_glitchPhase = GLITCH_CRASH_HOLD;
                s_glitchTimer = PT_CRASH_HOLD_DURATION;
            } else {
                s_glitchTimer = PT_CRASH_ANIM_FRAME_TIME;
            }
        }
        return;
    }
    if (s_glitchPhase == GLITCH_CRASH_HOLD) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            // Free the crash frames now — GLITCH_SNOW's Draw doesn't touch them, and Unload
            // will no-op on the already-cleared pointers when the level actually restarts.
            for (i32 i = 0; i < 4; i++) {
                if (s_crashFrames[i]) {
                    Asset_FreeBitmap(s_crashFrames[i]);
                    s_crashFrames[i] = NULL;
                }
            }
            s_glitchPhase = GLITCH_SNOW;
            s_glitchTimer = PT_DEATH_STATIC_DURATION;
            StopAllAmb(&audioManager);
            Visual_TriggerTVSnow(&visualManager, PT_DEATH_STATIC_DURATION);
            PlaySFX(&audioManager, SFX_STATIC, 200, false);
        }
        return;
    }
    if (s_lifecycle == PT_LIFE_STATIC) {
        s_deathStaticTimer -= deltaTime;
        if (s_deathStaticTimer <= 0.0f) {
            s_lifecycle = PT_LIFE_FMV;
            FMV_Init(&s_deathFMV, DEATH_FMV_PATH, false);
            FMV_Play(&s_deathFMV);
        }
        return;
    }
    if (s_lifecycle == PT_LIFE_FMV) {
        FMV_Update(&s_deathFMV, deltaTime);
        if (FMV_IsFinished(&s_deathFMV)) {
            PhysTestScreen_Unload();
            PhysTestScreen_Init();
        }
        return;
    }

    if (Signage_IsActive()) {
        // Physics is frozen while a sign is up, so a crate mid-slide never re-checks its
        // velocity and the loop below never gets a chance to stop the move sound.
        if (s_crateMoving) {
            StopSFX(&audioManager, SFX_CRATE);
            s_crateMoving = false;
        }
        Signage_Update(deltaTime);
        return;
    }

    Visual_UpdateManager(&visualManager, deltaTime);

    if (s_hurtTimer > 0.0f) {
        s_hurtTimer -= deltaTime;
        if (s_hurtTimer < 0.0f) s_hurtTimer = 0.0f;
    }

    bool left       = PT_IsDown(INPUT_LEFT);
    bool right      = PT_IsDown(INPUT_RIGHT);
    bool leftPress  = PT_IsPressed(INPUT_LEFT);
    bool rightPress = PT_IsPressed(INPUT_RIGHT);
    bool down      = PT_IsDown(INPUT_DOWN);
    bool downPress = PT_IsPressed(INPUT_DOWN);
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
    if (s_hurtTimer > 0.0f) {
        // Reeling from a spike hit — knockback velocity drives movement, ignore input
    } else if (s_inVent) {
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
    if (s_hurtTimer <= 0.0f && s_isPushing && s_body.onGround) {
        f32 maxPush = (s_exhaustTimer > 0.0f) ? 0.0f : PT_PUSH_SPEED;
        if (s_body.vx >  maxPush) s_body.vx =  maxPush;
        if (s_body.vx < -maxPush) s_body.vx = -maxPush;
    }

    // Stamina drain/regen — pushing crates or running both cost stamina (resting in a vent never does)
    bool isExerting = !s_inVent && (s_isPushing || s_body.isRunning);
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

    if (s_hurtTimer <= 0.0f && !s_inVent && PT_IsPressed(INPUT_A)) {
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
    f32 preCrateX[PT_MAX_CRATES], preCrateY[PT_MAX_CRATES];
    for (i32 i = 0; i < s_crateCount; i++) {
        preCrateX[i] = s_crates[i].x;
        preCrateY[i] = s_crates[i].y;
        if (s_crates[i].active) CratePhysStep(&s_crates[i], deltaTime);
    }
    for (i32 i = 0; i < s_crateCount; i++) {
        if (s_crates[i].active) ResolveCrateVsCrates(i);
    }

    // Carry the player with whatever crate they were standing on last frame — this is what
    // makes a crate act as a moving platform (e.g. while a spring is launching it) instead of
    // relying on overlap alone, which can't keep up with fast crate motion.
    if (s_riddenCrate >= 0 && s_riddenCrate < s_crateCount && s_crates[s_riddenCrate].active) {
        s_body.x += s_crates[s_riddenCrate].x - preCrateX[s_riddenCrate];
        s_body.y += s_crates[s_riddenCrate].y - preCrateY[s_riddenCrate];
    }

    if (!s_inVent) PhysStep(&s_body, deltaTime);
    ResolvePlayerVsCrates();

    // Spike hazard — knock the player back and lock out input briefly on contact
    if (s_hurtTimer <= 0.0f && !s_inVent && PlayerTouchingSpike()) {
        f32 knockDir = s_body.facingRight ? -1.0f : 1.0f;
        TriggerPlayerHurt(knockDir);
    }

    // Crate move sound — loop while any crate has lateral velocity, stop when all still
    bool anyCrateMoving = false;
    for (i32 i = 0; i < s_crateCount; i++) {
        if (s_crates[i].active && fabsf(s_crates[i].vx) > 1.0f)
            anyCrateMoving = true;
    }
    if (anyCrateMoving && !s_crateMoving)
        PlaySFX(&audioManager, SFX_CRATE, 160, true);
    else if (!anyCrateMoving && s_crateMoving)
        StopSFX(&audioManager, SFX_CRATE);
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
            PlaySFX(&audioManager, SFX_BUTTON, 180, false);
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
            PlaySFX(&audioManager, SFX_BUTTON, 180, false);
        }
    }

    UpdatePressurePlates();
    UpdateTogglePlatforms();
    TryPickupItems();

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

    // Spring launch — crates (check left, center, right foot points)
    f32 sampleXOff[3] = { 1.0f, PT_CRATE_SIZE * 0.5f, PT_CRATE_SIZE - 1.0f };
    for (i32 i = 0; i < s_crateCount; i++) {
        CrateBody* cr = &s_crates[i];
        if (!cr->active || !cr->onGround) continue;
        i32 fty = (i32)((cr->y + PT_CRATE_SIZE - 1.0f) / (f32)TILE_SIZE);
        if (fty >= WORLD_HEIGHT_TILES) continue;
        for (i32 s = 0; s < 3; s++) {
            i32 ftx = (i32)((cr->x + sampleXOff[s]) / (f32)TILE_SIZE);
            if (gWorld.tiles[LAYER_OBJECTS][fty][ftx] == OBJ_SPRING_A) {
                cr->vy       = PT_SPRING_VEL;
                cr->onGround = false;
                s_springTx = ftx; s_springTy = fty; s_springFrames = PT_SPRING_FRAMES;
                PlaySFX(&audioManager, SFX_SPRING, 180, false);
                // launch player too if they're riding this crate
                f32 pw = PT_BODY_W, ph = PT_BODY_H, cs = PT_CRATE_SIZE;
                f32 pCx = s_body.x + pw * 0.5f, pCy = s_body.y + ph * 0.5f;
                f32 cCx = cr->x   + cs * 0.5f,  cCy = cr->y   + cs * 0.5f;
                f32 ox  = (pw + cs) * 0.5f - fabsf(pCx - cCx);
                f32 oy  = (ph + cs) * 0.5f - fabsf(pCy - cCy);
                if (ox > 0.0f && oy > 0.0f && pCy < cCy) {
                    s_body.vy           = PT_SPRING_VEL;
                    s_body.onGround     = false;
                    s_body.canDoubleJump = true;
                }
                break;
            }
        }
    }

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
    if (s_hurtTimer > 0.0f)           next = PS_HURT;
    else if (!s_body.onGround)        next = (s_body.vy < 0.0f) ? PS_JUMP : PS_FALL;
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
            case PS_HURT:   AnimState_Play(&s_animState, &s_animSet, "hurt");   break;
        }
    }
    AnimState_Update(&s_animState, &s_animSet, deltaTime);

    // Low-meter warning — loops while health or sanity is below 20%, stops once both recover
    bool healthLow = s_health < PT_METER_LOW_THRESHOLD;
    bool sanityLow = (1.0f - s_sanityLevel) < PT_METER_LOW_THRESHOLD;
    bool anyMeterLow = healthLow || sanityLow;
    if (anyMeterLow && !s_meterLowPlaying)
        PlaySFX(&audioManager, SFX_METER_LOW, 180, true);
    else if (!anyMeterLow && s_meterLowPlaying)
        StopSFX(&audioManager, SFX_METER_LOW);
    s_meterLowPlaying = anyMeterLow;

    // Roll for a sanity glitch once per low-sanity dip (edge-triggered), not every frame, so it
    // doesn't re-roll as sanity hovers around the threshold. If it hits, pick which glitch fires.
    if (PT_SANITY_EFFECTS_ENABLED && sanityLow && !s_sanityWasLow && s_glitchPhase == GLITCH_NONE) {
        LOG("Sanity glitch roll: %d%% chance", s_glitchChancePct);
        if ((rand() % 100) < s_glitchChancePct) {
            s_glitchChancePct = PT_SANITY_GLITCH_CHANCE_MIN;  // cool down — no back-to-back glitches
            i32 pick;
            do {
                pick = rand() % 7;
            } while (pick == s_lastGlitchPick);  // never the same glitch twice in a row
            s_lastGlitchPick = pick;
            if (pick == 0) {
                LOG("Sanity glitch incoming: DIM");
                s_glitchPhase = GLITCH_DIM;
                s_glitchTimer = kGlitchDimDurations[rand() % 4];
            } else if (pick == 1) {
                LOG("Sanity glitch incoming: FAKE CRASH");
                s_glitchPhase = GLITCH_CRASH_BOOT;
                s_glitchTimer = PT_CRASH_BOOT_DURATION;
                s_body.vx = 0.0f;
                s_body.vy = 0.0f;
                StopSFX(&audioManager, SFX_CRATE);
                StopSFX(&audioManager, SFX_METER_LOW);
                StopAllAmb(&audioManager);
                s_crashBootImg = Asset_LoadBitmap(IMG_CRASH_BOOT);
            } else if (pick == 2) {
                LOG("Sanity glitch incoming: FAKE TURN-OFF");
                s_glitchPhase = GLITCH_OFF;
                s_glitchTimer = PT_TURNOFF_DURATION;
                s_body.vx = 0.0f;
                s_body.vy = 0.0f;
                StopSFX(&audioManager, SFX_CRATE);
                StopSFX(&audioManager, SFX_METER_LOW);
                StopAllAmb(&audioManager);
            } else if (pick == 3) {
                LOG("Sanity glitch incoming: BLOOD");
                s_glitchPhase = GLITCH_BLOOD;
                s_glitchTimer = kGlitchBloodDurations[rand() % 4];
                s_bloodSpawnTimer = 0.0f;  // spawn the first trickle immediately
            } else if (pick == 4) {
                LOG("Sanity glitch incoming: INVERT CONTROLS");
                s_glitchPhase = GLITCH_INVERT_CONTROLS;
                s_glitchTimer = kGlitchInvertControlsDurations[rand() % 4];
            } else if (pick == 5) {
                LOG("Sanity glitch incoming: INVERT VISUALS");
                s_glitchPhase = GLITCH_INVERT_VISUALS;
                s_glitchTimer = kGlitchInvertVisualsDurations[rand() % 4];
            } else {
                LOG("Sanity glitch incoming: MISSING OBJECTS");
                s_glitchPhase = GLITCH_MISSING_OBJECTS;
                s_glitchTimer = kGlitchMissingObjDurations[rand() % 4];
            }
        } else {
            s_glitchChancePct = MinI(s_glitchChancePct + PT_SANITY_GLITCH_CHANCE_STEP, PT_SANITY_GLITCH_CHANCE_START);
            LOG("Sanity glitch missed — next chance %d%%", s_glitchChancePct);
        }
    }
    s_sanityWasLow = sanityLow;
    s_lightsOff    = (s_glitchPhase == GLITCH_DIM);

    // The dim period runs its full random course independent of live sanity — once it expires,
    // snap back to normal and burn through a TV-snow burst before restarting the level.
    if (s_glitchPhase == GLITCH_DIM) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            s_glitchPhase = GLITCH_SNOW;
            s_glitchTimer = PT_DEATH_STATIC_DURATION;
            s_body.vx = 0.0f;
            s_body.vy = 0.0f;
            StopSFX(&audioManager, SFX_CRATE);
            StopSFX(&audioManager, SFX_METER_LOW);
            StopAllAmb(&audioManager);
            Visual_TriggerTVSnow(&visualManager, PT_DEATH_STATIC_DURATION);
            PlaySFX(&audioManager, SFX_STATIC, 200, false);
        }
    }

    // BLOOD, INVERT_CONTROLS, INVERT_VISUALS and MISSING_OBJECTS are all passive glitches too —
    // no early return, so movement/physics below still runs. Each just needs its own clock
    // ticked down here before handing off to the TV-snow burst.
    if (s_glitchPhase == GLITCH_BLOOD) {
        s_bloodSpawnTimer -= deltaTime;
        if (s_bloodSpawnTimer <= 0.0f) {
            s_bloodSpawnTimer = PT_BLOOD_SPAWN_INTERVAL;
            Drip_SpawnRandom(PT_BLOOD_SPAWN_COUNT, 40.0f, (f32)SCR_H);
        }
    }
    if (s_glitchPhase == GLITCH_BLOOD
     || s_glitchPhase == GLITCH_INVERT_CONTROLS
     || s_glitchPhase == GLITCH_INVERT_VISUALS
     || s_glitchPhase == GLITCH_MISSING_OBJECTS) {
        s_glitchTimer -= deltaTime;
        if (s_glitchTimer <= 0.0f) {
            s_glitchPhase = GLITCH_SNOW;
            s_glitchTimer = PT_DEATH_STATIC_DURATION;
            s_body.vx = 0.0f;
            s_body.vy = 0.0f;
            StopSFX(&audioManager, SFX_CRATE);
            StopSFX(&audioManager, SFX_METER_LOW);
            StopAllAmb(&audioManager);
            Visual_TriggerTVSnow(&visualManager, PT_DEATH_STATIC_DURATION);
            PlaySFX(&audioManager, SFX_STATIC, 200, false);
        }
    }

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
    // Update push-impact lines
    for (i32 i = 0; i < PT_MAX_PUSH_LINES; i++) {
        if (!s_pushLines[i].active) continue;
        s_pushLines[i].x    += s_pushLines[i].vx * deltaTime;
        s_pushLines[i].life -= deltaTime * 5.0f;
        s_pushLines[i].len  *= 0.85f;
        if (s_pushLines[i].life <= 0.0f) s_pushLines[i].active = false;
    }

    bool upPressed = PT_IsPressed(INPUT_UP);
    bool upConsumed = false;

    // Sign interaction — open on UP press near a sign. Independent of the sanity-glitch
    // roll/cooldown system entirely: below 20% sanity, every sign read has a flat 33% chance
    // of showing a random self-deprecating line instead (can't be dismissed manually — see
    // Signage_ShowGlitch).
    i32 stx, sty;
    if (upPressed && NearSign(&stx, &sty)) {
        if (PT_SANITY_EFFECTS_ENABLED && sanityLow && (rand() % 100) < PT_BLANK_SIGN_CHANCE_PCT) {
            const char* phrase = kBlankSignPhrases[rand() % PT_BLANK_SIGN_PHRASE_COUNT];
            LOG("Blank sign glitch triggered: \"%s\"", phrase);
            Signage_ShowGlitch(phrase, PT_BLANK_SIGN_HOLD_DURATION);
        } else {
            Signage_Show("Notice", "This is a test sign. Press { or } to dismiss.");
        }
        upConsumed = true;
    }

    // Vent interaction
    i32 vtx, vty;
    if (upPressed) {
        if (s_inVent) {
            // Exit vent — pop player out just below the vent tile
            s_body.x    = (f32)(s_ventTx * TILE_SIZE) + (TILE_SIZE - PT_BODY_W) * 0.5f;
            s_body.y    = (f32)((s_ventTy + 1) * TILE_SIZE);
            s_body.vx   = 0.0f;
            s_body.vy   = 0.0f;
            s_inVent    = false;
            s_ventTx    = -1;
            s_ventTy    = -1;
            PlaySFX(&audioManager, SFX_VENT_CLOSE, 200, false);
            upConsumed = true;
        } else if (NearVent(&vtx, &vty)) {
            // Enter vent — snap player into vent tile and freeze
            s_body.x    = (f32)(vtx * TILE_SIZE) + (TILE_SIZE - PT_BODY_W) * 0.5f;
            s_body.y    = (f32)(vty * TILE_SIZE) + (TILE_SIZE - PT_BODY_H) * 0.5f;
            s_body.vx   = 0.0f;
            s_body.vy   = 0.0f;
            s_inVent    = true;
            s_ventTx    = vtx;
            s_ventTy    = vty;
            PlaySFX(&audioManager, SFX_VENT_OPEN, 200, false);
            upConsumed = true;
        }
    }

    // Lever interaction — toggle on UP press near a lever
    i32 ltx, lty;
    if (upPressed && !s_inVent && NearLever(&ltx, &lty)) {
        i32 leverObj = gWorld.tiles[LAYER_OBJECTS][lty][ltx];
        gWorld.tiles[LAYER_OBJECTS][lty][ltx] = (leverObj == OBJ_LEVER_OFF) ? OBJ_LEVER_ON : OBJ_LEVER_OFF;
        PlaySFX(&audioManager, SFX_BUTTON, 180, false);
        upConsumed = true;
    }

    // Exit door — locked until the key is picked up (see UnlockExitDoors). Once unlocked,
    // walking out through it restarts the room (placeholder until real room-to-room transitions
    // exist), pausing everything and fading to black first, same as the death sequence, rather
    // than cutting the room instantly.
    i32 etx, ety;
    if (upPressed && !s_inVent && NearExitDoor(&etx, &ety)) {
        if (gWorld.tiles[LAYER_OBJECTS][ety][etx] == OBJ_DOOR_LOCKED) {
            PlaySFX(&audioManager, SFX_DOOR_LOCKED, 200, false);
            Notify_Show("Locked. Find the key first.", 2.5f);
        } else {
            PlaySFX(&audioManager, SFX_DOOR_OPEN, 200, false);
            s_body.vx = 0.0f;
            s_body.vy = 0.0f;
            StopSFX(&audioManager, SFX_CRATE);
            StopSFX(&audioManager, SFX_METER_LOW);
            StopAllAmb(&audioManager);
            s_doorTransition = PT_DOOR_FADE_OUT;
            Visual_FadeOutBlack(&visualManager, PT_DOOR_FADE_DURATION);
        }
        return;
    }

    if (PT_IsPressed(INPUT_B))
        SwitchScreen(&screenManager, SS_TEST_WORLD);
    if (upPressed && !s_inVent && !upConsumed)
        SwitchScreen(&screenManager, SS_TEST_6);
}

void PhysTestScreen_Draw(void) {
    if (s_lifecycle == PT_LIFE_FMV) {
        pd->graphics->clear(kColorBlack);
        FMV_Draw(&s_deathFMV);
        Visual_DrawFade(&visualManager);
        return;
    }
    if (s_glitchPhase == GLITCH_CRASH_BOOT) {
        pd->graphics->clear(kColorBlack);
        pd->graphics->setDrawMode(kDrawModeCopy);
        if (s_crashBootImg) pd->graphics->drawBitmap(s_crashBootImg, 0, 0, kBitmapUnflipped);
        Visual_DrawFade(&visualManager);
        return;
    }
    if (s_glitchPhase == GLITCH_CRASH_ANIM || s_glitchPhase == GLITCH_CRASH_HOLD) {
        pd->graphics->clear(kColorBlack);
        pd->graphics->setDrawMode(kDrawModeCopy);
        LCDBitmap* frame = s_crashFrames[s_crashFrameIndex];
        if (frame) pd->graphics->drawBitmap(frame, 0, 0, kBitmapUnflipped);
        Visual_DrawFade(&visualManager);
        return;
    }
    if (s_glitchPhase == GLITCH_OFF) {
        // Plain white screen, like the Playdate's reflective LCD with the backlight off —
        // TV snow (and its audio cue) only kicks in once GLITCH_OFF hands off to GLITCH_SNOW.
        pd->graphics->clear(kColorWhite);
        Visual_DrawFade(&visualManager);
        return;
    }

    pd->graphics->clear(kColorBlack);
    Visual_ApplyShakeOffset(&visualManager);

    // 1. Debug collision geometry (white shapes on black)
    DrawCollisionDebug();

    // 2. Blood stains — accumulated drips from past GLITCH_BLOOD glitches, plus any still
    // actively falling. Drawn over the world geometry, under objects/player.
    Drip_Draw();

    // 3. Objects from CSV — skip OBJ_PLAYER (spawn marker), use BlackTransparent so background shows through
    // Skipped entirely during GLITCH_MISSING_OBJECTS — collision/interaction logic is untouched,
    // the sprites just stop drawing.
    if (s_objTable && s_glitchPhase != GLITCH_MISSING_OBJECTS) {
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
    if (PT_SMOKE_CLOUDS_ENABLED) {
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
    // 5. Radial lights — drawn before player so player always renders on top.
    // While sanity is low, the glow is killed and the fixture draws unlit (Idea #3: "all lights
    // in a level are off").
    pd->graphics->setDrawMode(kDrawModeBlackTransparent);
    for (i32 i = 0; i < s_lightCount; i++) {
        if (!s_lightsOff)
            PT_DrawRadialLight(s_lightCX[i], s_lightCY[i], 38, s_smokeTimer);
        if (s_objTable) {
            pd->graphics->setDrawMode(s_lightsOff ? kDrawModeBlackTransparent : kDrawModeInverted);
            LCDBitmap* lbmp = pd->graphics->getTableBitmap(s_objTable, OBJ_LIGHT_A);
            if (lbmp) pd->graphics->drawBitmap(lbmp,
                s_lightCX[i] - TILE_SIZE/2, s_lightCY[i] - TILE_SIZE/2, kBitmapUnflipped);
            pd->graphics->setDrawMode(kDrawModeBlackTransparent);
        }
    }
    pd->graphics->setDrawMode(kDrawModeCopy);

    // 6. Smoke clouds — drawn before player so player always renders on top
    if (PT_SMOKE_CLOUDS_ENABLED) {
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
    }

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

    // 6c. Push-impact lines
    for (i32 i = 0; i < PT_MAX_PUSH_LINES; i++) {
        if (!s_pushLines[i].active) continue;
        i32 x1 = (i32)s_pushLines[i].x;
        i32 x2 = x1 + (i32)(s_pushLines[i].len * (s_pushLines[i].vx > 0 ? 1 : -1));
        i32 y  = (i32)s_pushLines[i].y;
        pd->graphics->drawLine(x1, y, x2, y, 1, kColorWhite);
    }

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

    // Stamina bar above player when below 50%
    if (!s_inVent && s_stamina < 0.99f) {
        i32 sbW = 16, sbH = 2;
        i32 sbX = (i32)(s_body.x + PT_BODY_W * 0.5f) - sbW / 2;
        i32 sbY = (i32)s_body.y - 6;
        f32 blinkRate = 30.0f;
        bool blink = s_stamina < 0.25f && (i32)(s_smokeTimer * blinkRate) % 2 == 1;
        pd->graphics->drawRect(sbX - 1, sbY - 1, sbW + 2, sbH + 2, kColorWhite);
        if (!blink) {
            i32 fill = (i32)(s_stamina * (f32)sbW);
            if (fill > 0) pd->graphics->fillRect(sbX, sbY, fill, sbH, kColorWhite);
        }
    }

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
    i32 ltx, lty;
    if (!s_inVent && NearLever(&ltx, &lty)) {
        i32 px = ltx * TILE_SIZE + (TILE_SIZE - 8) / 2;
        i32 py = lty * TILE_SIZE - 10;
        DrawIconText("^", px, py, 2, kColorWhite, 10);
    }

    // Lights-off sanity effect — dark dither over the whole world so it reads as darkness
    // regardless of the collision-debug geometry drawn underneath.
    if (s_lightsOff) Visual_DrawDarken(0.75f);

    // Sign box drawn last (on top of everything)
    Signage_Draw();
    Notify_Draw();

    Visual_ClearShakeOffset();

    // Door fade drawn here — behind the border/HUD — instead of over the whole frame, so the
    // HUD stays fully readable/unaffected while the world underneath fades.
    Visual_DrawFade(&visualManager);

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
        static const cstr labels[3] = { "HP", "SP", "SN" };
        const f32 vals[3] = { s_health, s_stamina, 1.0f - s_sanityLevel };
        i32 barW = 52, barH = 2;
        for (i32 i = 0; i < 3; i++) {
            i32 rowY = 214 + i * 8;
            DrawText(labels[i], 6, rowY, 2, kColorWhite);
            i32 bx = 22;
            f32 blinkRate = (i == 1) ? 24.0f : 6.0f;
            bool blink = vals[i] < 0.25f && (i32)(s_smokeTimer * blinkRate) % 2 == 1;
            i32 fill = blink ? 0 : (i32)(vals[i] * (f32)barW);
            i32 barY = rowY + 3;
            if (fill > 0) pd->graphics->fillRect(bx, barY, fill, barH, kColorWhite);
            pd->graphics->drawRect(bx - 1, barY - 1, barW + 2, barH + 2, kColorWhite);
        }
    }

    // 3 item slots in the bottom panel, right-aligned — filled ones draw the picked-up item's
    // own tileset icon inside the slot.
    for (i32 i = 0; i < PT_ITEM_SLOT_COUNT; i++) {
        i32 slotX = 310 + i * 20, slotY = 220;
        pd->graphics->drawRect(slotX, slotY, 14, 14, kColorWhite);
        if (s_pickedItemObj[i] >= 0 && s_objTable) {
            LCDBitmap* bmp = pd->graphics->getTableBitmap(s_objTable, s_pickedItemObj[i]);
            if (bmp) {
                pd->graphics->setDrawMode(kDrawModeBlackTransparent);
                pd->graphics->drawBitmap(bmp, slotX - 1, slotY - 1, kBitmapUnflipped);
                pd->graphics->setDrawMode(kDrawModeCopy);
            }
        }
    }

    // GLITCH_INVERT_VISUALS — invert the whole frame (world + HUD) as the very last step.
    if (s_glitchPhase == GLITCH_INVERT_VISUALS)
        pd->graphics->fillRect(0, 0, SCR_W, SCR_H, kColorXOR);
}

bool PhysTestScreen_IsSanityGlitchActive(void) {
    return s_glitchPhase == GLITCH_OFF
        || s_glitchPhase == GLITCH_CRASH_BOOT
        || s_glitchPhase == GLITCH_CRASH_ANIM
        || s_glitchPhase == GLITCH_CRASH_HOLD;
}

bool PhysTestScreen_IsSanityLow(void) {
    return PT_SANITY_EFFECTS_ENABLED && (1.0f - s_sanityLevel) < PT_METER_LOW_THRESHOLD;
}

void PhysTestScreen_Unload(void) {
    // Restore normal engine-wide fade drawing for whichever screen loads next. If this is just
    // an in-place room restart, the immediately-following PhysTestScreen_Init sets it right
    // back to true before any Draw() call happens.
    Visual_SetAutoFadeDrawSuppressed(&visualManager, false);

    if (s_lifecycle == PT_LIFE_FMV) {
        FMV_Free(&s_deathFMV);
    }
    s_lifecycle = PT_LIFE_ALIVE;
    if (s_crashBootImg) {
        Asset_FreeBitmap(s_crashBootImg);
        s_crashBootImg = NULL;
    }
    for (i32 i = 0; i < 4; i++) {
        if (s_crashFrames[i]) {
            Asset_FreeBitmap(s_crashFrames[i]);
            s_crashFrames[i] = NULL;
        }
    }
    s_glitchPhase = GLITCH_NONE;
    if (s_menuRestart) {
        pd->system->removeMenuItem(s_menuRestart);
        s_menuRestart = NULL;
    }
    // Stop looping SFX/ambience — their loop state lives in the AudioManager, not the screen,
    // so a restart would otherwise leave them playing forever in the background.
    StopSFX(&audioManager, SFX_CRATE);
    StopSFX(&audioManager, SFX_METER_LOW);
    StopAllAmb(&audioManager);
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
    Visual_ClearShakeOffset();
}
