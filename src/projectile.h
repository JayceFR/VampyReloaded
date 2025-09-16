#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "raylib.h"
#include "physics.h"
#include "dynarray.h"

typedef enum {
    PISTOL,
    SHOTGUN,
} GUN_TYPE;

struct projectile{
    entity e; 
    Vector2 dir; 
    float speed; 
    Vector2 startPos;
    GUN_TYPE gunType;
};
typedef struct projectile *projectile;

extern void projectileShoot(dynarray projectiles, Vector2 playerPos, Vector2 dir, float speed, GUN_TYPE gun_type);
extern bool projectileUpdate(projectile p, hash map);
extern void projectileDraw(projectile p);
extern void projectileFree(projectile p);


#endif