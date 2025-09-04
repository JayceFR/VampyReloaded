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

typedef enum{
  TOWN, 
  VILLAGE,
  FOREST
} BIOME; 

struct BIOME_DATA{
  Texture2D **texs; 
  int *size_of_texs; 
};
typedef struct BIOME_DATA *BIOME_DATA;

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

typedef enum {
    STONE_BL = 0,
    STONE_BR = 1,
    STONE_BOTTOM = 2,
    STONE_LEFT = 3,
    STONE_MIDDLE = 4,
    STONE_RIGHT = 5,
    STONE_TL = 6,
    STONE_TR = 7,
    STONE_TOP = 8,
    STONE_BL_1 = 9,
    STONE_BR_1 = 10,
    STONE_BOTTOM_1 = 11
} StoneVariant;

// Tile Types 
// [bottom_left, bottom_right, bottom, left, right, top_left, top_right]

struct rect{
  TILES tile; 
  Rectangle rectange; 
  pathNode node; 
  // NOTE : Make it an union if you want to have different types of grass as well.
  int tileType; // A number which holds the index.

  int offGridType; // -1 is None
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

struct offgrid{
  int width;
  int height; 
  Texture2D texture; 
};
typedef struct offgrid *offgrid;

struct offgridTile{
  int x; 
  int y; 
  Texture2D texture; 
};
typedef struct offgridTile *offgridTile;

extern mapData mapCreate(offgrid *properties, int size_of_properties, hash offgridTiles, BIOME_DATA biome_data, Texture2D pathDirt);
// extern void mapDraw(Camera2D camera);
extern void MapDrawCached(Camera2D camera);
void MapEnsureCache(hash map, Camera2D camera, Texture2D *tileMap, Texture2D *stoneMap, Texture2D *dirtMap, Texture2D *offgridMap);
extern dynarray rectsAround(hash map, Vector2 player_pos);
extern void mapFree(hash map);
extern rect mapGetRecAt(hash map, int x, int y);
// extern void generateRandomWalkerMap(TILES map[HEIGHT][WIDTH]);
extern void printMap(TILES map[HEIGHT][WIDTH]);
extern Door getPlayerRoomDoor(dynarray doors, Vector2 playerPos);

#endif