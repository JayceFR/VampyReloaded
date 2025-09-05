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
#include "utils.h"
#include "projectile.h"

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

    if (pcx == ecx && pcy == ecy) {
        // Already at goal tile: create a 1-node path
        dynarray p = create_dynarray(NULL, NULL);
        rect r = mapGetRecAt(map, ecx, ecy);
        if (r) add_dynarray(p, r->node);
        return p;
    }

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

// Smoothly rotate enemy toward velocity (unchanged)
static inline void updateAngleSmooth(Enemy e, Vector2 vel, float turnSpeed) {
    if (fabsf(vel.x) < 0.01f && fabsf(vel.y) < 0.01f) return;
    float targetAngle = atan2f(vel.y, vel.x);
    float delta = targetAngle - e->angle;
    if (delta > PI) delta -= 2*PI;
    if (delta < -PI) delta += 2*PI;
    e->angle += delta * turnSpeed * GetFrameTime();
}

#define torchRadius 150
#define torchFOV (60 * (PI/180)) // 60-degree cone

static inline void worldCenterOfNode(pathNode n, Rectangle entRect, Vector2 *out) {
    out->x = n->x * TILE_SIZE + TILE_SIZE/2.0f - entRect.width  / 2.0f;
    out->y = n->y * TILE_SIZE + TILE_SIZE/2.0f - entRect.height / 2.0f;
}

Vector2 computeVelOfEnemy(Enemy enemy, entity player, hash map, dynarray projectiles, bool isHacking) {
    const float dt = GetFrameTime();

    // --- Animation ---
    enemy->animTimer += dt;
    if (enemy->animTimer > 0.1f) {
        enemy->animTimer = 0.0f;
        enemy->currentFrame = (enemy->currentFrame + 1) % 4;
    }

    Vector2 vel = (Vector2){0,0};

    // --- Throttle sensing (vision/LOS) ---
    enemy->senseCooldown -= dt;
    if (enemy->senseCooldown <= 0.0f) {
        float baseSensePeriod = 0.10f;
        enemy->senseCooldown = baseSensePeriod + (enemy->staggerSlot * 0.01f);
        enemy->playerVisible = PlayerInTorchCone(enemy, player, torchRadius, torchFOV, map);
        if (enemy->playerVisible) enemy->lastKnownPlayerPos = player->pos;
    }

    // --- State update (cheap) ---
    // float distToLastKnown = Vector2Distance(enemy->e->pos, enemy->lastKnownPlayerPos);
    // if (enemy->playerVisible)       enemy->state = ACTIVE;
    // else if (distToLastKnown > torchRadius * 1.5f) enemy->state = IDLE;

    // --- State update (cheap) ---
    float distToLastKnown = Vector2Distance(enemy->e->pos, enemy->lastKnownPlayerPos);

    if (isHacking) {
        // Force enemy into ACTIVE and lock it there while hacking
        enemy->state = ACTIVE;
        enemy->lastKnownPlayerPos = player->pos;
        // enemy->playerVisible = true;  // treat as if player is always "seen"
    } else {
        if (enemy->playerVisible)       
            enemy->state = ACTIVE;
        else if (distToLastKnown > torchRadius * 1.5f) 
            enemy->state = IDLE;
    }

    // --- ACTIVE: follow player path ---
    if (enemy->state == ACTIVE) {
        int goalX = (int)(player->pos.x) / TILE_SIZE;
        int goalY = (int)(player->pos.y) / TILE_SIZE;

        bool needRecompute = false;
        if (!enemy->path || enemy->currentStep >= (enemy->path->len)) needRecompute = true;
        if (goalX != enemy->lastGoalTileX || goalY != enemy->lastGoalTileY) needRecompute = true;

        enemy->repathCooldown -= dt;

        if (needRecompute && enemy->repathCooldown <= 0.0f) {
            if (enemy->path) { free_dynarray(enemy->path); enemy->path = NULL; }
            enemy->path = pathFinding(player->pos, enemy->e->pos, map);
            enemy->lastGoalTileX = goalX;
            enemy->lastGoalTileY = goalY;
            enemy->currentStep = 1;
            enemy->repathCooldown = enemy->repathInterval + (GetRandomValue(-25,25) * 0.001f);
        }

        if (enemy->path && enemy->currentStep < enemy->path->len) {
            pathNode nextNode = enemy->path->data[enemy->currentStep];
            Vector2 nextPos; worldCenterOfNode(nextNode, enemy->e->rect, &nextPos);
            Vector2 toTarget = Vector2Subtract(nextPos, enemy->e->pos);

            if (Vector2Length(toTarget) < 2.0f) {
                enemy->currentStep++;
            } else {
                Vector2 dir = Vector2Normalize(toTarget);
                vel = Vector2Scale(dir, 2.0f);
            }
        } else {
            Vector2 toLkp = Vector2Subtract(enemy->lastKnownPlayerPos, enemy->e->pos);
            if (Vector2Length(toLkp) > 3.0f) {
                Vector2 dir = Vector2Normalize(toLkp);
                vel = Vector2Scale(dir, 1.6f);
            }
        }

        enemy->shootTimer = fmaxf(0.0f, enemy->shootTimer - dt);

        if (HasLOS(enemy->e->pos, player->pos, map) && enemy->shootTimer <= 0.0f){
            // Shoot 
            Vector2 toPlayer = Vector2Subtract(player->pos, enemy->e->pos);
            projectileShoot(projectiles, enemy->e->pos, Vector2Normalize(toPlayer), 4);
            enemy->shootTimer = enemy->shootCooldown;
        }

    }

    // --- IDLE: wander ---
    if (enemy->state == IDLE) {
        if (enemy->idleTimer <= 0) {
            if (enemy->movingIdle) {
                enemy->movingIdle = false;
                enemy->idleTimer = GetRandomValue(30, 90) / 60.0f;
            } else {
                enemy->movingIdle = true;
                enemy->idleTimer = GetRandomValue(60, 180) / 60.0f;

                float randomAngle = GetRandomValue(0, 360) * DEG2RAD;
                float dist = GetRandomValue(40, 120);
                Vector2 candidate = Vector2Add(enemy->e->pos,
                                     (Vector2){cosf(randomAngle)*dist, sinf(randomAngle)*dist});

                rect r = mapGetRecAt(map,
                    (int)(candidate.x / TILE_SIZE), (int)(candidate.y / TILE_SIZE));
                if (!r || !r->node->isWalkable) {
                    enemy->movingIdle = false;
                    enemy->idleTimer = GetRandomValue(30, 90) / 60.0f;
                } else {
                    enemy->idleTarget = candidate;
                }
            }
        }

        enemy->idleTimer -= dt;

        if (enemy->movingIdle) {
            Vector2 toTarget = Vector2Subtract(enemy->idleTarget, enemy->e->pos);
            if (Vector2Length(toTarget) < 2.0f) {
                enemy->movingIdle = false;
                enemy->idleTimer = GetRandomValue(30, 90) / 60.0f;
            } else {
                Vector2 dir = Vector2Normalize(toTarget);
                vel = Vector2Scale(dir, 1.0f);
            }
        }
    }

    // --- Facing update ---
    if (vel.x > 0.1f)  enemy->facingRight = 1;
    if (vel.x < -0.1f) enemy->facingRight = -1;

    if (Vector2Length(vel) >= 0.1f){
        enemy->running = 1; 
    }
    else{
        enemy->running = 0;
    }

    updateAngleSmooth(enemy, vel, 3.5f);
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

    enemy->repathCooldown  = 0.0f;
    enemy->repathInterval  = 0.25f;        // solve at most ~4x/sec (tweak)
    enemy->lastGoalTileX   = INT_MIN;
    enemy->lastGoalTileY   = INT_MIN;
    enemy->senseCooldown   = 0.0f;         // throttle vision checks
    enemy->playerVisible   = false;
    enemy->lastKnownPlayerPos = enemy->e->pos;
    enemy->staggerSlot     = GetRandomValue(0, 2); // spread work across ~3 frames

    enemy->health = 100; 
    enemy->maxHealth = 100;

    enemy->currentFrame = 0;
    enemy->animTimer = 0;
    enemy->facingRight = 1; 
    enemy->running = 0; 

    // enemy->projectiles = create_dynarray(&projectileFree, NULL);
    enemy->shootCooldown = GetRandomValue(40, 60) / 60.0f; 
    enemy->shootTimer = 0.0f; 

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




void enemyDraw(Enemy e, entity player, hash map, Animation *enemyAnimations, Texture2D gunTex){
    // Draw enemy
    // DrawRectangleRec(e->e->rect, RED);
    Texture2D frame = enemyAnimations[e->running]->frames[e->currentFrame];
    Rectangle src = (Rectangle) { 0, 0, (float)frame.width * e->facingRight, (float)frame.height };
    Rectangle dst = (Rectangle) { e->e->rect.x, e->e->rect.y, (float)frame.width, (float)frame.height };
    Vector2 origin = { 0, 0 };

    DrawTexturePro(frame, src, dst, origin, 0.0f, WHITE);

    // Draw gun
    Rectangle gunSrc = { 0, 0, (float)gunTex.width * e->facingRight, (float)gunTex.height };
    Rectangle gunDst = {
        e->e->rect.x + e->e->rect.width / 2,  // X
        e->e->rect.y + e->e->rect.height / 2, // Y
        (float)gunTex.width,
        (float)gunTex.height
    };
    Vector2 gunOrigin = { 9, gunTex.height / 2.0f };
    if (e->facingRight != 1){
        gunOrigin = (Vector2) {30, gunTex.height / 2.0f};
    }
    Vector2 toPlayer = Vector2Subtract(player->pos, e->e->pos);
    float aimAngle = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG; 
    if (e->facingRight != 1){
        aimAngle = aimAngle + 180.0f;
    }
    if (e->state == ACTIVE){
        DrawTexturePro(gunTex, gunSrc, gunDst, gunOrigin, aimAngle, WHITE);
    }
    else{
        DrawTexturePro(gunTex, gunSrc, gunDst, gunOrigin, 0.0f, WHITE);
    }
    // DrawTexturePro(gunTex, gunSrc, gunDst, gunOrigin, aimAngle, WHITE);
    // DrawTexturePro(frame, src, dst, origin, 0, WHITE);
    // DrawTexture(frame, e->e->rect.x, e->e->rect.y, WHITE);


    // Torch effect
    BeginBlendMode(BLEND_ADDITIVE);
    enemyDrawTorch(e, map, 10, ColorAlpha(WHITE, 0.2f));
    EndBlendMode();

    // --- Health bar ---
    float barWidth = e->e->rect.width;
    float barHeight = 5;
    float healthPercent = (float)e->health / (float)e->maxHealth;

    Rectangle healthBarBack = {
        e->e->rect.x,
        e->e->rect.y - barHeight - 2, // slightly above enemy
        barWidth,
        barHeight
    };
    Rectangle healthBarFront = {
        e->e->rect.x,
        e->e->rect.y - barHeight - 2,
        barWidth * healthPercent,
        barHeight
    };

    DrawRectangleRec(healthBarBack, DARKGRAY);
    DrawRectangleRec(healthBarFront, GREEN);
    DrawRectangleLinesEx(healthBarBack, 1, BLACK); // outline
}
