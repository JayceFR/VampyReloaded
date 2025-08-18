#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "map.h"
#include "physics.h"

struct Enemy{
    dynarray path;
    int targetTileX;
    int targetTileY;
    int currentStep;
    entity e; 
};
typedef struct Enemy *Enemy;  

extern Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map);
extern Enemy enemyCreate(int startX, int startY, int width, int height);

#endif