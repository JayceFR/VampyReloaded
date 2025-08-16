#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"

#include "hash.h"
#include "dynarray.h"
#include "map.h"

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
  for (int x = 0; x <= 36; x++){
    for (int y = 0; y <= 36; y++){
      TILES* tile = malloc(sizeof(TILES));
      sprintf(buffer, "%d:%d", x * TILE_SIZE, y * TILE_SIZE);
      if (x == 0 || y == 0 || x == 36 || y == 36){
        *tile = STONE;
      }
      else{
        *tile = DIRT;
      }
      hashSet(map, buffer, tile);
    }
  }
  hashDump(stdout, map);
  return map; 
}

void rectFree(DA_ELEMENT el){
  rect r = (rect) el; 
  free(r);
}

dynarray rectsAround(hash map, Vector2 player_pos){
  int gx = ((int) player_pos.x) / TILE_SIZE;
  int gy = ((int) player_pos.y) / TILE_SIZE;
  char buffer[22];
  dynarray arr = create_dynarray(&rectFree, NULL); 
  for (int x = gx - 3; x <= gx + 3; x++){
    for (int y = gy - 3; y <= gy + 3; y++){
      sprintf(buffer, "%d:%d", x * TILE_SIZE, y * TILE_SIZE);
      TILES* tile; 
      if ((tile = hashFind(map, buffer)) != NULL){
        if (*tile == STONE){
          rect r = malloc(sizeof(struct rect));
          r->tile = *tile; 
          r->rectange = (Rectangle) {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
          add_dynarray(arr, r);
        }
      }
    }
  }
  return arr; 
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
        if (*tile == DIRT){
          DrawRectangle(worldX, worldY, TILE_SIZE, TILE_SIZE, YELLOW);
        }
        if (*tile == STONE){
          DrawRectangle(worldX, worldY, TILE_SIZE, TILE_SIZE, GRAY);
        }
      }
    }
  }
}

void mapFree(hash map){
  hashFree(map);
}
