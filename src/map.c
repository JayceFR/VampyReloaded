#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "raylib.h"

#include "hash.h"
#include "dynarray.h"
#include "map.h"
#include "enemy.h"

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

#define WORLD_W 3
#define WORLD_H 3
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
                                       int corridorWidth)
{
    // room centers in *world tile* coords
    int x1 = a->x + a->w/2 + ax*CHUNK_SIZE;
    int y1 = a->y + a->h/2 + ay*CHUNK_SIZE;
    int x2 = b->x + b->w/2 + bx*CHUNK_SIZE;
    int y2 = b->y + b->h/2 + by*CHUNK_SIZE;

    // 1) carve the corridor (random L orientation inside)
    carve_corridor_grid(world, W, H, x1, y1, x2, y2, corridorWidth);

    // // 2) find crossing tile on the chunk boundary
    // int doorTileX = -1, doorTileY = -1;

    // if (ay == by && ax + 1 == bx) {
    //     // ---- horizontal neighbors: boundary is a vertical line ----
    //     int boundaryX = bx * CHUNK_SIZE; // leftmost column of the right chunk

    //     // Try both possible L-crossing rows (y1 for H-first, y2 for V-first),
    //     // widened by corridorWidth.
    //     int ys[2] = { y1, y2 };
    //     for (int i = 0; i < 2 && doorTileX == -1; ++i) {
    //         for (int dy = -corridorWidth/2; dy <= corridorWidth/2 && doorTileX == -1; ++dy) {
    //             int yy = ys[i] + dy;
    //             if (IN_BOUNDS(boundaryX, yy, W, H) && CELL(world, W, boundaryX, yy) == DIRT) {
    //                 doorTileX = boundaryX;
    //                 doorTileY = yy;
    //             }
    //         }
    //     }
    //     // Fallback: scan a small vertical band between y1..y2 on the boundary
    //     if (doorTileX == -1) {
    //         int ya = (y1 < y2 ? y1 : y2), yb = (y1 > y2 ? y1 : y2);
    //         for (int yy = ya; yy <= yb && doorTileX == -1; ++yy) {
    //             if (IN_BOUNDS(boundaryX, yy, W, H) && CELL(world, W, boundaryX, yy) == DIRT) {
    //                 doorTileX = boundaryX;
    //                 doorTileY = yy;
    //             }
    //         }
    //     }
    // } else if (ax == bx && ay + 1 == by) {
    //     // ---- vertical neighbors: boundary is a horizontal line ----
    //     int boundaryY = by * CHUNK_SIZE; // top row of the bottom chunk

    //     // Try both possible L-crossing columns (x1 for V-first, x2 for H-first),
    //     // widened by corridorWidth.
    //     int xs[2] = { x1, x2 };
    //     for (int i = 0; i < 2 && doorTileY == -1; ++i) {
    //         for (int dx = -corridorWidth/2; dx <= corridorWidth/2 && doorTileY == -1; ++dx) {
    //             int xx = xs[i] + dx;
    //             if (IN_BOUNDS(xx, boundaryY, W, H) && CELL(world, W, xx, boundaryY) == DIRT) {
    //                 doorTileX = xx;
    //                 doorTileY = boundaryY;
    //             }
    //         }
    //     }
    //     // Fallback: scan a small horizontal band between x1..x2 on the boundary
    //     if (doorTileY == -1) {
    //         int xa = (x1 < x2 ? x1 : x2), xb = (x1 > x2 ? x1 : x2);
    //         for (int xx = xa; xx <= xb && doorTileY == -1; ++xx) {
    //             if (IN_BOUNDS(xx, boundaryY, W, H) && CELL(world, W, xx, boundaryY) == DIRT) {
    //                 doorTileX = xx;
    //                 doorTileY = boundaryY;
    //             }
    //         }
    //     }
    // } else {
    //     // Not direct neighbors (shouldn't happen in your calls) — fallback to midpoint
    //     doorTileX = (x1 + x2) / 2;
    //     doorTileY = (y1 + y2) / 2;
    // }

    // // 3) store door in pixel coords (top-left of tile)
    // if (doorTileX != -1) {
    //     Door door = malloc(sizeof(struct Door));
    //     door->ax = ax * CHUNK_SIZE;
    //     door->ay = ay * CHUNK_SIZE;
    //     door->aw = CHUNK_SIZE; 
    //     door->ah = CHUNK_SIZE; 

    //     door->bx = bx * CHUNK_SIZE;
    //     door->by = by * CHUNK_SIZE;
    //     door->bw = CHUNK_SIZE; 
    //     door->bh = CHUNK_SIZE;
    //     door->locked = true;

    //     door->pos = (Vector2){
    //         doorTileX * TILE_SIZE,
    //         doorTileY * TILE_SIZE
    //     };
    //     add_dynarray(doors, door);
    // }
}


hash generateWorld(TILES world[GAME_HEIGHT][GAME_WIDTH]) {
    Room worldRooms[WORLD_H][WORLD_W][MAX_ROOMS];
    int roomCount[WORLD_H][WORLD_W];
    // dynarray doors = create_dynarray(NULL, NULL);

    // fill with stone
    for (int y=0;y<GAME_HEIGHT;y++)
        for (int x=0;x<GAME_WIDTH;x++)
            world[y][x]=STONE;

    // Enemy Spawner 
    // TODO: ADD in level difficulty as well, and lower the range for higher levels
    hash enemies = hashCreate(NULL, NULL, NULL);
    char buffer[22];
    dynarray enemy; 
    // generate chunks and paste
    for (int cy=0; cy<WORLD_H; cy++){
        for (int cx=0; cx<WORLD_W; cx++){
            TILES chunk[CHUNK_SIZE][CHUNK_SIZE];
            int cnt = generatePuzzleMap(chunk, worldRooms[cy][cx]);
            roomCount[cy][cx] = cnt;
            for (int y=0; y<CHUNK_SIZE; y++){
                for (int x=0; x<CHUNK_SIZE; x++){
                    if (chunk[y][x] == DIRT && GetRandomValue(1,100) == 2){
                        // Spawn an enemy in this location 
                        Enemy e = enemyCreate(
                            (cx * CHUNK_SIZE + x) * TILE_SIZE, 
                            (cy * CHUNK_SIZE + y) * TILE_SIZE,
                            15, 
                            15
                        );
                        sprintf(buffer, "%d:%d", cx, cy); 
                        if (hashFind(enemies, buffer) == NULL){
                            hashSet(enemies, buffer, create_dynarray(NULL, NULL));
                        } 
                        if ((enemy = hashFind(enemies, buffer)) != NULL){
                            add_dynarray(enemy, e);
                        }
                    }
                    world[cy*CHUNK_SIZE+y][cx*CHUNK_SIZE+x] = chunk[y][x];
                }
            }
        }
    }

    // connect horizontally (two connectors per border)
    for (int cy=0; cy<WORLD_H; cy++){
        for (int cx=0; cx<WORLD_W-1; cx++){
            if (roomCount[cy][cx] && roomCount[cy][cx+1]){
                Room *leftA  = pick_eastmost(worldRooms[cy][cx],   roomCount[cy][cx]);
                Room *rightB = pick_westmost(worldRooms[cy][cx+1], roomCount[cy][cx+1]);
                if (leftA && rightB) connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, leftA, cx, cy, rightB, cx+1, cy, 3);

                // second redundancy: random rooms
                Room *ra = &worldRooms[cy][cx][rand()%roomCount[cy][cx]];
                Room *rb = &worldRooms[cy][cx+1][rand()%roomCount[cy][cx+1]];
                connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, ra, cx, cy, rb, cx+1, cy, 3);
            }
        }
    }

    // connect vertically (two connectors per border)
    for (int cy=0; cy<WORLD_H-1; cy++){
        for (int cx=0; cx<WORLD_W; cx++){
            if (roomCount[cy][cx] && roomCount[cy+1][cx]){
                Room *topA  = pick_southmost(worldRooms[cy][cx],     roomCount[cy][cx]);
                Room *botB  = pick_northmost(worldRooms[cy+1][cx],   roomCount[cy+1][cx]);
                if (topA && botB) connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, topA, cx, cy, botB, cx, cy+1, 3);

                Room *ra = &worldRooms[cy][cx][rand()%roomCount[cy][cx]];
                Room *rb = &worldRooms[cy+1][cx][rand()%roomCount[cy+1][cx]];
                connect_room_centers_world(&world[0][0], GAME_WIDTH, GAME_HEIGHT, ra, cx, cy, rb, cx, cy+1, 3);
            }
        }
    }

    return enemies;
    // return doors;

    // (optional but recommended) flood-fill connectivity fixup:
    // If some DIRT is isolated, punch a minimal corridor through the nearest wall.
    // Implementing a full BFS + component-merge is ~50 lines; shout if you want me to drop that in.
}

Door getPlayerRoomDoor(dynarray doors, Vector2 playerPos) {
    for (int i = 0; i < doors->len; i++) {
        Door door = doors->data[i];

        // check room A bounds
        if (playerPos.x >= door->ax*TILE_SIZE && playerPos.x < (door->ax + door->aw)*TILE_SIZE &&
            playerPos.y >= door->ay*TILE_SIZE && playerPos.y < (door->ay + door->ah)*TILE_SIZE) {
            return door; // in room A
        }

        // check room B bounds
        if (playerPos.x >= door->bx*TILE_SIZE && playerPos.x < (door->bx + door->bw)*TILE_SIZE &&
            playerPos.y >= door->by*TILE_SIZE && playerPos.y < (door->by + door->bh)*TILE_SIZE) {
            return door; // in room B
        }
    }
    return NULL; // not in any room
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
  data.enemies = generateWorld(mappy);
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

// Rectangle GetCameraWorldBounds(Camera2D camera) {
//     Vector2 topLeft = GetScreenToWorld2D((Vector2){0,0}, camera);
//     Vector2 bottomRight = GetScreenToWorld2D(
//         (Vector2){GetScreenWidth(), GetScreenHeight()},
//         camera
//     );

//     Rectangle bounds = {
//         topLeft.x, topLeft.y,
//         bottomRight.x - topLeft.x,
//         bottomRight.y - topLeft.y
//     };
//     return bounds;
// }

// void mapDraw(hash map, Camera2D camera) {
//     // Get visible area in world space
//     Rectangle bounds = GetCameraWorldBounds(camera);

//     // Convert to tile indices
//     int minX = (int)(bounds.x / TILE_SIZE);
//     int maxX = (int)((bounds.x + bounds.width) / TILE_SIZE);
//     int minY = (int)(bounds.y / TILE_SIZE);
//     int maxY = (int)((bounds.y + bounds.height) / TILE_SIZE);

//     char buffer[32];
//     for (int x = minX; x <= maxX; x++) {
//         for (int y = minY; y <= maxY; y++) {
//             sprintf(buffer, "%d:%d", x, y);
//             TILES *tile = hashFind(map, buffer);
//             if (tile) {
//                 int worldX = x * TILE_SIZE;
//                 int worldY = y * TILE_SIZE;

//                 if (*tile == DIRT) {
//                     DrawRectangle(worldX, worldY, TILE_SIZE, TILE_SIZE, GRAY);
//                 } else if (*tile == STONE) {
//                     DrawRectangle(worldX, worldY, TILE_SIZE, TILE_SIZE, BLACK);
//                 }
//             }
//         }
//     }
// }

// ------------ batching cache -------------
typedef struct RoomCache {
    bool dirty;
    RenderTexture2D tex;
    Rectangle worldRect;   // cached rect in world coords (snapped to tiles, padded)
} RoomCache;

static RoomCache s_cache = {0};

static Rectangle GetCameraWorldBounds(Camera2D cam) {
    Vector2 tl = GetScreenToWorld2D((Vector2){0, 0}, cam);
    Vector2 br = GetScreenToWorld2D((Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}, cam);
    return (Rectangle){ tl.x, tl.y, br.x - tl.x, br.y - tl.y };
}

void MapEnsureCache(hash map, Camera2D camera, Texture2D *tileMap) {
    const int PAD_TILES_X = 2, PAD_TILES_Y = 2;

    Rectangle view = GetCameraWorldBounds(camera);

    // snap + pad in tile space
    int minTileX = (int)floorf(view.x / TILE_SIZE) - PAD_TILES_X;
    int minTileY = (int)floorf(view.y / TILE_SIZE) - PAD_TILES_Y;
    int maxTileX = (int)ceilf((view.x + view.width) / TILE_SIZE) + PAD_TILES_X;
    int maxTileY = (int)ceilf((view.y + view.height) / TILE_SIZE) + PAD_TILES_Y;

    if (maxTileX < minTileX) { int t=minTileX; minTileX=maxTileX; maxTileX=t; }
    if (maxTileY < minTileY) { int t=minTileY; minTileY=maxTileY; maxTileY=t; }

    int cacheX = minTileX * TILE_SIZE;
    int cacheY = minTileY * TILE_SIZE;
    int cacheW = (maxTileX - minTileX) * TILE_SIZE;
    int cacheH = (maxTileY - minTileY) * TILE_SIZE;

    bool needResize =
        !s_cache.tex.id ||
        s_cache.tex.texture.width  != cacheW ||
        s_cache.tex.texture.height != cacheH;

    bool movedOutside =
        (view.x < s_cache.worldRect.x) ||
        (view.y < s_cache.worldRect.y) ||
        (view.x + view.width  > s_cache.worldRect.x + s_cache.worldRect.width) ||
        (view.y + view.height > s_cache.worldRect.y + s_cache.worldRect.height);

    if (needResize) {
        if (s_cache.tex.id) UnloadRenderTexture(s_cache.tex);
        s_cache.tex = LoadRenderTexture(cacheW, cacheH);
    }

    if (needResize || movedOutside) {
        s_cache.worldRect = (Rectangle){ (float)cacheX, (float)cacheY, (float)cacheW, (float)cacheH };

        // IMPORTANT: draw cache with no camera, no nesting inside your main target
        BeginTextureMode(s_cache.tex);
            ClearBackground(BLACK); // or BLANK, your choice

            char key[32];
            for (int tx = minTileX; tx < maxTileX; tx++) {
                for (int ty = minTileY; ty < maxTileY; ty++) {
                    sprintf(key, "%d:%d", tx, ty);
                    rect r = (rect)hashFind(map, key);   // <-- correct type!
                    if (!r) continue;

                    int lx = tx * TILE_SIZE - cacheX;    // local coords in cache
                    int ly = ty * TILE_SIZE - cacheY;

                    if (r->tile == DIRT) {
                        // DrawRectangle(lx, ly, TILE_SIZE, TILE_SIZE, GRAY);
                        DrawTexture(tileMap[DIRT], lx, ly, WHITE);
                    } else if (r->tile == STONE) {
                        // DrawRectangle(lx, ly, TILE_SIZE, TILE_SIZE, BLACK);
                        DrawTexture(tileMap[STONE], lx, ly, WHITE);
                    }
                }
            }
        EndTextureMode();
    }
}

void MapDrawCached(Camera2D camera) {
    if (!s_cache.tex.id) return;
    DrawTexturePro(
        s_cache.tex.texture,
        (Rectangle){ 0, 0, (float)s_cache.tex.texture.width, -(float)s_cache.tex.texture.height },
        (Rectangle){ s_cache.worldRect.x, s_cache.worldRect.y, s_cache.worldRect.width, s_cache.worldRect.height },
        (Vector2){ 0, 0 }, 0.0f, WHITE
    );
}

// void mapDraw(hash map, Camera2D camera) {
//     const int PAD_TILES_X = 2;         // extra tiles around the screen to reduce rebuilds
//     const int PAD_TILES_Y = 2;

//     Rectangle view = GetCameraWorldBounds(camera);

//     // snap + pad in *tile* space
//     int minTileX = (int)floorf(view.x / TILE_SIZE) - PAD_TILES_X;
//     int minTileY = (int)floorf(view.y / TILE_SIZE) - PAD_TILES_Y;
//     int maxTileX = (int)ceilf((view.x + view.width) / TILE_SIZE) + PAD_TILES_X;
//     int maxTileY = (int)ceilf((view.y + view.height) / TILE_SIZE) + PAD_TILES_Y;

//     // clamp order
//     if (maxTileX < minTileX) { int t=minTileX; minTileX=maxTileX; maxTileX=t; }
//     if (maxTileY < minTileY) { int t=minTileY; minTileY=maxTileY; maxTileY=t; }

//     // convert to *pixel* space (snapped to tiles)
//     int cacheX = minTileX * TILE_SIZE;
//     int cacheY = minTileY * TILE_SIZE;
//     int cacheW = (maxTileX - minTileX) * TILE_SIZE;
//     int cacheH = (maxTileY - minTileY) * TILE_SIZE;

//     bool needResize =
//         !s_cache.tex.id ||
//         s_cache.tex.texture.width  != cacheW ||
//         s_cache.tex.texture.height != cacheH;

//     bool movedOutside =
//         (view.x < s_cache.worldRect.x) ||
//         (view.y < s_cache.worldRect.y) ||
//         (view.x + view.width  > s_cache.worldRect.x + s_cache.worldRect.width) ||
//         (view.y + view.height > s_cache.worldRect.y + s_cache.worldRect.height);

//     if (needResize) {
//         if (s_cache.tex.id) UnloadRenderTexture(s_cache.tex);
//         s_cache.tex = LoadRenderTexture(cacheW, cacheH);
//         s_cache.dirty = true;
//     }

//     if (needResize || movedOutside) {
//         // --- Rebuild cache with identity transform (no camera!) ---
//         // We assume mapDraw is called *inside* BeginMode2D(camera); temporarily disable it:
//         EndMode2D();

//         s_cache.worldRect = (Rectangle){ (float)cacheX, (float)cacheY, (float)cacheW, (float)cacheH };

//         BeginTextureMode(s_cache.tex);
//             ClearBackground(BLANK);

//             char key[32];
//             for (int tx = minTileX; tx < maxTileX; tx++) {
//                 for (int ty = minTileY; ty < maxTileY; ty++) {
//                     sprintf(key, "%d:%d", tx, ty);
//                     TILES *tile = hashFind(map, key);
//                     if (!tile) continue;

//                     // local (cache) coords
//                     int lx = tx * TILE_SIZE - cacheX;
//                     int ly = ty * TILE_SIZE - cacheY;

//                     if (*tile == DIRT) {
//                         DrawRectangle(lx, ly, TILE_SIZE, TILE_SIZE, GRAY);
//                     } else if (*tile == STONE) {
//                         DrawRectangle(lx, ly, TILE_SIZE, TILE_SIZE, BLACK);
//                     }
//                 }
//             }
//         EndTextureMode();

//         // restore camera for world drawing
//         BeginMode2D(camera);
//         s_cache.dirty = false;
//     }

//     // --- Draw cached quad in world-space (now camera is active) ---
//     DrawTexturePro(
//         s_cache.tex.texture,
//         (Rectangle){ 0, 0, (float)s_cache.tex.texture.width, -(float)s_cache.tex.texture.height }, // flip Y
//         (Rectangle){ s_cache.worldRect.x, s_cache.worldRect.y, s_cache.worldRect.width, s_cache.worldRect.height },
//         (Vector2){ 0, 0 }, 0.0f, WHITE
//     );
// }





void mapFree(hash map){
  hashFree(map);
}
