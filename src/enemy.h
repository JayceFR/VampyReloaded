#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "map.h"
#include "physics.h"

// Enemy states
// Idle -> circling around the spawn point 
// Active -> becomes true if LOS becomes true 
// Path Find -> pathfind if active and player's LOS is false 

typedef enum {
    IDLE, 
    ACTIVE,
} State; 

struct Enemy{
    dynarray path;
    int targetTileX;
    int targetTileY;
    int currentStep;
    entity e; 
    State state;
};
typedef struct Enemy *Enemy;  

extern Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map);
extern Enemy enemyCreate(int startX, int startY, int width, int height);

#endif