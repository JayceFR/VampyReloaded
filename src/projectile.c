#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "raylib.h"
#include "raymath.h"
#include "projectile.h"

void projectileShoot(dynarray projectiles, Vector2 playerPos, Vector2 dir){
    projectile p = malloc(sizeof(struct projectile));
    p->rect = (Rectangle) {playerPos.x, playerPos.y, 5, 5};
    p->dir = Vector2Normalize(dir); 
    add_dynarray(projectiles, p);
}   

void projectileUpdate(projectile p){
    p->rect.x += p->dir.x * 10; 
    p->rect.y += p->dir.y * 10;
}

void projectileDraw(projectile p){
    DrawRectangleRec(p->rect, WHITE);
}

void projectileFree(projectile p){
    free(p);
}