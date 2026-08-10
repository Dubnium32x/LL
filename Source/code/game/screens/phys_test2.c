// written by diskodev
// game/screens/phys_test2.c
// Shape collision sandbox: circles, AABBs, and rotating OBBs. SAT + circle-vs-AABB resolution.
#include "phys_test2.h"
#include <engine/engine_core.h>
#include <math.h>

extern PlaydateAPI*  pd;
extern InputManager  inputManager;
extern ScreenManager screenManager;

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
#define PT2_MAX_SHAPES     12
#define PT2_GRAVITY        400.0f
#define PT2_RESTITUTION    0.55f    // bounciness on collision
#define PT2_FRICTION       0.78f    // velocity damping on ground contact
#define PT2_AIR_DAMP       0.995f
#define PT2_FLOOR_Y        220.0f
#define PT2_WALL_L           4.0f
#define PT2_WALL_R         396.0f
#define PT2_CURSOR_SPEED   160.0f
#define PT2_LAUNCH_SCALE    5.0f   // cursor-drag velocity multiplier

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
typedef enum { SHAPE_CIRCLE, SHAPE_AABB, SHAPE_OBB } ShapeKind;

typedef struct {
    ShapeKind kind;
    f32 x, y;           // centre position
    f32 vx, vy;
    f32 angle;          // radians (OBB only, ignored for AABB/circle)
    f32 angularVel;     // rad/s
    f32 hw, hh;         // half-width, half-height (AABB/OBB); radius stored in hw (circle)
    bool sleeping;
} Shape;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static Shape  s_shapes[PT2_MAX_SHAPES];
static i32    s_count;
static i32    s_selected;  // index of shape under cursor, -1 = none

static f32    s_curX, s_curY;
static f32    s_dragStartX, s_dragStartY;
static bool   s_dragging;

// ---------------------------------------------------------------------------
// Math helpers
// ---------------------------------------------------------------------------
static f32 Dot2(f32 ax, f32 ay, f32 bx, f32 by) { return ax * bx + ay * by; }
static f32 Len2(f32 x, f32 y) { return sqrtf(x * x + y * y); }

// Rotate vector (x,y) by angle a.
static void Rotate2(f32 x, f32 y, f32 a, f32* ox, f32* oy) {
    f32 c = cosf(a), s = sinf(a);
    *ox = c * x - s * y;
    *oy = s * x + c * y;
}

// Project OBB onto axis (nx,ny). Returns half-extent.
static f32 OBBProject(Shape* s, f32 nx, f32 ny) {
    f32 ax, ay, bx, by;
    Rotate2(s->hw, 0, s->angle, &ax, &ay);
    Rotate2(0, s->hh, s->angle, &bx, &by);
    return fabsf(Dot2(ax, ay, nx, ny)) + fabsf(Dot2(bx, by, nx, ny));
}

// ---------------------------------------------------------------------------
// Collision detection – returns true + MTV (minimum translation vector)
// ---------------------------------------------------------------------------

// Circle vs Circle.
static bool CircleCircle(Shape* a, Shape* b, f32* nx, f32* ny, f32* depth) {
    f32 dx = b->x - a->x, dy = b->y - a->y;
    f32 dist = Len2(dx, dy);
    f32 radSum = a->hw + b->hw;
    if (dist >= radSum || dist < 0.0001f) return false;
    *nx = dx / dist; *ny = dy / dist;
    *depth = radSum - dist;
    return true;
}

// AABB vs AABB.
static bool AABBAABB(Shape* a, Shape* b, f32* nx, f32* ny, f32* depth) {
    f32 dx = b->x - a->x, dy = b->y - a->y;
    f32 ox = (a->hw + b->hw) - fabsf(dx);
    f32 oy = (a->hh + b->hh) - fabsf(dy);
    if (ox <= 0 || oy <= 0) return false;
    if (ox < oy) { *nx = dx < 0 ? -1.0f : 1.0f; *ny = 0.0f; *depth = ox; }
    else         { *nx = 0.0f; *ny = dy < 0 ? -1.0f : 1.0f; *depth = oy; }
    return true;
}

// Circle vs AABB.
static bool CircleAABB(Shape* circ, Shape* box, f32* nx, f32* ny, f32* depth) {
    f32 cx = circ->x - box->x, cy = circ->y - box->y;
    f32 clampX = cx < -box->hw ? -box->hw : (cx > box->hw ? box->hw : cx);
    f32 clampY = cy < -box->hh ? -box->hh : (cy > box->hh ? box->hh : cy);
    f32 dx = cx - clampX, dy = cy - clampY;
    f32 dist = Len2(dx, dy);
    if (dist >= circ->hw || dist < 0.0001f) return false;
    *nx = dx / dist; *ny = dy / dist;
    *depth = circ->hw - dist;
    return true;
}

// Generic SAT for OBB pairs (also handles AABB as OBB with angle=0).
// Tests 4 axes (2 per box). Returns false if separating axis found.
static bool OBBvsOBB(Shape* a, Shape* b, f32* nx, f32* ny, f32* depth) {
    f32 axes[4][2];
    Rotate2(1, 0, a->angle, &axes[0][0], &axes[0][1]);
    Rotate2(0, 1, a->angle, &axes[1][0], &axes[1][1]);
    Rotate2(1, 0, b->angle, &axes[2][0], &axes[2][1]);
    Rotate2(0, 1, b->angle, &axes[3][0], &axes[3][1]);

    f32 minDepth = 1e9f;
    f32 mnx = 0, mny = 0;
    f32 dCx = b->x - a->x, dCy = b->y - a->y;

    for (i32 i = 0; i < 4; i++) {
        f32 axNx = axes[i][0], axNy = axes[i][1];
        f32 projA = OBBProject(a, axNx, axNy);
        f32 projB = OBBProject(b, axNx, axNy);
        f32 dist  = fabsf(Dot2(dCx, dCy, axNx, axNy));
        f32 ov    = projA + projB - dist;
        if (ov <= 0) return false;
        if (ov < minDepth) {
            minDepth = ov;
            mnx = axNx; mny = axNy;
            // Ensure normal points from a to b.
            if (Dot2(dCx, dCy, mnx, mny) < 0) { mnx = -mnx; mny = -mny; }
        }
    }
    *nx = mnx; *ny = mny; *depth = minDepth;
    return true;
}

// Unified collision dispatch.
static bool TestShapes(Shape* a, Shape* b, f32* nx, f32* ny, f32* depth) {
    if (a->kind == SHAPE_CIRCLE && b->kind == SHAPE_CIRCLE)
        return CircleCircle(a, b, nx, ny, depth);

    if (a->kind == SHAPE_CIRCLE && b->kind == SHAPE_AABB)
        return CircleAABB(a, b, nx, ny, depth);
    if (a->kind == SHAPE_AABB && b->kind == SHAPE_CIRCLE) {
        bool r = CircleAABB(b, a, nx, ny, depth);
        if (r) { *nx = -*nx; *ny = -*ny; }
        return r;
    }

    // OBB vs OBB covers AABB vs AABB and any mix with OBB.
    return OBBvsOBB(a, b, nx, ny, depth);
}

// ---------------------------------------------------------------------------
// Impulse resolution
// ---------------------------------------------------------------------------
static void ResolveCollision(Shape* a, Shape* b, f32 nx, f32 ny, f32 depth) {
    // Positional correction – push apart equally.
    f32 half = depth * 0.5f;
    a->x -= nx * half; a->y -= ny * half;
    b->x += nx * half; b->y += ny * half;

    // Relative velocity along normal.
    f32 rvx = b->vx - a->vx, rvy = b->vy - a->vy;
    f32 velAlongN = Dot2(rvx, rvy, nx, ny);
    if (velAlongN > 0) return;  // already separating

    f32 j = -(1.0f + PT2_RESTITUTION) * velAlongN * 0.5f;
    a->vx -= j * nx; a->vy -= j * ny;
    b->vx += j * nx; b->vy += j * ny;

    // Light angular kick for OBBs so they spin on hit.
    if (a->kind == SHAPE_OBB) a->angularVel += j * 0.04f * (ny * 0.5f - nx * 0.5f);
    if (b->kind == SHAPE_OBB) b->angularVel -= j * 0.04f * (ny * 0.5f - nx * 0.5f);
}

// ---------------------------------------------------------------------------
// Spawn helpers
// ---------------------------------------------------------------------------
static void SpawnCircle(f32 x, f32 y, f32 r) {
    if (s_count >= PT2_MAX_SHAPES) return;
    Shape* s = &s_shapes[s_count++];
    s->kind = SHAPE_CIRCLE;
    s->x = x; s->y = y;
    s->vx = 0; s->vy = 0;
    s->hw = r; s->hh = r;
    s->angle = 0; s->angularVel = 0;
    s->sleeping = false;
}

static void SpawnAABB(f32 x, f32 y, f32 hw, f32 hh) {
    if (s_count >= PT2_MAX_SHAPES) return;
    Shape* s = &s_shapes[s_count++];
    s->kind = SHAPE_AABB;
    s->x = x; s->y = y;
    s->vx = 0; s->vy = 0;
    s->hw = hw; s->hh = hh;
    s->angle = 0; s->angularVel = 0;
    s->sleeping = false;
}

static void SpawnOBB(f32 x, f32 y, f32 hw, f32 hh, f32 angle) {
    if (s_count >= PT2_MAX_SHAPES) return;
    Shape* s = &s_shapes[s_count++];
    s->kind = SHAPE_OBB;
    s->x = x; s->y = y;
    s->vx = 0; s->vy = 0;
    s->hw = hw; s->hh = hh;
    s->angle = angle; s->angularVel = 0;
    s->sleeping = false;
}

// ---------------------------------------------------------------------------
// Screen interface
// ---------------------------------------------------------------------------
void PhysTest2Screen_Init(void) {
    s_count    = 0;
    s_selected = -1;
    s_dragging = false;
    s_curX     = 200.0f;
    s_curY     = 120.0f;

    // Seed with a mix of shape types.
    SpawnCircle(60,  80, 14);
    SpawnCircle(120, 60, 10);
    SpawnAABB  (200, 90, 18, 12);
    SpawnAABB  (300, 70, 12, 16);
    SpawnOBB   (160, 100, 20, 10, 0.4f);
    SpawnOBB   (260, 80,  14, 14, 0.9f);
}

void PhysTest2Screen_Update(f32 deltaTime) {
    // --- Cursor movement ---
    f32 moveX = 0, moveY = 0;
    if (Input_IsDown(&inputManager, INPUT_LEFT))  moveX -= 1.0f;
    if (Input_IsDown(&inputManager, INPUT_RIGHT)) moveX += 1.0f;
    if (Input_IsDown(&inputManager, INPUT_UP))    moveY -= 1.0f;
    if (Input_IsDown(&inputManager, INPUT_DOWN))  moveY += 1.0f;
    s_curX = Clamp(s_curX + moveX * PT2_CURSOR_SPEED * deltaTime, PT2_WALL_L, PT2_WALL_R);
    s_curY = Clamp(s_curY + moveY * PT2_CURSOR_SPEED * deltaTime, 4.0f, PT2_FLOOR_Y);

    // --- Spawn ---
    if (Input_IsPressed(&inputManager, INPUT_A)) {
        // Cycle: circle -> AABB -> OBB -> circle
        static i32 spawnType = 0;
        switch (spawnType % 3) {
            case 0: SpawnCircle(s_curX, s_curY, 10 + (spawnType / 3) % 3 * 4); break;
            case 1: SpawnAABB  (s_curX, s_curY, 14, 10); break;
            case 2: SpawnOBB   (s_curX, s_curY, 18, 10, 0.3f); break;
        }
        spawnType++;
    }

    // --- Delete selected ---
    if (Input_IsPressed(&inputManager, INPUT_B) && s_selected >= 0) {
        s_shapes[s_selected] = s_shapes[--s_count];
        s_selected = -1;
    }

    // --- Crank rotates nearest OBB ---
    f32 crankDelta = Input_GetCrankChange(&inputManager);
    if (fabsf(crankDelta) > 0.5f && s_selected >= 0 && s_shapes[s_selected].kind == SHAPE_OBB)
        s_shapes[s_selected].angle += crankDelta * (3.14159f / 180.0f);

    // --- Find shape nearest cursor (pick) ---
    s_selected = -1;
    f32 bestDist = 30.0f * 30.0f;
    for (i32 i = 0; i < s_count; i++) {
        f32 dx = s_shapes[i].x - s_curX, dy = s_shapes[i].y - s_curY;
        f32 d2 = dx*dx + dy*dy;
        if (d2 < bestDist) { bestDist = d2; s_selected = i; }
    }

    // --- Physics step ---
    for (i32 i = 0; i < s_count; i++) {
        Shape* s = &s_shapes[i];
        // Gravity + air damping.
        s->vy += PT2_GRAVITY * deltaTime;
        s->vx *= PT2_AIR_DAMP;
        s->vy *= PT2_AIR_DAMP;
        // Integrate.
        s->x += s->vx * deltaTime;
        s->y += s->vy * deltaTime;
        // Spin decay.
        s->angularVel *= 0.97f;
        s->angle      += s->angularVel * deltaTime;

        // --- Floor / wall bounds ---
        f32 rad = (s->kind == SHAPE_CIRCLE) ? s->hw
                : (s->kind == SHAPE_AABB)   ? s->hh
                : (fabsf(cosf(s->angle)) * s->hh + fabsf(sinf(s->angle)) * s->hw); // OBB AABB extent

        if (s->y + rad > PT2_FLOOR_Y) {
            s->y   = PT2_FLOOR_Y - rad;
            s->vy *= -PT2_RESTITUTION;
            s->vx *=  PT2_FRICTION;
            s->angularVel *= PT2_FRICTION;
        }
        f32 wrad = (s->kind == SHAPE_CIRCLE) ? s->hw
                 : (s->kind == SHAPE_AABB)   ? s->hw
                 : (fabsf(cosf(s->angle)) * s->hw + fabsf(sinf(s->angle)) * s->hh);

        if (s->x - wrad < PT2_WALL_L) {
            s->x   = PT2_WALL_L + wrad;
            s->vx *= -PT2_RESTITUTION;
        }
        if (s->x + wrad > PT2_WALL_R) {
            s->x   = PT2_WALL_R - wrad;
            s->vx *= -PT2_RESTITUTION;
        }
    }

    // --- Shape vs shape collisions ---
    for (i32 i = 0; i < s_count; i++) {
        for (i32 j = i + 1; j < s_count; j++) {
            f32 nx, ny, depth;
            if (TestShapes(&s_shapes[i], &s_shapes[j], &nx, &ny, &depth))
                ResolveCollision(&s_shapes[i], &s_shapes[j], nx, ny, depth);
        }
    }
}

// ---------------------------------------------------------------------------
// Drawing helpers (using raw Playdate graphics API)
// ---------------------------------------------------------------------------

// Draw a filled circle outline using Playdate's drawEllipse.
static void DrawCircleOutline(i32 cx, i32 cy, i32 r, LCDColor col) {
    pd->graphics->drawEllipse(cx - r, cy - r, r * 2, r * 2, 1, 0, 360, col);
}

// Draw an AABB outline.
static void DrawAABBOutline(i32 cx, i32 cy, i32 hw, i32 hh, LCDColor col) {
    pd->graphics->drawRect(cx - hw, cy - hh, hw * 2, hh * 2, col);
}

// Draw OBB by computing four corner points and drawing line segments.
static void DrawOBBOutline(f32 cx, f32 cy, f32 hw, f32 hh, f32 angle, LCDColor col) {
    f32 corners[4][2] = {{ hw, hh}, {-hw, hh}, {-hw, -hh}, { hw, -hh}};
    f32 px[4], py[4];
    for (i32 i = 0; i < 4; i++) {
        f32 rx, ry;
        Rotate2(corners[i][0], corners[i][1], angle, &rx, &ry);
        px[i] = cx + rx;
        py[i] = cy + ry;
    }
    for (i32 i = 0; i < 4; i++) {
        i32 j = (i + 1) % 4;
        pd->graphics->drawLine((i32)px[i], (i32)py[i], (i32)px[j], (i32)py[j], 1, col);
    }
    // Draw a tick showing angle so rotation is visible.
    f32 tx, ty;
    Rotate2(hw, 0, angle, &tx, &ty);
    pd->graphics->drawLine((i32)cx, (i32)cy, (i32)(cx + tx), (i32)(cy + ty), 1, col);
}

void PhysTest2Screen_Draw(void) {
    pd->graphics->clear(kColorWhite);

    // Floor line.
    pd->graphics->drawLine((i32)PT2_WALL_L, (i32)PT2_FLOOR_Y,
                           (i32)PT2_WALL_R, (i32)PT2_FLOOR_Y, 2, kColorBlack);
    // Left / right walls.
    pd->graphics->drawLine((i32)PT2_WALL_L, 0, (i32)PT2_WALL_L, (i32)PT2_FLOOR_Y, 2, kColorBlack);
    pd->graphics->drawLine((i32)PT2_WALL_R, 0, (i32)PT2_WALL_R, (i32)PT2_FLOOR_Y, 2, kColorBlack);

    // Shapes.
    for (i32 i = 0; i < s_count; i++) {
        Shape* s   = &s_shapes[i];
        LCDColor col = (i == s_selected) ? kColorXOR : kColorBlack;

        switch (s->kind) {
            case SHAPE_CIRCLE:
                DrawCircleOutline((i32)s->x, (i32)s->y, (i32)s->hw, col);
                // Tick to show rotation feel (visual only).
                pd->graphics->drawLine(
                    (i32)s->x, (i32)s->y,
                    (i32)(s->x + cosf(s->angle) * s->hw),
                    (i32)(s->y + sinf(s->angle) * s->hw),
                    1, col);
                break;
            case SHAPE_AABB:
                DrawAABBOutline((i32)s->x, (i32)s->y, (i32)s->hw, (i32)s->hh, col);
                break;
            case SHAPE_OBB:
                DrawOBBOutline(s->x, s->y, s->hw, s->hh, s->angle, col);
                break;
        }

        // Label.
        char lbl[4];
        const char* tag = (s->kind == SHAPE_CIRCLE) ? "C" :
                          (s->kind == SHAPE_AABB)   ? "A" : "O";
        pd->graphics->drawText(tag, 1, kASCIIEncoding,
                               (i32)s->x - 3, (i32)s->y - 4);
    }

    // Cursor crosshair.
    i32 cx = (i32)s_curX, cy = (i32)s_curY;
    pd->graphics->drawLine(cx - 5, cy, cx + 5, cy, 1, kColorBlack);
    pd->graphics->drawLine(cx, cy - 5, cx, cy + 5, 1, kColorBlack);

    // HUD.
    char hud[64];
    snprintf(hud, sizeof(hud), "A:spawn  B:del  crank:rot  shapes:%d", s_count);
    pd->graphics->drawText(hud, strlen(hud), kASCIIEncoding, 4, 224);
}

void PhysTest2Screen_Unload(void) {
    s_count    = 0;
    s_selected = -1;
}
