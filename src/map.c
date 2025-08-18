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

void generateRandomWalkerMap(TILES map[HEIGHT][WIDTH]) {
    // Fill with walls first
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            map[y][x] = STONE;
        }
    }

    int x = WIDTH / 2;
    int y = HEIGHT / 2;

    int floorCount = 0;
    int targetFloorCount = (WIDTH * HEIGHT) / 2; // carve ~50% floor

    while (floorCount < targetFloorCount) {
        if (map[y][x] == STONE) {
            map[y][x] = DIRT;
            floorCount++;
        }

        int dir = rand() % 4;
        if (dir == 0 && x > 1) x--;               // left
        if (dir == 1 && x < WIDTH - 2) x++;       // right
        if (dir == 2 && y > 1) y--;               // up
        if (dir == 3 && y < HEIGHT - 2) y++;      // down
    }

    // Put a stone border so things are closed off
    for (int i = 0; i < WIDTH; i++) {
        map[0][i] = STONE;
        map[HEIGHT - 1][i] = STONE;
    }
    for (int i = 0; i < HEIGHT; i++) {
        map[i][0] = STONE;
        map[i][WIDTH - 1] = STONE;
    }
}

// --- Simple ASCII preview ---
void printMap(TILES map[HEIGHT][WIDTH]) {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
        if (map[y][x] == STONE) printf("#");
        else if (map[y][x] == DIRT) printf(".");
        else printf(" ");
    }
    printf("\n");
  }
}

hash mapCreate(){
  hash map = hashCreate(&tilesPrint, &tilesFree, NULL);

  TILES mappy[HEIGHT][WIDTH];
  srand(time(NULL));
  generateRandomWalkerMap(mappy);
  for (int y = 0; y < HEIGHT; y++){
    for (int x = 0; x < WIDTH; x++){
      rect r = malloc(sizeof(struct rect));
      r->rectange = (Rectangle){ x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
      r->tile = mappy[y][x];

      r->node = malloc(sizeof(struct pathNode));
      r->node->x = x;
      r->node->y = y;
      r->node->isWalkable = (mappy[y][x] == DIRT);
      r->node->gCost = INT_MAX;
      r->node->hCost = 0;
      r->node->fCost = 0;
      r->node->prev = NULL;

      char buffer[22];
      sprintf(buffer, "%d:%d", x, y);
      hashSet(map, buffer, r);
    }
  }

  // Init the map for 3 grids around the player first 
  // char buffer[22];
  // for (int x = 0; x <= 36; x++){
  //   for (int y = 0; y <= 36; y++){
  //     rect r = malloc(sizeof(struct rect));
  //     r->rectange = (Rectangle) {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
  //     r->tile = AIR;
  //     // TILES* tile = malloc(sizeof(TILES));
  //     sprintf(buffer, "%d:%d", x, y );
  //     if (x == 0 || y == 0 || x == 36 || y == 36 || x == 1 || y == 1 || x == 35 || y == 35 || (x == 15 && y == 15) || (x == 16 && y == 15) || (x == 17 && y == 15) || (x == 15 && y == 16) || (x == 16 && y == 16) || (x == 17 && y == 16)){
  //       r->tile = STONE;
  //     }
  //     else{
  //       r->tile = DIRT;
  //     }
  //     r->node = malloc(sizeof(struct pathNode));
  //     assert(r->node != NULL);
  //     r->node->hCost = 0;
  //     r->node->gCost = INT_MAX;
  //     r->node->fCost = INT_MAX;
  //     r->node->prev = NULL;
  //     r->node->x = x;
  //     r->node->y = y; 
  //     if (r->tile == DIRT){
  //       r->node->isWalkable = true;
  //     }
  //     else{
  //       r->node->isWalkable = false;
  //     }
  //     hashSet(map, buffer, r);
  //   }
  // }
  // hashDump(stdout, map);
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
