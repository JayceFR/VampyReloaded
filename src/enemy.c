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
    int offset[8][2] = {
        { 1,  0}, { 0, -1}, {-1,  0}, { 0,  1}, // Cardinal
        { 1, -1}, { 1,  1}, {-1, -1}, {-1,  1}  // Diagonal
    };

    for (int i = 0; i < 8; i++){
        int nx = currentNode->x + offset[i][0];
        int ny = currentNode->y + offset[i][1];

        rect r = mapGetRecAt(map, nx, ny);
        if (!r || !r->node->isWalkable) continue;

        // Prevent diagonal corner-cutting
        if (offset[i][0] != 0 && offset[i][1] != 0) {
            rect r1 = mapGetRecAt(map, currentNode->x + offset[i][0], currentNode->y);     // Horizontal neighbor
            rect r2 = mapGetRecAt(map, currentNode->x, currentNode->y + offset[i][1]);     // Vertical neighbor
            if (!r1 || !r1->node->isWalkable || !r2 || !r2->node->isWalkable) {
                continue; // If either is blocked, skip this diagonal
            }
        }

        add_dynarray(neighbourList, r->node);
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

void updateAngle(Enemy e, Vector2 vel){
    e->angle = atan2f(vel.y , vel.x);
}

bool HasLOS(Vector2 from, Vector2 to, hash map) {
    Vector2 dir = Vector2Normalize(Vector2Subtract(to, from));
    Vector2 step = Vector2Scale(dir, 4.0f); 
    Vector2 ray = from;
    float maxDist = Vector2Distance(from, to);

    dynarray rects = rectsAround(map, from);

    float distTravelled = 0;
    bool blocked = false;

    while (distTravelled < maxDist) {
        if (CheckCollisionPointRec(ray, (Rectangle){to.x-2, to.y-2, 4, 4})) {
            // hit player region
            free_dynarray(rects);
            return true;
        }

        for (int i = 0; i < rects->len; i++) {
            rect r = rects->data[i];
            if (CheckCollisionPointRec(ray, r->rectange)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;

        ray = Vector2Add(ray, step);
        distTravelled += Vector2Length(step);
    }

    free_dynarray(rects);
    return false;
}


bool PlayerInTorchCone(Enemy enemy, entity player, float torchRadius, float torchFOV, hash map) {
    Vector2 toPlayer = Vector2Subtract(player->pos, enemy->e->pos);
    float dist = Vector2Length(toPlayer);

    // Too far
    if (dist > torchRadius) return false;

    // Angle check
    Vector2 dirToPlayer = Vector2Normalize(toPlayer);
    Vector2 facing = (Vector2){ cosf(enemy->angle), sinf(enemy->angle) };

    float dot = Vector2DotProduct(facing, dirToPlayer);
    float angleToPlayer = acosf(dot);

    if (angleToPlayer > torchFOV * 0.5f) return false;

    // LOS check (optional but recommended)
    if (!HasLOS(enemy->e->pos, player->pos, map)) return false;

    return true;
}

// Smoothly rotate enemy toward velocity
void updateAngleSmooth(Enemy e, Vector2 vel, float turnSpeed) {
    if (fabsf(vel.x) < 0.01f && fabsf(vel.y) < 0.01f) return; // cheap check
    float targetAngle = atan2f(vel.y, vel.x);
    float delta = targetAngle - e->angle;
    if (delta > PI) delta -= 2*PI;
    if (delta < -PI) delta += 2*PI;
    e->angle += delta * turnSpeed * GetFrameTime();
}



#define torchRadius 150
#define torchFOV (60 * (PI/180)) // 60 degree cone

Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map) {
    Vector2 vel = {0, 0};
    Vector2 toPlayer = Vector2Subtract(player->pos, enemy->e->pos);
    float distToPlayer = Vector2Length(toPlayer);

    // Update enemy state
    if (PlayerInTorchCone(enemy, player, torchRadius, torchFOV, map)) {
        enemy->state = ACTIVE;
    } else if (distToPlayer > torchRadius * 1.5f) {
        enemy->state = IDLE;
    }

    // ===== ACTIVE STATE =====
    if (enemy->state == ACTIVE) {
        int playerTileX = (int)(player->pos.x / TILE_SIZE);
        int playerTileY = (int)(player->pos.y / TILE_SIZE);

        bool needRecompute = false;
        if (!enemy->path ||
            enemy->targetTileX != playerTileX || enemy->targetTileY != playerTileY ||
            enemy->currentStep >= enemy->path->len) {
            needRecompute = true;
        }

        if (needRecompute) {
            if (enemy->path) free_dynarray(enemy->path);
            enemy->path = pathFinding(player->pos, enemy->e->pos, map);
            enemy->targetTileX = playerTileX;
            enemy->targetTileY = playerTileY;
            enemy->currentStep = 1;
        }

        if (enemy->path && enemy->currentStep < enemy->path->len) {
            pathNode nextNode = enemy->path->data[enemy->currentStep];
            Vector2 nextPos = {
                nextNode->x * TILE_SIZE + TILE_SIZE / 2 - enemy->e->rect.width / 2,
                nextNode->y * TILE_SIZE + TILE_SIZE / 2 - enemy->e->rect.height / 2
            };
            Vector2 dir = Vector2Subtract(nextPos, enemy->e->pos);

            if (Vector2Length(dir) < 2.0f) {
                enemy->currentStep++;
            } else {
                dir = Vector2Normalize(dir);
                vel = Vector2Scale(dir, 2.0f); // chasing speed
                updateAngleSmooth(enemy, vel, 4.0f);
                return vel;
            }
        }
        updateAngleSmooth(enemy, vel, 4.0f);
        return vel; // stationary if path finished
    }

    // ===== IDLE STATE =====
    if (enemy->state == IDLE) {
        // Idle timer handling
        if (enemy->idleTimer <= 0) {
            if (enemy->movingIdle) {
                enemy->movingIdle = false;
                enemy->idleTimer = GetRandomValue(30, 90) / 60.0f; // wait 0.5-1.5s
                return vel;
            } else {
                enemy->movingIdle = true;
                enemy->idleTimer = GetRandomValue(60, 180) / 60.0f; // move 1-3s

                // Weighted angle toward player (30%) + random (70%)
                float playerAngle = atan2f(toPlayer.y, toPlayer.x);
                float randomAngle = GetRandomValue(0, 360) * DEG2RAD;
                float wanderAngle = playerAngle * 0.3f + randomAngle * 0.7f;

                // Random wander distance
                float dist = GetRandomValue(20, 80);
                Vector2 candidateTarget = Vector2Add(enemy->e->pos,
                                                     (Vector2){cosf(wanderAngle) * dist, sinf(wanderAngle) * dist});
                

                // enemy->idleTarget = candidateTarget;
                // Check collision with walls
                rect r = mapGetRecAt(map, (int)(candidateTarget.x / TILE_SIZE), (int)(candidateTarget.y / TILE_SIZE));
                if (!r || !r->node->isWalkable) {
                    // fallback: just stay in place if blocked
                    enemy->movingIdle = false;
                    enemy->idleTarget = enemy->e->pos;
                } else {
                    enemy->idleTarget = candidateTarget;
                }
            }
        }

        // Decrease timer
        enemy->idleTimer -= GetFrameTime();

        if (enemy->movingIdle) {
            Vector2 toTarget = Vector2Subtract(enemy->idleTarget, enemy->e->pos);
            if (Vector2Length(toTarget) < 2.0f) {
                enemy->movingIdle = false;
                enemy->idleTimer = GetRandomValue(30, 90) / 60.0f; // wait again
                updateAngleSmooth(enemy, vel, 4.0f);
                return vel;
            }

            Vector2 dir = Vector2Normalize(toTarget);
            vel = Vector2Scale(dir, 1.0f); // slower than chasing
            updateAngleSmooth(enemy, vel, 4.0f);
            return vel;
        }
    }

    updateAngleSmooth(enemy, vel, 4.0f);
    return vel;
}



Enemy enemyCreate(int startX, int startY, int width, int height){
    Enemy enemy = malloc(sizeof(struct Enemy));
    assert(enemy != NULL);
    enemy->e = entityCreate(startX, startY, width, height);
    enemy->path = NULL;
    enemy->state = IDLE;
    enemy->idleTimer = 0;
    enemy->angle = 0;
    return enemy;
}

void enemyDrawTorch(Enemy e, hash map, int rays, Color col) {
    Vector2 origin = e->e->pos;
    dynarray rects = rectsAround(map, origin);

    float angleStep = torchFOV / rays;
    float rayDistances[rays + 1];

    for (int i = 0; i <= rays; i++) {
        float a = e->angle - torchFOV / 2 + angleStep * i;
        Vector2 dir = { cosf(a), sinf(a) };
        Vector2 ray = origin;
        float traveled = 0;
        bool blocked = false;

        while (traveled < torchRadius) {
            for (int j = 0; j < rects->len; j++) {
                rect r = rects->data[j];
                if (CheckCollisionPointRec(ray, r->rectange)) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) break;

            ray = Vector2Add(ray, Vector2Scale(dir, 4.0f));
            traveled += 4.0f;
        }

        rayDistances[i] = traveled;
    }

    free_dynarray(rects);

    // Draw a filled torch cone using DrawCircleSector approximation
    for (int i = 0; i < rays; i++) {
        float startAngle = (e->angle - torchFOV/2 + angleStep * i) * RAD2DEG;
        float endAngle   = (e->angle - torchFOV/2 + angleStep * (i+1)) * RAD2DEG;
        float radius     = MIN(rayDistances[i], rayDistances[i+1]);

        DrawCircleSector(origin, radius, startAngle, endAngle, 1, col);
    }
}




void enemyDraw(Enemy e, hash map){
    DrawRectangleRec(e->e->rect, RED);
    // enemyDrawTorchLines(e, map, 40, ColorAlpha(YELLOW, 0.3f));
    BeginBlendMode(BLEND_ADDITIVE);
    enemyDrawTorch(e, map, 40, ColorAlpha(WHITE, 0.2f));
    EndBlendMode();
    // BeginBlendMode(BLEND_ADDITIVE); // Additive blending for glow
    // int segments = 50;
    // for (int i = 0; i <= segments; i++) {
    //     float angle = e->angle - torchFOV/2 + (torchFOV / segments) * i;
    //     Vector2 endPos = (Vector2){ e->e->pos.x + cos(angle)*torchRadius,
    //                                 e->e->pos.y + sin(angle)*torchRadius };
    //     DrawLineV(e->e->pos, endPos, ColorAlpha(YELLOW, 0.3f));
    // }
    // EndBlendMode();
}