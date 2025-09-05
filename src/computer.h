#ifndef COMPUTER_H
#define COMPUTER_H

#include "physics.h"

struct Computer{
    entity e; 
    bool hacked; 
    int amountLeftToHack; // 100 when inited
};
typedef struct Computer *Computer; 

#endif