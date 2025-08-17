#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "raylib.h"
#include "map.h"
#include "physics.h"


entity entityCreate(Vector2 startPos, Rectangle rect){
  entity e = malloc(sizeof(struct entity));
  assert(e != NULL);
  e->pos = startPos;
  e->rect = rect;
  return e; 
}

static bool collideRect(entity e, hash map, rect *hitTile){
    dynarray arr = rectsAround(map, e->pos);
    for (int i = 0; i < arr->len; i++){
        rect r = arr->data[i];
        if (r->tile == STONE){
            if (CheckCollisionRecs(r->rectange, e->rect)){
                if (hitTile) *hitTile = r;
                return true;
            }
        }
    }
    return false;
}

void update(entity e, hash map, Vector2 newPos) {
    Vector2 oldPos = e->pos;

    // --- X axis first ---
    e->pos.x += newPos.x;
    e->rect.x = e->pos.x;
    dynarray arrX = rectsAround(map, e->pos);
    for (int i = 0; i < arrX->len; i++) {
      rect r = arrX->data[i];
      if (r->tile != STONE) continue;

      if (CheckCollisionRecs(r->rectange, e->rect)) {
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
    dynarray arrY = rectsAround(map, e->pos);
    for (int i = 0; i < arrY->len; i++) {
      rect r = arrY->data[i];
      if (r->tile != STONE) continue;

      if (CheckCollisionRecs(r->rectange, e->rect)) {
        Rectangle tr = r->rectange;

        if (newPos.y > 0) {
          e->pos.y = tr.y - e->rect.height;
        } else {
          e->pos.y = tr.y + tr.height;
        }

        e->rect.y = e->pos.y;
      }
    }
}


