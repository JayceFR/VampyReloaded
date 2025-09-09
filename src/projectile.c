#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "raylib.h"
#include "raymath.h"
#include "physics.h"
#include "projectile.h"

void projectileShoot(dynarray projectiles, Vector2 playerPos, Vector2 dir, float speed){
    projectile p = malloc(sizeof(struct projectile));
    // p->rect = (Rectangle) {playerPos.x, playerPos.y, 5, 5};
    p->e = entityCreate(playerPos.x, playerPos.y, 10, 10);
    p->dir = Vector2Normalize(dir);
    p->speed = speed;
    p->dir = Vector2Scale(p->dir, speed);
    add_dynarray(projectiles, p);
}   

bool projectileUpdate(projectile p, hash map){
    return update(p->e, map, p->dir);
}

void projectileDraw(projectile p){
    DrawRectangleRec(p->e->rect, WHITE);
}

void projectileFree(projectile p){
    free(p);
}