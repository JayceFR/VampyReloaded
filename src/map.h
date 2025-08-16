#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "hash.h"

extern hash mapCreate(void);
extern void mapDraw(hash map, Vector2 player_pos);

#endif