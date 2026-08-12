// written by diskodev
// game/screens/enemy_test.c
// Enemy AI showcase — no enemy art yet, so these are placeholder blobs. Goomba-style patrol:
// walk in a straight line, turn around at a wall or the moment the floor runs out underneath.
// No player in this screen — just watching the patrol behavior itself.
#include "enemy_test.h"
#include <engine/engine_core.h>
#include <game/data/world.h>

extern PlaydateAPI*   pd;
extern InputManager   inputManager;
extern ScreenManager  screenManager;

#define ENEMY_W 14
#define ENEMY_H 14
#define ENEMY_SPEED    40.0f   // px/sec
#define ENEMY_GRAVITY  600.0f  // px/sec^2
#define ENEMY_MAX_FALL 300.0f  // px/sec

typedef struct {
    f32  x, y;
    f32  vy;
    bool facingRight;
    bool onGround;
} Enemy;

#define ENEMY_COUNT 3
static Enemy s_enemies[ENEMY_COUNT];

static bool IsGroundish(CollisionType ct) {
    return ct == COL_SOLID || ct == COL_SOLID_DUP || ct == COL_PLATFORM || ct == COL_HALF_BOTTOM;
}

// Gravity + landing on solid/platform/half-bottom ground. No slopes here — this room doesn't
// have any, and enemies don't need to handle them for a first pass.
static void EnemyResolveY(Enemy* e, f32 deltaTime) {
    e->vy += ENEMY_GRAVITY * deltaTime;
    if (e->vy > ENEMY_MAX_FALL) e->vy = ENEMY_MAX_FALL;
    e->y += e->vy * deltaTime;

    f32 footX = e->x + ENEMY_W * 0.5f;
    f32 footY = e->y + ENEMY_H;
    i32 tx = (i32)(footX / (f32)TILE_SIZE);
    i32 ty = (i32)(footY / (f32)TILE_SIZE);
    CollisionType ct = World_GetCollisionType(&gWorld, tx, ty);

    e->onGround = false;
    if (e->vy >= 0.0f && IsGroundish(ct)) {
        f32 surface = (f32)(ty * TILE_SIZE);
        if (ct == COL_HALF_BOTTOM) surface += (f32)TILE_SIZE * 0.5f;
        if (footY >= surface) {
            e->y = surface - ENEMY_H;
            e->vy = 0.0f;
            e->onGround = true;
        }
    }
}

// Classic Goomba turn conditions, checked one tile ahead of the leading edge: a wall at body
// height, or no floor at all (about to walk off a ledge).
static void EnemyUpdatePatrol(Enemy* e) {
    if (!e->onGround) return;

    f32 leadX = e->facingRight ? (e->x + (f32)ENEMY_W + 1.0f) : (e->x - 1.0f);
    f32 footY = e->y + (f32)ENEMY_H;

    i32 wallTx = (i32)(leadX / (f32)TILE_SIZE);
    i32 wallTy = (i32)((e->y + (f32)ENEMY_H * 0.5f) / (f32)TILE_SIZE);
    CollisionType wallCt = World_GetCollisionType(&gWorld, wallTx, wallTy);
    bool wallAhead = (wallCt == COL_SOLID || wallCt == COL_SOLID_DUP);

    i32 ledgeTx = wallTx;
    i32 ledgeTy = (i32)((footY + 2.0f) / (f32)TILE_SIZE);
    CollisionType ledgeCt = World_GetCollisionType(&gWorld, ledgeTx, ledgeTy);
    bool floorAhead = IsGroundish(ledgeCt);

    if (wallAhead || !floorAhead) {
        e->facingRight = !e->facingRight;
    }
}

static void SpawnEnemy(i32 index, i32 tx, i32 ty, bool facingRight) {
    Enemy* e = &s_enemies[index];
    e->x = (f32)(tx * TILE_SIZE);
    e->y = (f32)(ty * TILE_SIZE) - (f32)ENEMY_H;
    e->vy = 0.0f;
    e->facingRight = facingRight;
    e->onGround = false;
}

void EnemyTestScreen_Init(void) {
    World_Init(pd, &gWorld, NULL);

    cstr paths[LAYER_COUNT] = {
        NULL, NULL, NULL, NULL,
        "assets/data/csv/enemy_test/enemy_test_Collision.csv"
    };
    if (!World_LoadCSV(pd, &gWorld, paths))
        LOG("EnemyTest: failed to load room CSV");

    // A: patrols the floating platform (tx3-7 @ ty8) — turns at both ledges, never hits a wall.
    SpawnEnemy(0, 5, 8, true);
    // B: patrols the left floor (tx1-8 @ ty12) — bounces between the left wall and the gap edge.
    SpawnEnemy(1, 4, 12, false);
    // C: patrols the right floor (tx12-22 @ ty12) — bounces off the pillar at tx18 and the gap edge.
    SpawnEnemy(2, 14, 12, true);
}

void EnemyTestScreen_Update(f32 deltaTime) {
    for (i32 i = 0; i < ENEMY_COUNT; i++) {
        Enemy* e = &s_enemies[i];
        EnemyResolveY(e, deltaTime);
        EnemyUpdatePatrol(e);
        e->x += (e->facingRight ? 1.0f : -1.0f) * ENEMY_SPEED * deltaTime;
    }

    if (Input_IsPressed(&inputManager, INPUT_B))
        SwitchScreen(&screenManager, SS_TEST_4);
    if (Input_IsPressed(&inputManager, INPUT_UP))
        SwitchScreen(&screenManager, SS_TEST_8);
}

// Plain white shapes on black, same debug convention as the physics test screen — there's no
// enemy art yet, so this is a rectangle body with a black "eye" toward the facing direction.
static void DrawCollisionDebug(void) {
    for (i32 ty = 0; ty < WORLD_HEIGHT_TILES; ty++) {
        for (i32 tx = 0; tx < WORLD_WIDTH_TILES; tx++) {
            CollisionType ct = World_GetCollisionType(&gWorld, tx, ty);
            if (ct == COL_EMPTY || ct == COL_NULL) continue;
            i32 px = tx * TILE_SIZE, py = ty * TILE_SIZE;
            if (ct == COL_SOLID || ct == COL_SOLID_DUP) {
                pd->graphics->fillRect(px, py, TILE_SIZE, TILE_SIZE, kColorWhite);
            } else if (ct == COL_PLATFORM) {
                pd->graphics->fillRect(px, py, TILE_SIZE, 3, kColorWhite);
            } else if (ct == COL_HALF_BOTTOM) {
                pd->graphics->fillRect(px, py + TILE_SIZE / 2, TILE_SIZE, TILE_SIZE / 2, kColorWhite);
            }
        }
    }
}

static void DrawEnemy(const Enemy* e) {
    i32 x = (i32)e->x, y = (i32)e->y;
    pd->graphics->fillRoundRect(x, y, ENEMY_W, ENEMY_H, 3, kColorWhite);
    i32 eyeX = e->facingRight ? x + ENEMY_W - 5 : x + 2;
    pd->graphics->fillRect(eyeX, y + 4, 3, 3, kColorBlack);
}

void EnemyTestScreen_Draw(void) {
    pd->graphics->clear(kColorBlack);
    DrawCollisionDebug();
    for (i32 i = 0; i < ENEMY_COUNT; i++) DrawEnemy(&s_enemies[i]);
}

void EnemyTestScreen_Unload(void) {
    World_Free(pd, &gWorld);
}
