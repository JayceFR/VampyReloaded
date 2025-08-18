#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "raylib.h"

#include "hash.h"
#include "dynarray.h"
#include "map.h"

void tilesFree(hashvalue val){
  rect r = (rect) val; 
  free(r);
}

void tilesPrint(FILE *out, hashkey key, hashvalue val){
  char *k = (char *)key;
  rect r = (rect)val; 
  printf("key : %s, tile : %d\n", k, r->tile);
}

rect mapGetRecAt(hash map, int x, int y){
  char buffer[22];
  sprintf(buffer, "%d:%d", x, y);
  return hashFind(map, buffer);
}

hash mapCreate(){
  hash map = hashCreate(&tilesPrint, &tilesFree, NULL);
  // Init the map for 3 grids around the player first 
  char buffer[22];
  for (int x = 0; x <= 36; x++){
    for (int y = 0; y <= 36; y++){
      rect r = malloc(sizeof(struct rect));
      r->rectange = (Rectangle) {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
      r->tile = AIR;
      // TILES* tile = malloc(sizeof(TILES));
      sprintf(buffer, "%d:%d", x, y );
      if (x == 0 || y == 0 || x == 36 || y == 36 || x == 1 || y == 1 || x == 35 || y == 35 || (x == 15 && y == 15)){
        r->tile = STONE;
      }
      else{
        r->tile = DIRT;
      }
      r->node = malloc(sizeof(struct pathNode));
      assert(r->node != NULL);
      r->node->hCost = 0;
      r->node->gCost = INT_MAX;
      r->node->fCost = INT_MAX;
      r->node->prev = NULL;
      r->node->x = x;
      r->node->y = y; 
      hashSet(map, buffer, r);
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
  for (int x = gx - 5; x <= gx + 5; x++){
    for (int y = gy - 5; y <= gy + 5; y++){
      sprintf(buffer, "%d:%d", x, y);
      rect rec; 
      if ((rec = hashFind(map, buffer)) != NULL){
        if (rec->tile == STONE){
          rect r = malloc(sizeof(struct rect));
          r->tile = rec->tile; 
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
      sprintf(buffer, "%d:%d", x, y);
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
