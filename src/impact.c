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
            DrawCircleV(particles[i].pos, 3, particles[i].color);
        }
    }
}
