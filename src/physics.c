#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "raylib.h"
#include "map.h"
#include "physics.h"


entity entityCreate(float startX, float startY, int width, int height){
  entity e = malloc(sizeof(struct entity));
  assert(e != NULL);
  e->pos = (Vector2) {startX, startY};
  e->rect = (Rectangle) {startX, startY, width, height};
  return e; 
}

// static bool collideRect(entity e, hash map, rect *hitTile){
//     dynarray arr = rectsAround(map, e->pos);
//     for (int i = 0; i < arr->len; i++){
//         rect r = arr->data[i];
//         if (r->tile == STONE){
//             if (CheckCollisionRecs(r->rectange, e->rect)){
//                 if (hitTile) *hitTile = r;
//                 return true;
//             }
//         }
//     }
//     return false;
// }

// I dont think so the copy works here. 
static bool collideRect(entity e, hash map, rect *hitTile){
    struct rect nearby[MAX_RECTS];          // static array to hold rects
    int count = rectsAround(map, e->pos, nearby); // fill array and get count

    for (int i = 0; i < count; i++){
        rect r = &nearby[i];       // pointer to current rect
        if (r->tile == STONE){
            if (CheckCollisionRecs(r->rectange, e->rect)){
                if (hitTile) *hitTile = r; // copy the rect to hitTile
                return true;
            }
        }
    }
    return false;
}

// Returns true if collided
bool update(entity e, hash map, Vector2 newPos) {
    Vector2 oldPos = e->pos;
    bool collided = false;
    // --- X axis first ---
    e->pos.x += newPos.x;
    e->rect.x = e->pos.x;

    struct rect arrX[MAX_RECTS];
    int countX = rectsAround(map, e->pos, arrX);
    for (int i = 0; i < countX; i++) {
      rect r = &arrX[i];
      if (r->tile != STONE) continue;

      if (CheckCollisionRecs(r->rectange, e->rect)) {
        collided = true;
        Rectangle tr = r->rectange;

        if (newPos.x > 0) {
          e->pos.x = tr.x - e->rect.width;
        } else {
          e->pos.x = tr.x + tr.width;
        }

        e->rect.x = e->pos.x;
      }
    }

    // --- Y axis next ---
    e->pos.y += newPos.y;
    e->rect.y = e->pos.y;
    struct rect arrY[MAX_RECTS];
    int countY = rectsAround(map, e->pos, arrY);
    for (int i = 0; i < countY; i++) {
      rect r = &arrY[i];
      if (r->tile != STONE) continue;

      if (CheckCollisionRecs(r->rectange, e->rect)) {
        Rectangle tr = r->rectange;
        collided = true;

        if (newPos.y > 0) {
          e->pos.y = tr.y - e->rect.height;
        } else {
          e->pos.y = tr.y + tr.height;
        }

        e->rect.y = e->pos.y;
      }
    }
    return collided;
}


