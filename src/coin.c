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
                coins[j].vel = (Vector2){ (rand()%100 - 50) * 0.05f, (rand()%100 - 50) * 0.05f }; // little scatter
                coins[j].active = true;
                break;
            }
        }
    }
}

void updateCoins(Coin *coins, entity killer, float dt) {
    for (int i = 0; i < MAX_COINS; i++) {
        if (!coins[i].active) continue;

        // Direction towards killer
        Vector2 dir = Vector2Subtract(killer->pos, coins[i].pos);
        float dist = Vector2Length(dir);

        if (dist < 10.0f) {
            // Collected
            coins[i].active = false;
            continue;
        }

        // Normalize direction
        Vector2 dirNorm = Vector2Normalize(dir);

        float pullStrength = Clamp(400.0f / (dist + 1.0f), 4.0f, 40.0f);

        Vector2 attraction = Vector2Scale(dirNorm, pullStrength * dt);

        coins[i].vel = Vector2Lerp(coins[i].vel, attraction, 0.25f);

        // Update position
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


