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

#define CHUNK_SIZE 64   // one puzzle map
#define WORLD_W 3
#define WORLD_H 1
#define DOORS_SIZE (((WORLD_W - 1) * WORLD_H) + ((WORLD_H - 1) * WORLD_W))
#define GAME_WIDTH (WORLD_W * CHUNK_SIZE)
#define GAME_HEIGHT (WORLD_H * CHUNK_SIZE)
#define MAX_ROOMS 8

typedef struct {
    int x, y, w, h;
} Room;

// typedef enum { STONE, DIRT } TILES;

// -------- corridor carving ----------
#define IN_BOUNDS(nx,ny,W,H) ((nx) >= 0 && (nx) < (W) && (ny) >= 0 && (ny) < (H))
#define CELL(grid,W,x,y)     (grid[(y)*(W) + (x)])

// dimension-agnostic corridor carving (L-first or V-first, widened)
static void carve_corridor_grid(TILES *grid, int W, int H,
                                int x1, int y1, int x2, int y2, int width)
{
    if (rand() & 1) {
        // horizontal first
        int xa = (x1 < x2 ? x1 : x2), xb = (x1 > x2 ? x1 : x2);
        for (int x = xa; x <= xb; x++)
            for (int dy = -width/2; dy <= width/2; dy++) {
                int ny = y1 + dy;
                if (IN_BOUNDS(x, ny, W, H)) CELL(grid, W, x, ny) = DIRT;
            }
        int ya = (y1 < y2 ? y1 : y2), yb = (y1 > y2 ? y1 : y2);
        for (int y = ya; y <= yb; y++)
            for (int dx = -width/2; dx <= width/2; dx++) {
                int nx = x2 + dx;
                if (IN_BOUNDS(nx, y, W, H)) CELL(grid, W, nx, y) = DIRT;
            }
    } else {
        // vertical first
        int ya = (y1 < y2 ? y1 : y2), yb = (y1 > y2 ? y1 : y2);
        for (int y = ya; y <= yb; y++)
            for (int dx = -width/2; dx <= width/2; dx++) {
                int nx = x1 + dx;
                if (IN_BOUNDS(nx, y, W, H)) CELL(grid, W, nx, y) = DIRT;
            }
        int xa = (x1 < x2 ? x1 : x2), xb = (x1 > x2 ? x1 : x2);
        for (int x = xa; x <= xb; x++)
            for (int dy = -width/2; dy <= width/2; dy++) {
                int ny = y2 + dy;
                if (IN_BOUNDS(x, ny, W, H)) CELL(grid, W, x, ny) = DIRT;
            }
    }
}


// -------- generate one chunk ----------
int generatePuzzleMap(TILES chunk[CHUNK_SIZE][CHUNK_SIZE], Room rooms[MAX_ROOMS]) {
    // fill
    for (int y=0; y<CHUNK_SIZE; ++y)
        for (int x=0; x<CHUNK_SIZE; ++x)
            chunk[y][x] = STONE;

    // rooms (keep your overlap logic if you want)
    int roomCount = 0;
    for (int i=0; i<MAX_ROOMS; ++i) {
        int w = 6 + rand()%6, h = 6 + rand()%6;
        int x = 2 + rand()%(CHUNK_SIZE - w - 2);
        int y = 2 + rand()%(CHUNK_SIZE - h - 2);
        for (int yy=y; yy<y+h; ++yy)
            for (int xx=x; xx<x+w; ++xx)
                chunk[yy][xx] = DIRT;
        rooms[roomCount++] = (Room){x,y,w,h};
    }

    // connect rooms in-chunk (spanning path + occasional extra)
    int corridorWidth = 3;
    for (int i=1; i<roomCount; ++i) {
        int x1 = rooms[i-1].x + rooms[i-1].w/2;
        int y1 = rooms[i-1].y + rooms[i-1].h/2;
        int x2 = rooms[i].x   + rooms[i].w/2;
        int y2 = rooms[i].y   + rooms[i].h/2;
        carve_corridor_grid(&chunk[0][0], CHUNK_SIZE, CHUNK_SIZE, x1,y1,x2,y2,corridorWidth);

        if ((rand()%3)==0) { // an extra loop edge
            int j = rand()%i;
            int rx = rooms[j].x + rooms[j].w/2;
            int ry = rooms[j].y + rooms[j].h/2;
            carve_corridor_grid(&chunk[0][0], CHUNK_SIZE, CHUNK_SIZE, x1,y1,rx,ry,corridorWidth);
        }
    }

    return roomCount;
}

static Room* pick_eastmost(Room *rs, int n) {
    Room *best = NULL; int bestX = -1;
    for (int i=0;i<n;i++){ int rx = rs[i].x + rs[i].w; if (rx > bestX){bestX=rx; best=&rs[i];}}
    return best;
}
static Room* pick_westmost(Room *rs, int n) {
    Room *best = NULL; int bestX = INT_MAX;
    for (int i=0;i<n;i++){ int rx = rs[i].x; if (rx < bestX){bestX=rx; best=&rs[i];}}
    return best;
}
static Room* pick_southmost(Room *rs, int n) {
    Room *best = NULL; int bestY = -1;
    for (int i=0;i<n;i++){ int ry = rs[i].y + rs[i].h; if (ry > bestY){bestY=ry; best=&rs[i];}}
    return best;
}
static Room* pick_northmost(Room *rs, int n) {
    Room *best = NULL; int bestY = INT_MAX;
    for (int i=0;i<n;i++){ int ry = rs[i].y; if (ry < bestY){bestY=ry; best=&rs[i];}}
    return best;
}

static void connect_room_centers_world(TILES *world, int W, int H,
                                       Room *a, int ax, int ay,
                                       Room *b, int bx, int by,
                                       int corridorWidth, dynarray doors)
{
    // room centers in *world tile* coords
    int x1 = a->x + a->w/2 + ax*CHUNK_SIZE;
    int y1 = a->y + a->h/2 + ay*CHUNK_SIZE;
    int x2 = b->x + b->w/2 + bx*CHUNK_SIZE;
    int y2 = b->y + b->h/2 + by*CHUNK_SIZE;

    // 1) carve the corridor (random L orientation inside)
    carve_corridor_grid(world, W, H, x1, y1, x2, y2, corridorWidth);

    // 2) find crossing tile on the chunk boundary
    int doorTileX = -1, doorTileY = -1;

    if (ay == by && ax + 1 == bx) {
        // ---- horizontal neighbors: boundary is a vertical line ----
        int boundaryX = bx * CHUNK_SIZE; // leftmost column of the right chunk

        // Try both possible L-crossing rows (y1 for H-first, y2 for V-first),
        // widened by corridorWidth.
        int ys[2] = { y1, y2 };
        for (int i = 0; i < 2 && doorTileX == -1; ++i) {
            for (int dy = -corridorWidth/2; dy <= corridorWidth/2 && doorTileX == -1; ++dy) {
                int yy = ys[i] + dy;
                if (IN_BOUNDS(boundaryX, yy, W, H) && CELL(world, W, boundaryX, yy) == DIRT) {
                    doorTileX = boundaryX;
                    doorTileY = yy;
                }
            }
        }
        // Fallback: scan a small vertical band between y1..y2 on the boundary
        if (doorTileX == -1) {
            int ya = (y1 < y2 ? y1 : y2), yb = (y1 > y2 ? y1 : y2);
            for (int yy = ya; yy <= yb && doorTileX == -1; ++yy) {
                if (IN_BOUNDS(boundaryX, yy, W, H) && CELL(world, W, boundaryX, yy) == DIRT) {
                    doorTileX = boundaryX;
                    doorTileY = yy;
                }
            }
        }
    } else if (ax == bx && ay + 1 == by) {
        // ---- vertical neighbors: boundary is a horizontal line ----
        int boundaryY = by * CHUNK_SIZE; // top row of the bottom chunk

        // Try both possible L-crossing columns (x1 for V-first, x2 for H-first),
        // widened by corridorWidth.
        int xs[2] = { x1, x2 };
        for (int i = 0; i < 2 && doorTileY == -1; ++i) {
            for (int dx = -corridorWidth/2; dx <= corridorWidth/2 && doorTileY == -1; ++dx) {
                int xx = xs[i] + dx;
                if (IN_BOUNDS(xx, boundaryY, W, H) && CELL(world, W, xx, boundaryY) == DIRT) {
                    doorTileX = xx;
                    doorTileY = boundaryY;
                }
            }
        }
        // Fallback: scan a small horizontal band between x1..x2 on the boundary
        if (doorTileY == -1) {
            int xa = (x1 < x2 ? x1 : x2), xb = (x1 > x2 ? x1 : x2);
            for (int xx = xa; xx <= xb && doorTileY == -1; ++xx) {
                if (IN_BOUNDS(xx, boundaryY, W, H) && CELL(world, W, xx, boundaryY) == DIRT) {
                    doorTileX = xx;
                    doorTileY = boundaryY;
                }
            }
        }
    } else {
        // Not direct neighbors (shouldn't happen in your calls) — fallback to midpoint
        doorTileX = (x1 + x2) / 2;
        doorTileY = (y1 + y2) / 2;
    }

    // 3) store door in pixel coords (top-left of tile)
    if (doorTileX != -1) {
        Door door = malloc(sizeof(struct Door));
        door->ax = x1;
        door->ay = y1;
        door->bx = x2;
        door->by = y2;
        door->locked = true;

        door->pos = (Vector2){
            doorTileX * TILE_SIZE,
            doorTileY * TILE_SIZE
        };
        add_dynarray(doors, door);
    }
}


dynarray generateWorld(TILES world[GAME_HEIGHT][GAME_WIDTH]) {
    Room worldRooms[WORLD_H][WORLD_W][MAX_ROOMS];
    int roomCount[WORLD_H][WORLD_W];
    dynarray doors = create_dynarray(NULL, NULL);

    // fill with stone
    for (int y=0;y<GAME_HEIGHT;y++)
        for (int x=0;x<GAME_WIDTH;x++)
            world[y][x]=STONE;

    // generate chunks and paste
    for (int cy=0; cy<WORLD_H; cy++){
        for (int cx=0; cx<WORLD_W; cx++){
            TILES chunk[CHUNK_SIZE][CHUNK_SIZE];
            int cnt = generatePuzzleMap(chunk, worldRooms[cy][cx]);
            roomCount[cy][cx] = cnt;
            for (int y=0; y<CHUNK_SIZE; y++)
                for (int x=0; x<CHUNK_SIZE; x++)
                    world[cy*CHUNK_SIZE+y][cx*CHUNK_SIZE+x] = chunk[y][x];
        }
    }

    // connect horizontally (two connectors per border)
    for (int cy=0; cy<WORLD_H; cy++){
        for (int cx=0; cx<WORLD_W-1; cx++){
            if (roomCount[cy][cx] && roomCount[cy][cx+1]){
                Room *leftA  = pick_eastmost(worldRooms[cy][cx],   roomCount[cy][cx]);
                Room *rightB = pick_westmost(worldRooms[cy][cx+1], roomCount[cy][cx+1]);
                if (leftA && rightB) connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, leftA, cx, cy, rightB, cx+1, cy, 3, doors);

                // second redundancy: random rooms
                Room *ra = &worldRooms[cy][cx][rand()%roomCount[cy][cx]];
                Room *rb = &worldRooms[cy][cx+1][rand()%roomCount[cy][cx+1]];
                connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, ra, cx, cy, rb, cx+1, cy, 3, doors);
            }
        }
    }

    // connect vertically (two connectors per border)
    for (int cy=0; cy<WORLD_H-1; cy++){
        for (int cx=0; cx<WORLD_W; cx++){
            if (roomCount[cy][cx] && roomCount[cy+1][cx]){
                Room *topA  = pick_southmost(worldRooms[cy][cx],     roomCount[cy][cx]);
                Room *botB  = pick_northmost(worldRooms[cy+1][cx],   roomCount[cy+1][cx]);
                if (topA && botB) connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, topA, cx, cy, botB, cx, cy+1, 3, doors);

                Room *ra = &worldRooms[cy][cx][rand()%roomCount[cy][cx]];
                Room *rb = &worldRooms[cy+1][cx][rand()%roomCount[cy+1][cx]];
                connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, ra, cx, cy, rb, cx, cy+1, 3, doors);
            }
        }
    }

    return doors;

    // (optional but recommended) flood-fill connectivity fixup:
    // If some DIRT is isolated, punch a minimal corridor through the nearest wall.
    // Implementing a full BFS + component-merge is ~50 lines; shout if you want me to drop that in.
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

mapData mapCreate(){
  mapData data; 
  data.map = hashCreate(&tilesPrint, &tilesFree, NULL);

  TILES mappy[GAME_HEIGHT][GAME_WIDTH];
  srand(time(NULL));
  // generatePuzzleMap(mappy);
  data.doors = generateWorld(mappy);
  for (int y = 0; y < GAME_HEIGHT; y++){
    for (int x = 0; x < GAME_WIDTH; x++){
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
      hashSet(data.map, buffer, r);
    }
  }
  return data; 
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
