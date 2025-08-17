#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib.h"
#include "hash.h"


struct entity{
  Vector2 pos; 
  Rectangle rect; 
};
typedef struct entity *entity;

extern entity entityCreate(Vector2 startPos, Rectangle rect);
extern void update(entity e, hash map, Vector2 newPos);

#endif