#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "raylib.h"
#include "dynarray.h"

struct projectile{
    Rectangle rect; 
    Vector2 dir; 
};
typedef struct projectile *projectile;

extern void projectileShoot(dynarray projectiles, Vector2 playerPos, Vector2 dir);
extern void projectileUpdate(projectile p);
extern void projectileDraw(projectile p);
extern void projectileFree(projectile p);


#endif