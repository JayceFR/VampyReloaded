#ifndef UTILS_H
#define UTILS_H

#include "raylib.h"

struct Animation{
    Texture2D *frames; 
    int numberOfFrames; 
};
typedef struct Animation *Animation;

extern Animation loadAnimation(char *path, int numberOfFrames);
extern Texture2D *loadTexturesFromDirectory(char *path, int numberOfTexs);
extern void loadDirectory();
extern void closeDirectory();

#endif