#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "hash.h"

extern hash mapCreate(Vector2 player_pos);
extern void mapDraw(hash map, Vector2 player_pos);

#endif