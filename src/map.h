#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "hash.h"
#include "dynarray.h"


#define TILE_SIZE 16 
#define CHUNK_SIZE 64   // one puzzle map
#define ROOM_SIZE (TILE_SIZE * CHUNK_SIZE)

#define WIDTH  64
#define HEIGHT 64

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

struct Door{
  Vector2 pos; 
  int ax, ay;
  int bx, by;
  int aw, bw; 
  int ah, bh; 
  bool locked;
};
typedef struct Door *Door;

typedef struct{
  hash map;
  hash enemies; 
} mapData;

extern mapData mapCreate(void);
extern void mapDraw(hash map, Vector2 player_pos);
extern dynarray rectsAround(hash map, Vector2 player_pos);
extern void mapFree(hash map);
extern rect mapGetRecAt(hash map, int x, int y);
// extern void generateRandomWalkerMap(TILES map[HEIGHT][WIDTH]);
extern void printMap(TILES map[HEIGHT][WIDTH]);
extern Door getPlayerRoomDoor(dynarray doors, Vector2 playerPos);

#endif