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

typedef struct {
    int x, y, w, h;
} Room;

#define MAX_ROOMS 20

void carveCorridor(TILES map[HEIGHT][WIDTH], int x1, int y1, int x2, int y2, int corridorWidth) {
    // L-shaped corridor with thickness
    if (rand() % 2) {
        // horizontal first
        for (int x = (x1<x2?x1:x2); x <= (x1>x2?x1:x2); x++) {
            for (int w = -corridorWidth/2; w <= corridorWidth/2; w++) {
                if (y1+w > 0 && y1+w < HEIGHT-1)
                    map[y1+w][x] = DIRT;
            }
        }
        for (int y = (y1<y2?y1:y2); y <= (y1>y2?y1:y2); y++) {
            for (int w = -corridorWidth/2; w <= corridorWidth/2; w++) {
                if (x2+w > 0 && x2+w < WIDTH-1)
                    map[y][x2+w] = DIRT;
            }
        }
    } else {
        // vertical first
        for (int y = (y1<y2?y1:y2); y <= (y1>y2?y1:y2); y++) {
            for (int w = -corridorWidth/2; w <= corridorWidth/2; w++) {
                if (x1+w > 0 && x1+w < WIDTH-1)
                    map[y][x1+w] = DIRT;
            }
        }
        for (int x = (x1<x2?x1:x2); x <= (x1>x2?x1:x2); x++) {
            for (int w = -corridorWidth/2; w <= corridorWidth/2; w++) {
                if (y2+w > 0 && y2+w < HEIGHT-1)
                    map[y2+w][x] = DIRT;
            }
        }
    }
}

void generatePuzzleMap(TILES map[HEIGHT][WIDTH]) {
    // Step 1: Fill with walls
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            map[y][x] = STONE;
        }
    }

    Room rooms[MAX_ROOMS];
    int roomCount = 0;

    // Step 2: Place random rooms
    for (int i = 0; i < MAX_ROOMS; i++) {
        int w = 6 + rand() % 6; // room width 6–11
        int h = 6 + rand() % 6; // room height 6–11
        int x = 2 + rand() % (WIDTH - w - 2);
        int y = 2 + rand() % (HEIGHT - h - 2);

        // carve room
        for (int yy = y; yy < y + h; yy++) {
            for (int xx = x; xx < x + w; xx++) {
                map[yy][xx] = DIRT;
            }
        }

        rooms[roomCount++] = (Room){x, y, w, h};
    }

    // Step 3: Connect rooms with wider corridors
    int corridorWidth = 3;  // adjust this (2, 3, 4…) for thicker paths
    for (int i = 1; i < roomCount; i++) {
        int x1 = rooms[i-1].x + rooms[i-1].w/2;
        int y1 = rooms[i-1].y + rooms[i-1].h/2;
        int x2 = rooms[i].x + rooms[i].w/2;
        int y2 = rooms[i].y + rooms[i].h/2;

        carveCorridor(map, x1, y1, x2, y2, corridorWidth);
    }

    // Step 4: Outer border
    for (int i = 0; i < WIDTH; i++) {
        map[0][i] = STONE;
        map[HEIGHT - 1][i] = STONE;
    }
    for (int i = 0; i < HEIGHT; i++) {
        map[i][0] = STONE;
        map[i][WIDTH - 1] = STONE;
    }
}

// void generateRandomWalkerMap(TILES map[HEIGHT][WIDTH]) {
//     // Fill with walls first
//     for (int y = 0; y < HEIGHT; y++) {
//         for (int x = 0; x < WIDTH; x++) {
//             map[y][x] = STONE;
//         }
//     }

//     int x = WIDTH / 2;
//     int y = HEIGHT / 2;

//     int floorCount = 0;
//     int targetFloorCount = (WIDTH * HEIGHT) / 2; // carve ~50% floor

//     while (floorCount < targetFloorCount) {
//         if (map[y][x] == STONE) {
//             map[y][x] = DIRT;
//             floorCount++;
//         }

//         int dir = rand() % 4;
//         if (dir == 0 && x > 1) x--;               // left
//         if (dir == 1 && x < WIDTH - 2) x++;       // right
//         if (dir == 2 && y > 1) y--;               // up
//         if (dir == 3 && y < HEIGHT - 2) y++;      // down
//     }

//     // Put a stone border so things are closed off
//     for (int i = 0; i < WIDTH; i++) {
//         map[0][i] = STONE;
//         map[HEIGHT - 1][i] = STONE;
//     }
//     for (int i = 0; i < HEIGHT; i++) {
//         map[i][0] = STONE;
//         map[i][WIDTH - 1] = STONE;
//     }
// }

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
  generatePuzzleMap(mappy);
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
