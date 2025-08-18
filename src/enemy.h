#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "map.h"
#include "physics.h"

typedef struct{
    dynarray path;
    int targetTileX;
    int targetTileY;
    int currentStep;
    entity e; 
} Enemy; 

extern Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map);

#endif