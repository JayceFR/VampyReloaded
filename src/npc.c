#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "raylib.h"
#include "physics.h"
#include "npc.h"
#include "hash.h"

NPC npcCreate(int x, int y, int width, int height){
    NPC npc = malloc(sizeof(struct NPC));
    npc->e = entityCreate(x, y, width, height);
    npc->type = NPC_TYPE_COW;
    npc->state = 0; // idle
    npc->currentFrame = GetRandomValue(0,3);
    npc->animTimer = 0.0f;

    // init new fields
    npc->vel = (Vector2){0.0f, 0.0f};
    npc->stateTimer = (GetRandomValue(50,300) / 100.0f); // 0.5 - 3.0s idle initially
    npc->moveTimer = 0.0f;
    npc->speed = (GetRandomValue(20,80) / 60.0f); // ~0.33 - 1.33 per-frame

    return npc; 
}

void npcUpdate(NPC npc, hash map){
    if (!npc) return;
    float dt = GetFrameTime();

    // Animation update (same for idle/wander)
    npc->animTimer += dt;
    if (npc->animTimer > 0.18f) {
        npc->animTimer = 0.0f;
        npc->currentFrame = (npc->currentFrame + 1) % 4;
    }

    // State machine: 0 = idle, 1 = wander
    if (npc->state == 0) {
        // idle: count down, then start wandering
        npc->stateTimer -= dt;
        if (npc->stateTimer <= 0.0f) {
            // enter wander
            npc->state = 1;
            npc->moveTimer = (GetRandomValue(30,150) / 60.0f); // 0.5 - 2.5s
            float ang = (GetRandomValue(0,359) * (PI / 180.0f));
            npc->vel.x = cosf(ang) * npc->speed;
            npc->vel.y = sinf(ang) * npc->speed;
            // small jitter: if vel nearly zero, re-randomize
            if (fabsf(npc->vel.x) < 0.001f && fabsf(npc->vel.y) < 0.001f) {
                npc->vel.x = npc->speed;
                npc->vel.y = 0;
            }
        }
    } else {
        // wander: move and count down; if collision, pick new direction
        bool collided = update(npc->e, map, npc->vel);
        npc->moveTimer -= dt;
        if (collided) {
            // pick a new random direction and slightly shorten remaining time
            float ang = (GetRandomValue(0,359) * (PI / 180.0f));
            npc->vel.x = cosf(ang) * npc->speed;
            npc->vel.y = sinf(ang) * npc->speed;
            npc->moveTimer = fmaxf(0.1f, npc->moveTimer - 0.05f);
        }
        // if wander time finished, snap to idle
        if (npc->moveTimer <= 0.0f) {
            npc->state = 0;
            npc->stateTimer = (GetRandomValue(50,300) / 100.0f); // 0.5 - 3.0s
            npc->vel = (Vector2){0.0f, 0.0f};
        }
    }
}
