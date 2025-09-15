#ifndef COIN_H
#define COIN_H

#include "raylib.h"

typedef struct Coin {
    Vector2 pos;
    Vector2 vel;
    bool active;
    float attractDelay; // time before attraction starts
} Coin;


#define MAX_COINS 50

extern void spawnCoins(Coin *coins, Vector2 deathPos, int numCoins);
extern void updateCoins(Coin *coins, entity killer, float dt, int *currency);
extern void drawCoins(Coin *coins);
extern Coin *createCoins();

#endif