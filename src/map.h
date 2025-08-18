#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "hash.h"
#include "dynarray.h"

#define TILE_SIZE 16 

typedef enum{
  DIRT,
  STONE,
  AIR
} TILES; 

struct pathNode;
typedef struct pathNode *pathNode;

struct pathNode{
  int x; 
  int y; 

  int gCost;
  int hCost; 
  int fCost;
  pathNode prev; 

  bool isWalkable;
};

struct rect{
  TILES tile; 
  Rectangle rectange; 
  pathNode node; 
};
typedef struct rect *rect;

extern hash mapCreate(void);
extern void mapDraw(hash map, Vector2 player_pos);
extern dynarray rectsAround(hash map, Vector2 player_pos);
extern void mapFree(hash map);
extern rect mapGetRecAt(hash map, int x, int y);

#endif