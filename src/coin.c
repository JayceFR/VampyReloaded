#include <stdio.h>

#include "raylib.h"
#include "raymath.h"
#include "physics.h"
#include "coin.h"


void spawnCoins(Coin *coins, Vector2 deathPos, int numCoins) {
    for (int i = 0; i < numCoins; i++) {
        for (int j = 0; j < MAX_COINS; j++) {
            if (!coins[j].active) {
                coins[j].pos = deathPos;

                // Burst out in random directions
                float angle = (rand() % 360) * DEG2RAD;
                float speed = 20.0f + (rand() % 30); // 20–50 px/sec
                coins[j].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };

                coins[j].active = true;
                coins[j].attractDelay = 0.25f; // 0.25s burst phase before attraction
                break;
            }
        }
    }
}

void updateCoins(Coin *coins, entity killer, float dt) {
    for (int i = 0; i < MAX_COINS; i++) {
        if (!coins[i].active) continue;

        // During burst phase: just apply velocity
        if (coins[i].attractDelay > 0.0f) {
            coins[i].pos = Vector2Add(coins[i].pos, Vector2Scale(coins[i].vel, dt));
            coins[i].vel = Vector2Scale(coins[i].vel, 0.95f); // slight slowdown
            coins[i].attractDelay -= dt;
            continue;
        }

        // --- Attraction phase ---
        Vector2 dir = Vector2Subtract(killer->pos, coins[i].pos);
        float dist = Vector2Length(dir);

        if (dist < 12.0f) {
            // Collected
            coins[i].active = false;
            // killer->coins += 1;
            continue;
        }

        Vector2 dirNorm = Vector2Normalize(dir);

        // Attraction strength grows when closer
        float pullStrength = Clamp(400.0f / (dist + 1.0f), 6.0f, 45.0f);

        Vector2 attraction = Vector2Scale(dirNorm, pullStrength * dt);

        // Magnet-like smoothness
        coins[i].vel = Vector2Lerp(coins[i].vel, attraction, 0.3f);

        coins[i].pos = Vector2Add(coins[i].pos, coins[i].vel);
    }
}

void drawCoins(Coin *coins) {
    for (int i = 0; i < MAX_COINS; i++) {
        if (coins[i].active) {
            DrawCircleV(coins[i].pos, 5, GOLD);
        }
    }
}
