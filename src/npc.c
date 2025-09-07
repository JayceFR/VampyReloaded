#include "stdio.h"
#include "stdlib.h"

#include "physics.h"
#include "npc.h"


NPC npcCreate(int x, int y, int width, int height){
    NPC npc = malloc(sizeof(struct NPC));
    npc->e = entityCreate(x, y, width, height);
    npc->type = NPC_TYPE_COW;
    npc->state = 0; // idle
    npc->currentFrame = GetRandomValue(0,3);
    npc->animTimer = 0.0f;
    return npc; 
}

void npcUpdate(NPC npc){
    npc->animTimer += GetFrameTime();
    if (npc->animTimer > 0.2f) {
        npc->animTimer = 0.0f;
        npc->currentFrame = (npc->currentFrame + 1) % 4;
    }
}
