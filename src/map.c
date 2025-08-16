#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"

#include "hash.h"

typedef enum{
  DIRT,
  STONE
} TILES; 

#define TILE_SIZE 16 
#define GRID_SIZE (16 * TILE_SIZE) 

int** arrayEmpty(int rows, int cols){
  int **arr = malloc(rows * sizeof(int *));
  arr[0] = malloc(rows * cols * sizeof(int));

  // Point each row to the right spot in the block
  for (int i = 1; i < rows; i++) {
      arr[i] = arr[0] + i * cols;
  }

  // For now set all of them to DIRT 
  for (int i = 0; i < rows; i++){
    for (int j = 0; j < cols; j++){
      arr[i][j] = DIRT;
    }
  }

  return arr; 
}

void arrayFree(hashvalue val){
  int **arr = (int **) val; 
  free(arr[0]);
  free(arr);
}

hash mapCreate(Vector2 player_pos){
  hash map = hashCreate(NULL, &arrayFree, NULL);
  int gx = ((int) player_pos.x) / GRID_SIZE;
  int gy = ((int) player_pos.y) / GRID_SIZE;
  // Init the map for 3 grids around the player first 
  char buffer[22];
  for (int x = gx - 1; x <= gx + 1; x++){
    for (int y = gy - 1; y <= gy + 1; y++){
      sprintf(buffer, "%d:%d", x, y);
      int **arr = arrayEmpty(16, 16);
      hashSet(map, buffer, arr);
    }
  }
  return map; 
}

void mapDraw(hash map, Vector2 player_pos){
  int gx = ((int) player_pos.x) / GRID_SIZE;
  int gy = ((int) player_pos.y) / GRID_SIZE;
  // Init the map for 3 grids around the player first 
  char buffer[22];
  int **arr;
  for (int x = gx - 4; x <= gx + 4; x++){
    for (int y = gy - 4; y <= gy + 4; y++){
      sprintf(buffer, "%d:%d", x, y);
      if (hashFind(map, buffer) == NULL){
        arr = arrayEmpty(16, 16);
        hashSet(map, buffer, arr);
      }
      arr = hashFind(map, buffer);
      for (int cx = 0; cx < TILE_SIZE; cx++){
        for (int cy = 0; cy < TILE_SIZE; cy++){
          int worldX = (x * TILE_SIZE * TILE_SIZE) + (cx * TILE_SIZE);
          int worldY = (y * TILE_SIZE * TILE_SIZE) + (cy * TILE_SIZE);
          DrawRectangle(worldX, worldY, TILE_SIZE, TILE_SIZE, YELLOW);
        }
      }
    }
  }
}

void mapFree(hash map){
  hashFree(map);
}
