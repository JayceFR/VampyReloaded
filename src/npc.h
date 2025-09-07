#ifndef NPC_H
#define NPC_H

#include "raylib.h"
#include "hash.h"

typedef enum{
    NPC_TYPE_VILLAGER, 
    NPC_TYPE_COW
} NPCType;

struct NPC{
    entity e;
    NPCType type; 
    int state; // 0 is idle // 1 is wander 
    int currentFrame; 
    float animTimer; 

    // Added fields:
    Vector2 vel;        // per-frame delta applied to entity via update()
    float stateTimer;   // time left in current idle/wait state (seconds)
    float moveTimer;    // time left while wandering (seconds)
    float speed;        // movement speed (per-frame units)
};
typedef struct NPC *NPC;

extern NPC npcCreate(int x, int y, int width, int height);
// Updated signature: pass map so movement uses collision
extern void npcUpdate(NPC npc, hash map);

#endif // NPC_H