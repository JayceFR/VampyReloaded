#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib.h"
#include "hash.h"


struct entity{
  Vector2 pos; 
  Rectangle rect; 
};
typedef struct entity *entity;

extern entity entityCreate(float startX, float startY, int width, int height);
extern bool update(entity e, hash map, Vector2 newPos);

#endif