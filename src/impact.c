#include "impact.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

// ------------------ Screenshake ------------------
static float shakeTime = 0.0f;
static float shakeStrength = 0.0f;

void Impact_StartShake(float duration, float strength) {
    shakeTime = duration;
    shakeStrength = strength;
}

void Impact_UpdateShake(Camera2D *camera, float delta) {
    if (shakeTime > 0.0f) {
        shakeTime -= delta;
        float s = shakeStrength * (shakeTime); // decay over time
        camera->target.x += ((float)GetRandomValue(-1000, 1000) / 1000.0f) * s;
        camera->target.y += ((float)GetRandomValue(-1000, 1000) / 1000.0f) * s;
    }
}

// ------------------ Enemy Hit Flash ------------------
void Impact_HitFlashTrigger(ImpactHitFlash *flash, float duration) {
    flash->flashTimer = duration;
}

Color Impact_HitFlashColor(ImpactHitFlash *flash, Color baseColor, float delta) {
    if (flash->flashTimer > 0.0f) {
        flash->flashTimer -= delta;
        return WHITE; // flash white
    }
    return baseColor;
}

// ------------------ Particle Burst ------------------
#define MAX_PARTICLES 128
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float lifetime;
    Color color;
    bool active;
    int radius; 
} Particle;

static Particle particles[MAX_PARTICLES];

void Impact_SpawnBurst(Vector2 pos, Color c, int count) {
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].pos = pos;
            float angle = ((float)GetRandomValue(0, 360)) * DEG2RAD;
            float speed = (float)GetRandomValue(30, 100) / 10.0f;
            particles[i].vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed};
            particles[i].lifetime = 0.5f + (float)GetRandomValue(0, 200) / 1000.0f;
            particles[i].color = c;
            particles[i].radius = 2 + GetRandomValue(0,3);
            count--;
        }
    }
}

void Impact_UpdateParticles(float delta) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].lifetime -= delta;
            if (particles[i].lifetime <= 0.0f) {
                particles[i].active = false;
                continue;
            }
            particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, delta * 60));
            particles[i].color.a = (unsigned char)(255 * (particles[i].lifetime)); // fade out
        }
    }
}

void Impact_DrawParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            DrawCircleV(particles[i].pos, particles[i].radius, particles[i].color);
        }
    }
}

void Impact_SpawnDirectedBurst(Vector2 pos, Vector2 dir, Color c, int count, float spreadDeg) {
    if (Vector2Length(dir) < 0.001f) dir = (Vector2){1, 0}; // safety
    dir = Vector2Normalize(dir);

    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].pos = pos;

            // base angle opposite the shot direction
            float baseAngle = atan2f(-dir.y, -dir.x);
            // add some random spread (in radians)
            float spread = ((float)GetRandomValue(-1000, 1000) / 1000.0f) * (spreadDeg * DEG2RAD);
            float angle = baseAngle + spread;

            // give them speed
            float speed = (float)GetRandomValue(100, 250) / 10.0f;
            particles[i].vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed};

            particles[i].lifetime = 0.15f + (float)GetRandomValue(0, 100) / 1000.0f; // short life
            particles[i].color = c;
            particles[i].radius = 2 + GetRandomValue(0,2);

            count--;
        }
    }
}

// ------------------ Ammo Shells ------------------
#define MAX_SHELLS 32

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float bouncePos; 
    float rotation;     // current angle
    float rotSpeed;     // spin speed
    float lifetime;
    bool active;
} Shell;

static Shell shells[MAX_SHELLS];

void Impact_SpawnShell(Vector2 pos, Vector2 ejectDir) {
    for (int i = 0; i < MAX_SHELLS; i++) {
        if (!shells[i].active) {
            shells[i].active = true;
            shells[i].pos = pos;
            shells[i].pos.x += 10;
            shells[i].pos.y += 20;

            // randomize velocity in eject direction
            float speed = (float)GetRandomValue(80, 160) / 10.0f;
            Vector2 jitter = (Vector2){ ((float)GetRandomValue(-30, 30)) / 100.0f,
                                        ((float)GetRandomValue(-30, 30)) / 100.0f };
            shells[i].vel = Vector2Scale(Vector2Normalize(Vector2Add(ejectDir, jitter)), speed);

            // random spin
            shells[i].rotation = (float)GetRandomValue(0, 360);
            shells[i].rotSpeed = (float)GetRandomValue(-200, 200);

            shells[i].lifetime = 1.5f; // longer-lived
            shells[i].bouncePos = shells[i].pos.y + 20;
            break;
        }
    }
}

void Impact_UpdateShells(float delta) {
    for (int i = 0; i < MAX_SHELLS; i++) {
        if (shells[i].active) {
            shells[i].lifetime -= delta;
            if (shells[i].lifetime <= 0.0f) {
                shells[i].active = false;
                continue;
            }

            // gravity
            shells[i].vel.y += 500.0f * delta;

            // update position
            shells[i].pos = Vector2Add(shells[i].pos, Vector2Scale(shells[i].vel, delta));

            // rotation
            shells[i].rotation += shells[i].rotSpeed * delta;

            // ground bounce at y=400 (example)
            if (shells[i].pos.y > shells[i].bouncePos) {
                shells[i].pos.y = shells[i].bouncePos;
                shells[i].vel.y *= -0.3f;   // bounce damping
                shells[i].vel.x *= 0.5f;    // slow down horizontally
                shells[i].rotSpeed *= 0.3f; // reduce spin
                if (fabsf(shells[i].vel.y) < 10.0f) {
                    shells[i].vel = (Vector2){0,0};
                    shells[i].rotSpeed = 0;
                }
            }
        }
    }
}

void Impact_DrawShells(void) {
    for (int i = 0; i < MAX_SHELLS; i++) {
        if (shells[i].active) {
            Rectangle rect = { shells[i].pos.x, shells[i].pos.y, 6, 2 };
            DrawRectanglePro(rect, (Vector2){3, 1}, shells[i].rotation, WHITE);
        }
    }
}
