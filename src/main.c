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
#include "projectile.h"
#include "impact.h"
#include "utils.h"
// #include <math.h>

#define MAX_BOIDS 100
#define SCREEN_WIDTH 400 
#define SCREEN_HEIGHT 225 

#define GRID_SIZE 20

#define MAX_FORCE 0.4
#define MAX_SPEED 6

typedef enum {
    JOY_IDLE,
    JOY_AIMING,
    JOY_SHOOTING
} JoystickState;

// typedef struct {
//     Vector2 basePos;     // Center of joystick base
//     float baseRadius;
//     Vector2 thumbPos;    // Position of joystick thumb
//     float thumbRadius;
//     Vector2 value;       // Normalized [-1,1] x/y output
//     bool active;         // Is joystick currently touched?
//     float offvalue;      // Distance from center (0..baseRadius)
//     JoystickState state; // Idle / aiming / shooting
// } Joystick;


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

typedef struct {
    Vector2 basePos;
    float baseRadius;
    Vector2 thumbPos;
    float thumbRadius;
    Vector2 value;
    bool active;
    float offvalue;
    JoystickState state;
    int pointerId;      // <- NEW: which finger is bound (-1 == none)
} Joystick;

Joystick CreateJoystick(Vector2 basePos, float baseRadius) {
    Joystick joy = (Joystick){0};
    joy.basePos = basePos;
    joy.baseRadius = baseRadius;
    joy.thumbRadius = baseRadius / 2.5f;
    joy.thumbPos = basePos;
    joy.value = (Vector2){0,0};
    joy.active = false;
    joy.offvalue = 0.0f;
    joy.state = JOY_IDLE;
    joy.pointerId = -1;                 // <- NEW
    return joy;
}


static inline void ResetJoystick(Joystick *j){
    j->active = false;
    j->thumbPos = j->basePos;
    j->value = (Vector2){0,0};
    j->offvalue = 0.0f;
    j->state = JOY_IDLE;
    j->pointerId = -1;
}

static inline int FindTouchIndexById(int id){
    int n = GetTouchPointCount();
    for (int i = 0; i < n; i++){
        if (GetTouchPointId(i) == id) return i;
    }
    return -1;
}

static inline void ApplyTouchToJoystick(Joystick *j, Vector2 touch){
    Vector2 offset = Vector2Subtract(touch, j->basePos);
    float len = Vector2Length(offset);
    if (len > j->baseRadius){
        offset = Vector2Scale(Vector2Normalize(offset), j->baseRadius);
        len = j->baseRadius;
    }
    j->offvalue = len;
    j->thumbPos = Vector2Add(j->basePos, offset);
    j->value = (Vector2){ offset.x / j->baseRadius, offset.y / j->baseRadius };

    float norm = j->offvalue / j->baseRadius;
    if (norm > 0.7f) j->state = JOY_SHOOTING;
    else if (norm > 0.2f) j->state = JOY_AIMING;
    else j->state = JOY_IDLE;

    j->active = true;
}


void UpdateJoystick(Joystick *joy) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) { // On Android, mouse==touch
        Vector2 touch = GetMousePosition();
        // touch.x /= 2.0f;
        // touch.y /= 2.0f;

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

            // --- AIM vs SHOOT ---
            float normDist = joy->offvalue / joy->baseRadius;
            if (normDist > 0.7f) {
                joy->state = JOY_SHOOTING;
            } else if (normDist > 0.2f) {
                joy->state = JOY_AIMING;
            } else {
                joy->state = JOY_IDLE;
            }
        }
    } else {
        // Reset when released
        joy->active = false;
        joy->thumbPos = joy->basePos;
        joy->value = (Vector2){0, 0};
        joy->offvalue = 0.0;
        joy->state = JOY_IDLE;
    }
}

void UpdateJoysticks(Joystick *left, Joystick *right){
    int touchCount = GetTouchPointCount();

    // 1) Drop bindings that lifted this frame
    if (left->pointerId  != -1 && FindTouchIndexById(left->pointerId)  == -1)  ResetJoystick(left);
    if (right->pointerId != -1 && FindTouchIndexById(right->pointerId) == -1) ResetJoystick(right);

    // 2) Update bound joysticks with their finger
    if (left->pointerId != -1){
        int li = FindTouchIndexById(left->pointerId);
        if (li != -1) ApplyTouchToJoystick(left, GetTouchPosition(li));
    }
    if (right->pointerId != -1){
        int ri = FindTouchIndexById(right->pointerId);
        if (ri != -1) ApplyTouchToJoystick(right, GetTouchPosition(ri));
    }

    // 3) Assign free touches to free joysticks (no double-binding)
    for (int i = 0; i < touchCount; i++){
        int id = GetTouchPointId(i);
        Vector2 p = GetTouchPosition(i);
        // p.x /= 2.0f;
        // p.y /= 2.0f;

        if (id == left->pointerId || id == right->pointerId) continue;

        if (left->pointerId == -1 && Vector2Distance(left->basePos, p) <= left->baseRadius){
            left->pointerId = id;
            ApplyTouchToJoystick(left, p);
            continue;
        }
        if (right->pointerId == -1 && Vector2Distance(right->basePos, p) <= right->baseRadius){
            right->pointerId = id;
            ApplyTouchToJoystick(right, p);
            continue;
        }
    }

    // 4) If still unbound, keep them centered
    if (left->pointerId  == -1)  ResetJoystick(left);
    if (right->pointerId == -1) ResetJoystick(right);

#if defined(PLATFORM_DESKTOP)
    // Desktop fallback for debugging: WASD -> left stick, mouse (LMB) -> right stick
    if (touchCount == 0){
        Vector2 mv = {0,0};
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  mv.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) mv.x += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    mv.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  mv.y += 1.0f;

        if (Vector2Length(mv) > 0.0f){
            mv = Vector2Normalize(mv);
            left->active = true;
            left->thumbPos = Vector2Add(left->basePos, Vector2Scale(mv, left->baseRadius));
            left->value = mv;
            left->offvalue = left->baseRadius * 0.8f;
            left->state = JOY_AIMING;
        } else {
            ResetJoystick(left);
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
            ApplyTouchToJoystick(right, GetMousePosition());
        } else {
            ResetJoystick(right);
        }
    }
#endif
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

int main() {
    InitWindow(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, "Vampy Reloaded (x2 scaled)");
    SetTargetFPS(60);

    // 🎯 Offscreen render target at original resolution
    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Loading files 
    Animation player_idle = loadAnimation("entities/player/", 4);

    Joystick joy = CreateJoystick((Vector2){100, 350}, 60);
    Joystick aim = CreateJoystick((Vector2){700, 350}, 60);

    hash flockGrid = hashCreate(NULL, &free_dynarray, NULL); 
    entity player = entityCreate(400, 225, 15, 15);
    Vector2 offset = {0, 0};

    steeringData data = malloc(sizeof(struct steeringData));
    data->flockGrid = flockGrid;
    data->playerPos = player->pos;

    Camera2D camera = {0};
    camera.target = player->pos;
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.rotation = 0.0f; 
    camera.zoom = 1.0f; 

    dynarray projectiles = create_dynarray(&projectileFree,NULL);

    // 🐦 Init boids (same as before)
    for (int i = 0; i < MAX_BOIDS; i++) {
        boid b = malloc(sizeof(struct boid));
        assert(b != NULL);

        b->pos = (Vector2){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT)};
        b->cellX = (int)b->pos.x / GRID_SIZE; 
        b->cellY = (int)b->pos.y / GRID_SIZE;
        b->velocity = randomVelocity(-2, 2);
        b->acceleration = (Vector2){0, 0};
        b->isMoving = true;
        b->moveTimer = randFloat(0.5, 1);

        char buffer[25];
        dynarray arr; 
        sprintf(buffer, "%d:%d", b->cellX, b->cellY);
        if ((arr = hashFind(flockGrid, buffer)) != NULL) {
            add_dynarray(arr, b);
        } else {
            arr = create_dynarray(NULL, NULL);
            add_dynarray(arr, b);
            hashSet(flockGrid, buffer, arr);
        }
    }

    Vector2 averageVels[MAX_BOIDS];
    Vector2 swarmTarget = player->pos;

    mapData mData = mapCreate();
    hash map = mData.map;

    Enemy enemy = enemyCreate(50, 60, 15, 15);

    char enemyKey[22];
    dynarray enemies; 
    srand(time(NULL));

    float shootCooldown = 0.0f; 
    int currentFrame = 0;
    float frameTime = 0.1f;
    float timer = 0;

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        int roomX = player->pos.x / ROOM_SIZE;
        int roomY = player->pos.y / ROOM_SIZE;

        // Animation frame timer
        timer += delta;
        if (timer > frameTime) {
            timer = 0;
            currentFrame = (currentFrame + 1) % player_idle->numberOfFrames;
        }

        UpdateJoysticks(&joy, &aim);

        shootCooldown = fmaxf(0.0f, shootCooldown - delta);

        offset = (Vector2){ joy.value.x * 5, joy.value.y * 5 };

        if (aim.state == JOY_SHOOTING && shootCooldown <= 0.0f) {
            projectileShoot(projectiles, player->pos, aim.value, 10.0f);
            shootCooldown = 40.0f / 60.0f;
            offset.x -= (aim.value.x * 3); 
            offset.y -= (aim.value.y * 3);  
            Impact_StartShake(0.4f, 8.0f);
        }

        update(player, map, offset);
        UpdateCameraRoom(&camera, player);

        Impact_UpdateShake(&camera, delta);
        Impact_UpdateParticles(delta);

        sprintf(enemyKey, "%d:%d", roomX, roomY);

        MapEnsureCache(map, camera);

        // === 🎯 Draw to render texture at base resolution ===
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            BeginMode2D(camera);
                // mapDraw(map, camera);
                MapDrawCached(camera);
                
                if ((enemies = hashFind(mData.enemies, enemyKey)) != NULL) {
                    for (int i = 0; i < enemies->len; i++) {
                        Enemy e = enemies->data[i];
                        Vector2 vel = computeVelOfEnemy(e, player, map);
                        update(e->e, map, vel);
                        enemyDraw(e, map);
                    }
                }

                DrawTexture(player_idle->frames[currentFrame], player->rect.x, player->rect.y, WHITE);

                int pos = 0;
                while (pos < projectiles->len) {
                    projectile p = projectiles->data[pos];

                    if (projectileUpdate(p, map)) {
                        remove_dynarray(projectiles, pos);
                        continue;
                    }
                    projectileDraw(p);
                    pos++;
                }

                Impact_DrawParticles();
            EndMode2D();

            DrawText(TextFormat("fps: %d", GetFPS()), 10, 10, 10, RED);
        EndTextureMode();

        // === 🎯 Draw scaled texture to screen ===
        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle src = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
            Rectangle dst = { 0, 0, (float)SCREEN_WIDTH * 2, (float)SCREEN_HEIGHT * 2 };
            DrawTexturePro(target.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

            DrawJoystick(joy);
            DrawJoystick(aim);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    mapFree(map);
    CloseWindow();
    return 0;
}

