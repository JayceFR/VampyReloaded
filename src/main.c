#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "hash.h"
#include "dynarray.h"
#include "map.h"
#include "physics.h"
#include "enemy.h"
#include "camera.h"
// #include <math.h>

#define MAX_BOIDS 100
#define SCREEN_WIDTH 800 
#define SCREEN_HEIGHT 450 

#define GRID_SIZE 20

#define MAX_FORCE 0.4
#define MAX_SPEED 6

typedef struct {
    Vector2 basePos;     // Center of joystick base
    float baseRadius;
    Vector2 thumbPos;    // Position of joystick thumb
    float thumbRadius;
    Vector2 value;       // Normalized [-1,1] x/y output
    bool active;         // Is joystick currently touched?
    float offvalue;
} Joystick;

struct boid{
    Vector2 pos; 
    int cellX;
    int cellY;
    Vector2 velocity; 
    Vector2 acceleration; 
    bool broken; 
    float moveTimer;
    bool isMoving; 
};
typedef struct boid *boid;

Joystick CreateJoystick(Vector2 basePos, float baseRadius) {
    Joystick joy = {0};
    joy.basePos = basePos;
    joy.baseRadius = baseRadius;
    joy.thumbRadius = baseRadius / 2.5f;
    joy.thumbPos = basePos;
    joy.value = (Vector2){0, 0};
    joy.active = false;
    joy.offvalue = 0.0; 
    return joy;
}

void UpdateJoystick(Joystick *joy) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) { // On Android, mouse==touch
        Vector2 touch = GetMousePosition();
        float dist = Vector2Distance(joy->basePos, touch);

        if (!joy->active && dist <= joy->baseRadius) {
            joy->active = true; // Started touch inside joystick
        }

        if (joy->active) {
            Vector2 offset = Vector2Subtract(touch, joy->basePos);
            float len = Vector2Length(offset);

            if (len > joy->baseRadius) {
                offset = Vector2Scale(Vector2Normalize(offset), joy->baseRadius);
            }

            joy->offvalue = Vector2Length(offset); 

            joy->thumbPos = Vector2Add(joy->basePos, offset);
            joy->value = (Vector2){ offset.x / joy->baseRadius, offset.y / joy->baseRadius };
        }
    } else {
        // Reset when released
        joy->active = false;
        joy->thumbPos = joy->basePos;
        joy->value = (Vector2){0, 0};
        joy->offvalue = 0.0;
    }
}

void DrawJoystick(Joystick joy) {
    DrawCircleV(joy.basePos, joy.baseRadius, Fade(DARKGRAY, 0.5f));
    DrawCircleV(joy.thumbPos, joy.thumbRadius, Fade(LIGHTGRAY, 0.8f));
}

float randRange(float min, float max) {
    return min + ((float) GetRandomValue(0, 10000) / 10000.0f) * (max - min);
}


void updateForEachBoid(hashkey k, hashvalue v, void * arg){
    char *key = (char *) k; 
    dynarray arr = (dynarray) v; 
    dynarray toChange = (dynarray) arg; 
    for (int i = 0; i < arr->len; i++){
        boid b = arr->data[i];
        
        // b->acceleration.x = averageVels[i].x;
        // b->acceleration.y = averageVels[i].y;

        Vector2 offvel = Vector2Add(b->velocity, b->acceleration);
        b->velocity.x = offvel.x;
        b->velocity.y = offvel.y; 

        if (Vector2Length(b->velocity) > MAX_SPEED){
            Vector2 normalised = Vector2Normalize(b->velocity);
            b->velocity.x = normalised.x * MAX_SPEED;
            b->velocity.y = normalised.y * MAX_SPEED;
        }
        
        Vector2 off = Vector2Add(b->pos, b->velocity);
        b->pos.x = off.x;
        b->pos.y = off.y;

        int newCX = ((int) b->pos.x) / GRID_SIZE;
        int newCY = ((int) b->pos.y) / GRID_SIZE;

        if (newCX != b->cellX || newCY != b->cellY){
            // Need to update 
            b->broken = true;
            add_dynarray(toChange, strdup(key));
        }

    }
}

void printToChange(FILE *out, DA_ELEMENT el, int n){
    char *arg = (char *) el; 
    fprintf(out, "key : %s", arg);
}

void freeToChange(DA_ELEMENT el){
    char *arg = (char *) el; 
    free(arg);
}

void updateBoids(hash flockGrid, Vector2 *averageVels){
    dynarray toChange = create_dynarray(&freeToChange, &printToChange);
    hashForeach(flockGrid, &updateForEachBoid, toChange);

    for (int i = 0; i < toChange->len; i++){
        char *key = toChange->data[i];
        dynarray arr = hashFind(flockGrid, key);

        int j = 0;
        while (j < arr->len){
            boid old = arr->data[j];
            if (!old->broken){
                j += 1;
                continue; 
            }
            // delete the boid from the list
            remove_dynarray(arr, j);
            // update the boid 
            int newCX = ((int) old->pos.x) / GRID_SIZE;
            int newCY = ((int) old->pos.y) / GRID_SIZE;
            old->cellX = newCX;
            old->cellY = newCY;
            old->broken = false;

            // insert the new boid into the hashmap
            char buffer[25]; 
            
            dynarray array;

            sprintf(buffer, "%d:%d", old->cellX, old->cellY);
            if ( (array = hashFind(flockGrid, buffer)) != NULL){
                // Then append it to the list 
                add_dynarray(array, old);
            }
            else{
                array = create_dynarray(NULL, NULL);
                add_dynarray(array, old);
                hashSet(flockGrid, buffer, array);
            }

        }
    }

    free_dynarray(toChange);
}

Vector2 ClampMagnitude(Vector2 v, float maxLength) {
    float len = Vector2Length(v);
    if (len > maxLength) {
        v = Vector2Scale(Vector2Normalize(v), maxLength);
    }
    return v;
}

struct steeringData{
    Vector2 playerPos; 
    hash flockGrid;
};
typedef struct steeringData *steeringData; 

#define ALIGN_WEIGHT 1.0f
#define COHESION_WEIGHT 1.0f
#define SEPARATION_WEIGHT 1.45f

void calculateSteeringForEach(hashkey k, hashvalue v, void *arg) {
    dynarray arr = (dynarray)v; 
    steeringData data = (steeringData)arg; 
    if (arr->len == 0) return; 
    
    char buffer[25];

    for (int i = 0; i < arr->len; i++) {
        boid boi = arr->data[i];

        Vector2 avgVel = {0, 0}, avgPos = {0, 0}, avgSep = {0, 0};
        int total = 0;

        for (int x = boi->cellX - 1; x <= boi->cellX + 1; x++) {
            for (int y = boi->cellY - 1; y <= boi->cellY + 1; y++) {
                sprintf(buffer, "%d:%d", x, y);
                dynarray array = hashFind(data->flockGrid, buffer);
                if (!array) continue;

                for (int z = 0; z < array->len; z++) {
                    if (x == boi->cellX && y == boi->cellY && z == i) continue; 
                    boid b = array->data[z];
                    total++;
                    avgVel = Vector2Add(avgVel, b->velocity);
                    avgPos = Vector2Add(avgPos, b->pos);

                    float d = Vector2Distance(boi->pos, b->pos);
                    if (d > 0.001f) {
                        Vector2 diff = Vector2Subtract(boi->pos, b->pos);
                        diff = Vector2Scale(diff, 1.0f / (d * d));
                        avgSep = Vector2Add(avgSep, diff);
                    }
                }
            }
        }

        Vector2 steering = {0, 0};
        if (total > 0) {
            // Alignment
            avgVel = Vector2Scale(avgVel, 1.0f / total);
            if (Vector2Length(avgVel) > 0.001f) {
                avgVel = Vector2Normalize(avgVel);
                avgVel = Vector2Scale(avgVel, MAX_SPEED);
            }
            Vector2 alignForce = ClampMagnitude(Vector2Subtract(avgVel, boi->velocity), MAX_FORCE);

            // Cohesion
            avgPos = Vector2Scale(avgPos, 1.0f / total);
            Vector2 cohVector = Vector2Subtract(avgPos, boi->pos);
            if (Vector2Length(cohVector) > 0.001f) {
                cohVector = Vector2Normalize(cohVector);
                cohVector = Vector2Scale(cohVector, MAX_SPEED);
            }
            cohVector = ClampMagnitude(Vector2Subtract(cohVector, boi->velocity), MAX_FORCE);

            // Separation
            avgSep = Vector2Scale(avgSep, 1.0f / total);
            Vector2 sepVector = avgSep;
            if (Vector2Length(sepVector) > 0.001f) {
                sepVector = Vector2Normalize(sepVector);
                sepVector = Vector2Scale(sepVector, MAX_SPEED);
            }
            sepVector = ClampMagnitude(Vector2Subtract(sepVector, boi->velocity), MAX_FORCE);

            steering = Vector2Add(
                Vector2Scale(sepVector, SEPARATION_WEIGHT),
                Vector2Add(Vector2Scale(alignForce, ALIGN_WEIGHT),
                           Vector2Scale(cohVector, COHESION_WEIGHT))
            );
        }

        Vector2 goal = (Vector2) {SCREEN_WIDTH / 2, SCREEN_HEIGHT/ 2};
        Vector2 toGoal = Vector2Subtract(goal, boi->pos);
        toGoal = Vector2Scale(Vector2Normalize(toGoal), 0.3f);

        boi->acceleration = Vector2Add(steering, toGoal);
        
    }
}




void calculateSteering(hash flock ,steeringData data) {
    hashForeach(flock, &calculateSteeringForEach, data);
}

void drawForEachBoid(hashkey k, hashvalue v, void * arg){
    dynarray arr = (dynarray) v; 
    for (int i = 0; i < arr->len; i++){
        boid b = arr->data[i];
        DrawCircleV(
            (Vector2) {b->pos.x, b->pos.y},
            5,
            BLUE
        );
    }
}

void DrawBoids(hash flockGrid){
    hashForeach(flockGrid, &drawForEachBoid, NULL);
}

float randFloat(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}


Vector2 randomVelocity(float minSpeed, float maxSpeed) {
    float angle = ((float)randFloat(0, 360)) * (PI / 180.0f);
    float speed = randRange(minSpeed, maxSpeed);
    return (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
}


// Vector2 computeVelOfEnemy(entity enemy, entity player, hash map) {

//     dynarray path = pathFinding(player->pos, enemy->pos, map);
//     if (path != NULL){
//         if (path->len > 0){
//             printf("We found a path\n");
//             pathNode lastNode = path->data[path->len - 1];
//             Vector2 lastPos = {lastNode->x, lastNode->y};
//             Vector2 dir = Vector2Normalize(Vector2Subtract(lastPos, enemy->pos));
//             return Vector2Scale(dir, 2.0f);
//         }
//     }

//     return (Vector2) {0, 0};

    // Vector2 dir = Vector2Normalize(Vector2Subtract(player->pos, enemy->pos));
    // Vector2 step = Vector2Scale(dir, 2.0f); 
    // Vector2 ray = enemy->pos;
    // float maxDist = Vector2Distance(enemy->pos, player->pos);

    // dynarray rects = rectsAround(map, enemy->pos);

    // float distTravelled = 0;
    // bool blocked = false;

    // while (distTravelled < maxDist) {
    //     // hit player
    //     if (CheckCollisionPointRec(ray, player->rect)) break;

    //     // hit a wall
    //     for (int i = 0; i < rects->len; i++) {
    //         rect r = rects->data[i];
    //         if (CheckCollisionPointRec(ray, r->rectange)) {
    //             blocked = true;
    //             break;
    //         }
    //     }
    //     if (blocked) break;

    //     // step forward
    //     ray = Vector2Add(ray, step);
    //     distTravelled += Vector2Length(step);
    // }

    // free_dynarray(rects);

    // if (blocked) {
    //     return (Vector2){0, 0}; // LoS blocked
    // } else {
    //     return Vector2Scale(dir, 2.0f); // chase
    // }
// }

// float Lerp(float a, float b, float t) {
//     return a + (b - a) * t;
// }


int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vampy Reloaded");
    SetTargetFPS(60);

    Joystick joy = CreateJoystick((Vector2){100, 350}, 60);
    Joystick aim = CreateJoystick((Vector2){300, 350}, 60);

    hash flockGrid = hashCreate(NULL, &free_dynarray, NULL); 

    entity player = entityCreate(400, 225, 15, 15);
    Vector2 offset = {0, 0};

    steeringData data = malloc(sizeof(struct steeringData));
    data->flockGrid = flockGrid;
    data->playerPos = player->pos;

    Camera2D camera = {0};
    camera.target = player->pos;
    camera.offset = (Vector2) {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.rotation = 0.0f; 
    camera.zoom = 1.0f; 

    for (int i = 0; i < MAX_BOIDS; i++){

        boid b = malloc(sizeof(struct boid));
        assert(b != NULL);

        
        b->pos = (Vector2) {GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT)};
        b->cellX = (int) b->pos.x / GRID_SIZE; 
        b->cellY = (int) b->pos.y / GRID_SIZE;
        b->velocity = randomVelocity(-2,2);
        b->acceleration = (Vector2) {0,0};
        b->isMoving = true;
        b->moveTimer = randFloat(0.5, 1);


        char buffer[25];

        dynarray arr; 

        sprintf(buffer, "%d:%d", b->cellX, b->cellY);
        printf("bpos : %f : %f", b->pos.x, b->pos.y);
        printf("Buffer when inititing : %s",buffer);
        if ( (arr = hashFind(flockGrid, buffer)) != NULL){
            // Then append it to the list 
            add_dynarray(arr, b);
        }
        else{
            arr = create_dynarray(NULL, NULL);
            add_dynarray(arr, b);
            hashSet(flockGrid, buffer, arr);
        }

    }

    Vector2 averageVels[MAX_BOIDS];

    Vector2 swarmTarget = player->pos;
    float timeSinceUpdate = 0.0f;
    float updateInterval = 0.0f; // seconds

    mapData mData = mapCreate();
    hash map = mData.map;

    Image noise = GenImagePerlinNoise(256, 256, 50, 50, 0.4f);

    Enemy enemy = enemyCreate(50, 60, 15, 15);
    
    srand(time(NULL));

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();
        UpdateJoystick(&joy);

        char buffer[22];
        sprintf(buffer, "fps : %d", GetFPS());

        offset.x = 0;
        offset.y = 0;

        offset.x += joy.value.x * 5;
        offset.y += joy.value.y * 5;

        update(player, map, offset);

        update(enemy->e, map, computeVelOfEnemy(enemy, player, map)); 

        // Update swarm target only every few seconds
        timeSinceUpdate += delta;
        if (timeSinceUpdate >= updateInterval) {
            timeSinceUpdate = 0.0f;

            // Random offset from player's current position
            // swarmTarget = Vector2Add(playerPos, 
            //     (Vector2){ GetRandomValue(-50, 50), GetRandomValue(-50, 50) });
            swarmTarget = player->pos;
        }

        // calculateSteering(flock, averageVels, swarmTarget);
        data->playerPos = player->pos;
        calculateSteering(flockGrid, data);
        updateBoids(flockGrid, averageVels);

        // float followSpeed = 4.0f; 
        // Vector2 diff = {
        //     player->pos.x - camera.target.x,
        //     player->pos.y - camera.target.y
        // };

        // camera.target.x += diff.x * followSpeed * GetFrameTime();
        // camera.target.y += diff.y * followSpeed * GetFrameTime(); 

        UpdateCameraRoom(&camera, player);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        mapDraw(map, player->pos);

        Door door = getPlayerRoomDoor(mData.doors, player->pos);
        if (door != NULL){
            DrawRectangle((int)door->pos.x,(int) door->pos.y, 15, 15, GREEN);   
        }

        // for (int i = 0; i < mData.doors->len; i++){
        //     Door door = mData.doors->data[i];
        //     if (player->pos.x > door->ax && player->pos.x < door->bx && player->pos.y > door->ay && player->pos.y < door->by){
        //         DrawRectangle((int)door->pos.x,(int) door->pos.y, 15, 15, GREEN);
        //     }
        // }

        // DrawCircleV(playerPos, 20, RED);
        DrawRectangleRec(player->rect, BLUE);
        DrawRectangleRec(enemy->e->rect, RED);
        // DrawCircleV(swarmTarget, 5, GREEN); // visualize swarm target
        // DrawBoids(flockGrid);

        EndMode2D();


        DrawText(buffer, 10, 10, 10, RED);

        DrawJoystick(joy);

        EndDrawing();
    }

    mapFree(map);

    CloseWindow();
    return 0;
}
