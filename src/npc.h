#ifndef NPC_H
#define NPC_H

typedef enum{
    NPC_TYPE_VILLAGER, 
    NPC_TYPE_COW
} NPCType;

struct NPC{
    entity e;
    NPCType type; 
    int state; // 0 is idle // 1 is run 
    int currentFrame; 
    float animTimer; 
};
typedef struct NPC *NPC;

extern NPC npcCreate(int x, int y, int width, int height);
extern void npcUpdate(NPC npc);

#endif // NPC_H