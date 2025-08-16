#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"

#include "hash.h"

typedef enum{
  DIRT,
  STONE
} TILES; 

#define TILE_SIZE 16 

void tilesFree(hashvalue val){
  TILES *tile = (TILES *) val; 
  free(tile);
}

void tilesPrint(FILE *out, hashkey key, hashvalue val){
  char *k = (char *)key;
  TILES *tile = (TILES *)val;
  printf("key : %s, tile : %d\n", k, *tile);
}

hash mapCreate(){
  hash map = hashCreate(&tilesPrint, &tilesFree, NULL);
  // Init the map for 3 grids around the player first 
  char buffer[22];
  for (int x = 0; x <= 16; x++){
    for (int y = 0; y <= 16; y++){
      sprintf(buffer, "%d:%d", x * TILE_SIZE, y * TILE_SIZE);
      TILES* tile = malloc(sizeof(TILES));
      *tile = DIRT;
      hashSet(map, buffer, tile);
    }
  }
  hashDump(stdout, map);
  return map; 
}

void mapDraw(hash map, Vector2 player_pos){
  int gx = ((int) player_pos.x) / TILE_SIZE;
  int gy = ((int) player_pos.y) / TILE_SIZE;
  // Init the map for 3 grids around the player first 
  char buffer[22];
  for (int x = gx - 32; x <= gx + 32; x++){
    for (int y = gy - 32; y <= gy + 32; y++){
      sprintf(buffer, "%d:%d", x * TILE_SIZE, y * TILE_SIZE);
      TILES* tile; 
      if ((tile = hashFind(map, buffer)) != NULL){
        int worldX = (x * TILE_SIZE);
        int worldY = (y * TILE_SIZE);
        DrawRectangle(worldX, worldY, TILE_SIZE, TILE_SIZE, YELLOW);
      }
    }
  }
}

void mapFree(hash map){
  hashFree(map);
}
