#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>

#include "raylib.h"
#include "raymath.h"
#include "dynarray.h"
#include "physics.h"
#include "map.h"
#include "enemy.h"

#define MOVE_STRAIGHT_COST 10
#define MOVE_DIAGONAL_COST 14
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int calculateDistanceCost(pathNode a, pathNode b){
    int xDistance = abs(a->x - b->x);
    int yDistance = abs(a->y - b->y);

    int remaining = abs(xDistance - yDistance);

    return MOVE_DIAGONAL_COST * MIN(xDistance, yDistance) + MOVE_STRAIGHT_COST * remaining;
}

static pathNode getLowestFCostNode(dynarray list, int *pos){
    pathNode lowestCostNode = list->data[0];
    *pos = 0; 
    for (int i = 1; i < list->len; i++){
        pathNode currentNode = list->data[i];
        if (currentNode->fCost < lowestCostNode->fCost){
            *pos = i; 
            lowestCostNode = currentNode;
        }
    }
    return lowestCostNode;
}

static dynarray calculatePath(pathNode endNode){
    dynarray path = create_dynarray(NULL, NULL);
    pathNode currentNode = endNode;

    // temporary stack to reverse
    dynarray temp = create_dynarray(NULL, NULL);
    while (currentNode != NULL){
        add_dynarray(temp, currentNode);
        currentNode = currentNode->prev;
    }

    // copy reversed order into final path
    for (int i = temp->len - 1; i >= 0; i--){
        add_dynarray(path, temp->data[i]);
    }

    free_dynarray(temp);
    return path;
}


static dynarray getNeighbourList(pathNode currentNode, hash map){
    dynarray neighbourList = create_dynarray(NULL, NULL);
    int offset[8][2] = {{1,0}, {0,-1}, {-1, 0}, {0, 1}, {1, -1}, {1, 1}, {-1, -1}, {-1, 1}};
    for (int i = 0; i < 8; i++){
        rect r = mapGetRecAt(map, currentNode->x + offset[i][0], currentNode->y + offset[i][1]);
        if (r != NULL){
            add_dynarray(neighbourList, r->node);
        }
    }
    return neighbourList;
}

static bool isIn(dynarray list, pathNode p){
    for (int i = 0; i < list->len; i++){
        pathNode curr = list->data[i];
        if (curr == p){
            return true;
        }
    }
    return false;
}

static void resetNode(pathNode node) {
    node->gCost = INT_MAX; // or some large number
    node->hCost = 0;
    node->fCost = 0;
    node->prev = NULL;
}


static dynarray pathFinding(Vector2 playerPos, Vector2 enemyPos, hash map){
    int pcx = ((int) playerPos.x) / TILE_SIZE;
    int pcy = ((int) playerPos.y) / TILE_SIZE;
    int ecx = ((int) enemyPos.x) / TILE_SIZE;
    int ecy = ((int) enemyPos.y) / TILE_SIZE;

    pathNode startNode = mapGetRecAt(map, ecx, ecy)->node;
    pathNode endNode   = mapGetRecAt(map, pcx, pcy)->node;

    dynarray openList = create_dynarray(NULL, NULL);
    dynarray closedList = create_dynarray(NULL, NULL);

    add_dynarray(openList, startNode);

    startNode->gCost = 0;
    startNode->hCost = calculateDistanceCost(startNode, endNode);
    startNode->fCost = startNode->gCost + startNode->hCost;

    while (openList->len > 0){
        int pos = 0; 
        pathNode currentNode = getLowestFCostNode(openList, &pos);
        if (currentNode == endNode){
            dynarray path = calculatePath(endNode);
            for (int i = 0; i < openList->len; i++)
                resetNode(openList->data[i]);
            for (int i = 0; i < closedList->len; i++)
                resetNode(closedList->data[i]);
            free_dynarray(openList);
            free_dynarray(closedList);
            return path;
        }
        remove_dynarray(openList, pos);
        add_dynarray(closedList, currentNode);

        dynarray neighbourList = getNeighbourList(currentNode, map);
        for (int i = 0; i < neighbourList->len; i++){
            pathNode neighbour = neighbourList->data[i];
            if (isIn(closedList, neighbour)){
                continue;
            }
            if (!neighbour->isWalkable){
                add_dynarray(closedList, neighbour);
                continue;
            }
            int tentaiveGCost = currentNode->gCost + calculateDistanceCost(currentNode, neighbour);
            if (tentaiveGCost < neighbour->gCost){
                neighbour->prev = currentNode;
                neighbour->gCost = tentaiveGCost;
                neighbour->hCost = calculateDistanceCost(neighbour, endNode);
                neighbour->fCost = neighbour->gCost + neighbour->hCost;

                if (!isIn(openList, neighbour)){
                    add_dynarray(openList, neighbour);
                }

            }
        }
    }

    for (int i = 0; i < openList->len; i++)
        resetNode(openList->data[i]);
    for (int i = 0; i < closedList->len; i++)
        resetNode(closedList->data[i]);

    free_dynarray(openList);
    free_dynarray(closedList);

    return NULL;

}

Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map) {
    int playerTileX = (int)(player->pos.x / TILE_SIZE);
    int playerTileY = (int)(player->pos.y / TILE_SIZE);
    int enemyTileX  = (int)(enemy->e->pos.x  / TILE_SIZE);
    int enemyTileY  = (int)(enemy->e->pos.y  / TILE_SIZE);

    bool needRecompute = false;

    // recompute if path not set
    if (enemy->path == NULL) {
        needRecompute = true;
    }
    // recompute if player moved tiles
    else if (enemy->targetTileX != playerTileX || enemy->targetTileY != playerTileY) {
        needRecompute = true;
    }
    // recompute if finished path
    else if (enemy->currentStep >= enemy->path->len) {
        needRecompute = true;
    }

    if (needRecompute) {
        if (enemy->path != NULL) {
            free_dynarray(enemy->path);
        }
        enemy->path = pathFinding(player->pos, enemy->e->pos, map);
        enemy->targetTileX = playerTileX;
        enemy->targetTileY = playerTileY;
        enemy->currentStep = 1; // step 0 is the start (enemy pos), so step 1 is the first move
    }

    if (enemy->path != NULL && enemy->currentStep < enemy->path->len) {
        pathNode nextNode = enemy->path->data[enemy->currentStep];
        Vector2 nextPos = { nextNode->x * TILE_SIZE, nextNode->y * TILE_SIZE };
        Vector2 toTarget = Vector2Subtract(nextPos, enemy->e->pos);

        // If close enough to next node → advance to next step
        if (Vector2Length(toTarget) < 2.0f) {
            enemy->currentStep++;
        }

        // Move toward current node
        if (enemy->currentStep < enemy->path->len) {
            nextNode = enemy->path->data[enemy->currentStep];
            nextPos = (Vector2){ nextNode->x * TILE_SIZE, nextNode->y * TILE_SIZE };
            Vector2 dir = Vector2Normalize(Vector2Subtract(nextPos, enemy->e->pos));
            return Vector2Scale(dir, 2.0f);
        }
    }

    return (Vector2){0, 0};
}

Enemy enemyCreate(int startX, int startY, int width, int height){
    Enemy enemy = malloc(sizeof(struct Enemy));
    assert(enemy != NULL);
    enemy->e = entityCreate(startX, startY, width, height);
    enemy->path = NULL;
    return enemy;
}