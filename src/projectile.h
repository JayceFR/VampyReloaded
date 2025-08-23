#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "raylib.h"
#include "physics.h"
#include "dynarray.h"

struct projectile{
    entity e; 
    Vector2 dir; 
    float speed; 
};
typedef struct projectile *projectile;

extern void projectileShoot(dynarray projectiles, Vector2 playerPos, Vector2 dir, float speed);
extern bool projectileUpdate(projectile p, hash map);
extern void projectileDraw(projectile p);
extern void projectileFree(projectile p);


#endif