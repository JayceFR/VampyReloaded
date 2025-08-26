#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "utils.h"
#include "raylib.h"

void loadDirectory(){
    #ifndef PLATFORM_ANDROID
	    ChangeDirectory("assets");
    #endif
}

void closeDirectory(){
    #ifndef PLATFORM_ANDROID
	    ChangeDirectory("..");
    #endif
}

Animation loadAnimation(char *path, int numberOfFrames){
    loadDirectory();
    Animation animation = malloc(sizeof(struct Animation));
    assert(animation != NULL);
    animation->frames = malloc(sizeof(Texture2D) * numberOfFrames);
    assert(animation->frames != NULL);
    animation->numberOfFrames = numberOfFrames;

    char buffer[50];
    for (int i = 1; i < numberOfFrames + 1; i++){
        sprintf(buffer, "%s%d.png", path, i);
        animation->frames[i-1] = LoadTexture(buffer);
    }
    closeDirectory();
    return animation;
}