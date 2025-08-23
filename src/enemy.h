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
    Vector2 idleTarget; 
    float idleTimer; 
    bool movingIdle;
    float angle; 

    // enemy.h (add to struct Enemy)
    float repathCooldown;       // time left until we’re allowed to repath
    float repathInterval;       // base interval between path solves (sec)
    int   lastGoalTileX;        // last player tile we solved towards
    int   lastGoalTileY;
    float senseCooldown;        // throttle LOS/cone checks
    bool  playerVisible;        // cached result of PlayerInTorchCone
    Vector2 lastKnownPlayerPos; // where we last saw the player
    int   staggerSlot;          // 0..(N-1) to distribute work across frames

    int health;
    int maxHealth; 

};
typedef struct Enemy *Enemy;  

extern Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map);
extern Enemy enemyCreate(int startX, int startY, int width, int height);
extern void updateAngle(Enemy e, Vector2 vel);
extern void enemyDraw(Enemy e, hash map);

#endif