#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "hash.h"
#include "dynarray.h"

#define TILE_SIZE 16 

typedef enum{
  DIRT,
  STONE
} TILES; 

struct rect{
  TILES tile; 
  Rectangle rectange; 
};
typedef struct rect *rect;

extern hash mapCreate(void);
extern void mapDraw(hash map, Vector2 player_pos);
extern dynarray rectsAround(hash map, Vector2 player_pos);

#endif