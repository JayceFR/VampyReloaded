#ifndef NPC_H
#define NPC_H

typedef enum{
    NPC_TYPE_VILLAGER, 
    NPC_TYPE_COW
} NPCType;

struct NPC{
    entity e;
    NPCType type; 
};
typedef struct NPC *NPC;

#endif // NPC_H