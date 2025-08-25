#ifndef IMPACT_H
#define IMPACT_H

#include "raylib.h"

// ---- Screenshake ----
void Impact_StartShake(float duration, float strength);
void Impact_UpdateShake(Camera2D *camera, float delta);

// ---- Enemy Hit Flash ----
typedef struct {
    float flashTimer;
} ImpactHitFlash;

void Impact_HitFlashTrigger(ImpactHitFlash *flash, float duration);
Color Impact_HitFlashColor(ImpactHitFlash *flash, Color baseColor, float delta);

// ---- Particle Burst ----
void Impact_SpawnBurst(Vector2 pos, Color c, int count);
void Impact_UpdateParticles(float delta);
void Impact_DrawParticles(void);

#endif
