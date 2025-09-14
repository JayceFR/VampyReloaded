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
#include "gun.h"
#include "computer.h"
#include "npc.h"
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

typedef enum{
    P_IDLE, 
    P_RUN,
} PlayerState; 

// free offgrids 
void offgridsFree(hashvalue val){
    dynarray o = (dynarray) val; 
    free_dynarray(o);
}

int main() {
    InitWindow(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, "Vampy Reloaded (x2 scaled)");
    SetTargetFPS(60);

    // 🎯 Offscreen render target at original resolution
    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Loading files 
    Animation PlayerAnimations[] = {
        loadAnimation("entities/player/idle/", 4),
        loadAnimation("entities/player/run/", 4),
    };
    Animation EnemyAnimations[] = {
        loadAnimation("entities/enemy/idle/", 4),
        loadAnimation("entities/enemy/run/", 4),
    };

    Animation CowAnimations[] = {
        loadAnimation("entities/npcs/cow/idle/", 4),
        loadAnimation("entities/npcs/cow/run/", 4),
    };

    Animation CitizenAnimations[] = {
        loadAnimation("entities/npcs/citizen/idle/", 4),
        loadAnimation("entities/npcs/citizen/run/", 4),
    };

    Animation *NPCAnimations[] = {
        CitizenAnimations,
        CowAnimations,
    };

    int NO_OF_BIOMES = 3; 
    int NO_OF_FOREST_TEXS = 2;
    int NO_OF_TOWN_TEXS = 9;
    int NO_OF_VILLAGE_TEXS = 14;
    Texture2D *forestTexs = loadTexturesFromDirectory("tiles/offgrid/forest/", NO_OF_FOREST_TEXS);
    Texture2D *townTexs = loadTexturesFromDirectory("tiles/offgrid/town/", NO_OF_TOWN_TEXS);
    Texture2D *villageTexs = loadTexturesFromDirectory("tiles/offgrid/village/", NO_OF_VILLAGE_TEXS);
    Texture2D *gunTexs = loadTexturesFromDirectory("entities/guns/", 4);

    BIOME_DATA biome_data = malloc(sizeof(struct BIOME_DATA)); 
    biome_data->texs = malloc(sizeof(Texture2D *) * NO_OF_BIOMES);
    biome_data->texs[FOREST] = forestTexs;
    biome_data->texs[TOWN] = townTexs;
    biome_data->texs[VILLAGE] = villageTexs;
    biome_data->size_of_texs = malloc(sizeof(int) * NO_OF_BIOMES);
    biome_data->size_of_texs[FOREST] = NO_OF_FOREST_TEXS;
    biome_data->size_of_texs[TOWN] = NO_OF_TOWN_TEXS;
    biome_data->size_of_texs[VILLAGE] = NO_OF_VILLAGE_TEXS;

    // printf("%d", biome_data->texs[TOWN][0].height);

    loadDirectory();
    // Texture2D gunTex = LoadTexture("entities/enemy/gun.png");
    Texture2D tiles[] = {
        LoadTexture("tiles/dirt.png"),
        LoadTexture("tiles/stone.png")
    };
    // Tile Types 
    // [bottom_left, bottom_right, bottom, left, middle, right, top_left, top_right, top, bottom-left-1, bottom-right-1, bottom-1]
    Texture2D stoneTiles[] = {
        LoadTexture("tiles/stone2/bottom-left.png"),
        LoadTexture("tiles/stone2/bottom-right.png"),
        LoadTexture("tiles/stone2/bottom.png"),
        LoadTexture("tiles/stone2/left.png"),
        LoadTexture("tiles/stone2/middle.png"),
        LoadTexture("tiles/stone2/right.png"),
        LoadTexture("tiles/stone2/top-left.png"),
        LoadTexture("tiles/stone2/top-right.png"),
        LoadTexture("tiles/stone2/top.png"),
        LoadTexture("tiles/stone2/bottom-left-1.png"),
        LoadTexture("tiles/stone2/bottom-right-1.png"),
        LoadTexture("tiles/stone2/bottom-1.png"),
    };

    Texture2D dirtTiles[] = {
        LoadTexture("tiles/dirt2/1.png"),
        LoadTexture("tiles/dirt2/2.png"),
        LoadTexture("tiles/dirt2/3.png"),
        LoadTexture("tiles/dirt2/4.png"),
    };

    Texture2D pathDirt = LoadTexture("tiles/dirt/1.png");
    Texture2D enemyGunTex = LoadTexture("entities/enemy/pistol.png");
    Texture2D computerTex = LoadTexture("entities/computer/computer.png");
    closeDirectory();

    Joystick joy = CreateJoystick((Vector2){100, 350}, 60);
    Joystick aim = CreateJoystick((Vector2){700, 350}, 60);

    hash flockGrid = hashCreate(NULL, &free_dynarray, NULL); 
    entity player = entityCreate(400, 225, 15, 15);
    PlayerState pState = P_IDLE;
    int facingRight = 1; 
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
    dynarray eprojectiles = create_dynarray(&projectileFree, NULL);

    hash offgridMap = hashCreate(NULL, &offgridsFree, NULL);
    dynarray offgrids;

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
    Vector2 previousOffset = {0.0f, 0.0f};

    mapData mData = mapCreate(offgridMap, biome_data, pathDirt);
    hash map = mData.map;

    player->pos = mapFindSpawnTopLeft(map);

    Enemy enemy = enemyCreate(50, 60, 15, 15);

    char enemyKey[22];
    dynarray enemies; 
    srand(time(NULL));

    float shootCooldown = 0.0f; 
    int currentFrame = 0;
    float frameTime = 0.1f;
    float timer = 0;

    Rectangle src;
    Rectangle dst; 

    // Shader stuff
    loadDirectory();
    Shader shader = LoadShader(0, "shaders/atmosphere.fs");
    closeDirectory();
    int timeLoc = GetShaderLocation(shader, "time");
    int itimeLoc = GetShaderLocation(shader, "itime");
    int darknessLoc = GetShaderLocation(shader, "darkness");
    int jekyllLoc = GetShaderLocation(shader, "jekyll");
    int camScrollLoc = GetShaderLocation(shader, "cam_scroll");

    // Bind Noise Texture
    loadDirectory();
    Texture2D noise1 = LoadTexture("misc/pnoise.png");
    Texture2D noise2 = LoadTexture("misc/pnoise2.png");
    closeDirectory();

    // Then bind the textures

    float startTime = GetTime();

    bool playerAlive = true; 
    bool transitioning = false; 
    float transitionRadius = 0.0f; 
    float transitionSpeed = 200.0f; 
    Vector2 transitionCenter; 

    // Guns 
    
    Gun *guns = malloc(sizeof(Gun) * 4);
    // sniper
    guns[0].cooldown = 1.5f;
    guns[0].maxAmmo = 3;
    guns[0].reloadTime = 3.0f;
    guns[0].texture = gunTexs[0];
    guns[0].damage = 100.0f;
    guns[0].speed = 16.0f;
    guns[0].numberOfProjectiles = 1;

    // submachine gun 
    guns[1].cooldown = 0.3f;
    guns[1].maxAmmo = 30;
    guns[1].reloadTime = 2.5f;
    guns[1].texture = gunTexs[1];
    guns[1].damage = 20.0f;
    guns[1].speed = 12.0f;
    guns[1].numberOfProjectiles = 1;
    // Double uzi 
    guns[2].cooldown = 0.2f;
    guns[2].maxAmmo = 20;
    guns[2].reloadTime = 2.0f;
    guns[2].texture = gunTexs[2];
    guns[2].damage = 15.0f;
    guns[2].speed = 13.0f;
    guns[2].numberOfProjectiles = 2;
    // pistol 
    guns[3].cooldown = 40.0f / 60.0f;
    guns[3].maxAmmo = 10;
    guns[3].reloadTime = 2.0f;
    guns[3].texture = gunTexs[3];
    guns[3].damage = 25.0f;
    guns[3].speed = 10.0f;
    guns[3].numberOfProjectiles = 1;

    Gun g = guns[GetRandomValue(0, 3)]; // start with random gun
    int ammo = g.maxAmmo;
    float reloadTimer = 0.0f; 
    bool reloading = false;

    // Computer time 
    hash computers = mData.computers;
    dynarray computer; 
    bool collidingComputer = false;
    Computer currComputer; 
    bool isHacking = false;

    int computersHacked = 0;

    // NPCs
    dynarray npcs; 

    while (!WindowShouldClose()) {
        double t_frame_start = GetTime();

        int roomX = player->pos.x / ROOM_SIZE;
        int roomY = player->pos.y / ROOM_SIZE;

        float delta = GetFrameTime();

        double t_anim_start = GetTime();
        // Animation frame timer
        timer += delta;
        if (timer > frameTime) {
            timer = 0;
            currentFrame = (currentFrame + 1) % 4;
        }
        double t_anim_end = GetTime();

        double t_input_start = GetTime();
        if (isHacking && currComputer){
            currComputer->amountLeftToHack -= GetFrameTime() * 5;
            if (currComputer->amountLeftToHack <= 0) {
                currComputer->hacked = true;
                computersHacked += 1;
                isHacking = false;
                printf("Computer hacked!\n");
            }
        }
        else{
            UpdateJoysticks(&joy, &aim);
        }
        double t_input_end = GetTime();

        double t_logic_start = GetTime();
        // --- Game logic ---
        // (all your logic code here, e.g. update, Impact_UpdateShake, etc.)
        float aimAngle = atan2f(aim.value.y, aim.value.x) * RAD2DEG;
        if (fabsf(aim.value.x) < 0.1f && fabsf(aim.value.y) < 0.1f) {
            aimAngle = facingRight ? 0.0f : 180.0f;
        }
        if (reloading) {
            reloadTimer -= delta;
            if (reloadTimer <= 0.0f){
                ammo = g.maxAmmo;
                reloading = false;
            }
        }
        shootCooldown = fmaxf(0.0f, shootCooldown - delta);
        offset = (Vector2){ joy.value.x * 5, joy.value.y * 5 };
        if (aim.state == JOY_SHOOTING && shootCooldown <= 0.0f && !reloading && ammo > 0) {
            if (g.numberOfProjectiles == 2) {
                // Ensure we have a valid direction
                Vector2 dir = Vector2Normalize(aim.value);
                if (Vector2Length(dir) < 0.001f) {
                    // fallback (use facing direction if no aim)
                    dir = (Vector2){ (facingRight == 1) ? 1.0f : -1.0f, 0.0f };
                }

                // Perp vector (left/right relative to aim)
                Vector2 perp = (Vector2){ -dir.y, dir.x };

                // Tweak these to taste:
                float barrelHalfSeparation = 6.0f;   // half distance between barrels (pixels). increase to see them farther apart.
                float forwardOffset = player->rect.height * 0.6f + 4.0f; // push spawn in front of player so it doesn't immediately hit player

                // Build final spawn positions
                Vector2 muzzleCenter = Vector2Add(player->pos, Vector2Scale(dir, forwardOffset));
                Vector2 spawn1 = Vector2Add(muzzleCenter, Vector2Scale(perp,  barrelHalfSeparation));
                Vector2 spawn2 = Vector2Add(muzzleCenter, Vector2Scale(perp, -barrelHalfSeparation));

                // Shoot both bullets (same direction)
                projectileShoot(projectiles, spawn1, dir, g.speed);
                projectileShoot(projectiles, spawn2, dir, g.speed);

                // --- Optional debug: draw tiny markers for the two spawns ---
                // Draw these temporarily (put them in the draw/update area after shooting)
                // DrawCircleV(spawn1, 2, RED);
                // DrawCircleV(spawn2, 2, BLUE);

            } else {
                projectileShoot(projectiles, player->pos, aim.value, g.speed);
            }


            shootCooldown = g.cooldown;
            ammo--; 
            offset.x -= (aim.value.x * 5); 
            offset.y -= (aim.value.y * 5);  
            Impact_StartShake(0.4f, 8.0f);
            if (ammo <= 0){
                reloading = true;
                reloadTimer = g.reloadTime;
            }
        }
        if (Vector2Length(offset) > 0.1f) {
            pState = P_RUN;
        } else {
            pState = P_IDLE;
        }
        if (Vector2Length(aim.value) > 0.1f) {
            if (aim.value.x > 0.1f) facingRight = 1;
            else if (aim.value.x < -0.1f) facingRight = -1;
        } else {
            if (offset.x > 0.1f) facingRight = 1;
            else if (offset.x < -0.1f) facingRight = -1;
        }
        float t = GetTime() - startTime;
        SetShaderValue(shader, timeLoc, &t, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, itimeLoc, &t, SHADER_UNIFORM_FLOAT);
        float darkness = 0.0f;
        SetShaderValue(shader, darknessLoc, &darkness, SHADER_UNIFORM_FLOAT);
        int jekyllVal = 1;
        SetShaderValue(shader, jekyllLoc, &jekyllVal, SHADER_UNIFORM_INT);
        Vector2 camScroll = camera.target;
        SetShaderValue(shader, camScrollLoc, &camScroll, SHADER_UNIFORM_VEC2);

        update(player, map, offset);
        UpdateCameraRoom(&camera, player);
        Impact_UpdateShake(&camera, delta);
        Impact_UpdateParticles(delta);

        sprintf(enemyKey, "%d:%d", roomX, roomY);
        MapEnsureCache(map, camera, tiles, stoneTiles, dirtTiles);

        if (transitioning) {
            transitionRadius -= transitionSpeed * delta;
            if (transitionRadius <= 0.0f) {
                mapFree(map);
                hashFree(offgridMap);
                hashFree(mData.enemies);
                hashFree(mData.computers);
                offgridMap = hashCreate(NULL, &offgridsFree, NULL);
                mData = mapCreate(offgridMap, biome_data, pathDirt);
                map = mData.map;
                computers = mData.computers;
                player->pos = mapFindSpawnTopLeft(map);
                player->rect.x = player->pos.x;
                player->rect.y = player->pos.y;
                g = guns[GetRandomValue(0, 3)];
                ammo = g.maxAmmo;
                reloadTimer = 0.0f; 
                reloading = false;
                computersHacked = 0;
                currComputer = NULL;
                isHacking = false;
                playerAlive = true;
                transitioning = false;
            }
        }
        collidingComputer = false;
        currComputer = NULL;
        if ((computer = hashFind(computers, enemyKey)) != NULL){
            for (int i = 0; i < computer->len; i++){
                Computer comp = computer->data[i];
                if (CheckCollisionRecs(comp->e->rect, player->rect)){
                    collidingComputer = true;
                    currComputer = comp;
                }
            }
        }
        double t_logic_end = GetTime();

        double t_draw_start = GetTime();
        // --- Drawing ---
        BeginTextureMode(target);
            ClearBackground((Color) {0, 0, 0, 0});
            BeginMode2D(camera);
                MapDrawCached(camera);
                if ((offgrids = hashFind(offgridMap, enemyKey)) != NULL){
                    for (int i = 0; i < offgrids->len; i++){
                        offgridTile o = (offgridTile) offgrids->data[i];
                        DrawTexture(o->texture, o->x , o->y , WHITE);
                    }
                }
                if ((enemies = hashFind(mData.enemies, enemyKey)) != NULL) {
                    for (int i = 0; i < enemies->len; i++) {
                        Enemy e = enemies->data[i];
                        Vector2 vel = computeVelOfEnemy(e, player, map, eprojectiles, isHacking);
                        update(e->e, map, vel);
                        enemyDraw(e, player, map, EnemyAnimations, enemyGunTex);
                    }
                }
                if ((computer = hashFind(computers, enemyKey)) != NULL){
                    for (int i = 0; i < computer->len; i++){
                        Computer comp = computer->data[i];
                        DrawTexture(computerTex, comp->e->rect.x - 10, comp->e->rect.y - 10, WHITE);
                    }
                }

                // Draw NPCS
                if ((npcs = hashFind(mData.npcs, enemyKey)) != NULL){
                    for (int i = 0; i < npcs->len; i++){
                        NPC n = npcs->data[i];
                        npcUpdate(n, map);
                        Texture2D npcFrame = NPCAnimations[n->type][n->state]->frames[n->currentFrame];
                        // Flip based on facingRight using DrawTexturePro
                        Rectangle nsrc = (Rectangle){ 0, 0, (float)npcFrame.width * n->facingRight, (float)npcFrame.height };
                        Rectangle ndst = (Rectangle){ n->e->rect.x, n->e->rect.y, (float)npcFrame.width, (float)npcFrame.height };
                        Vector2 norigin = (Vector2){ 0, 0 };
                        DrawTexturePro(npcFrame, nsrc, ndst, norigin, 0.0f, WHITE);
                    }
                }

                // Draw Player
                // DrawTexture(PlayerAnimations[pState]->frames[currentFrame], player->rect.x, player->rect.y, WHITE);
                Texture2D frame = PlayerAnimations[pState]->frames[currentFrame];
                src = (Rectangle) { 0, 0, (float)frame.width * facingRight, (float)frame.height };
                dst = (Rectangle) { player->rect.x, player->rect.y, (float)frame.width, (float)frame.height };
                Vector2 origin = { 0, 0 };
                DrawTexturePro(frame, src, dst, origin, 0.0f, WHITE);

                // Draw gun rotated around its image center so the center sits at the player's hand
                Rectangle gunSrc = (Rectangle){ 0, 0, (float)g.texture.width * facingRight, (float)g.texture.height };
                // place dest.x/y at the hand location (center), dest width/height equals texture size
                Rectangle gunDst = (Rectangle){
                    player->rect.x + player->rect.width * 0.5f,
                    player->rect.y + player->rect.height,
                    (float)g.texture.width,
                    (float)g.texture.height
                };
                Vector2 gunOrigin = (Vector2){ (float)g.texture.width * 0.5f, (float)g.texture.height * 0.5f };
                // adjust angle when flipped so the orientation matches
                float gunAngle = aimAngle;
                if (facingRight != 1) gunAngle += 180.0f;
                if (Vector2Length(aim.value) >= 0.1f){
                    DrawTexturePro(g.texture, gunSrc, gunDst, gunOrigin, gunAngle, WHITE);
                } else {
                    DrawTexturePro(g.texture, gunSrc, gunDst, gunOrigin, 0.0f, WHITE);
                }




                int pos = 0;
                while (pos < projectiles->len){
                    projectile p = projectiles->data[pos];

                    if (projectileUpdate(p, map)){
                        remove_dynarray(projectiles, pos);
                        continue;
                    }

                    if (p->e->rect.x > roomX * ROOM_SIZE + ROOM_SIZE ||
                        p->e->rect.y > roomY * ROOM_SIZE + ROOM_SIZE ||
                        p->e->rect.x < roomX * ROOM_SIZE ||
                        p->e->rect.y < roomY * ROOM_SIZE)
                    {
                        remove_dynarray(projectiles, pos);
                        continue;
                    }

                    bool removedProjectile = false;

                    if ((enemies = hashFind(mData.enemies, enemyKey)) != NULL){
                        int epos = 0;
                        while (epos < enemies->len){
                            Enemy e = enemies->data[epos];

                            if (CheckCollisionRecs(e->e->rect, p->e->rect)){
                                e->health -= g.damage;
                                e->state = ACTIVE;
                                // Impact_HitFlashTrigger(&e->flash )
                                Impact_SpawnBurst((Vector2){p->e->rect.x, p->e->rect.y}, RED, 8);
                                Impact_StartShake(0.15f, 3.0f);
                                remove_dynarray(projectiles, pos);
                                removedProjectile = true;

                                if (e->health <= 0){
                                    remove_dynarray(enemies, epos);
                                }

                                break;
                            }


                            epos += 1;
                        }
                    }

                    if (removedProjectile) {
                        continue;
                    }

                    projectileDraw(p);
                    pos += 1;
                }

                // Enemy Projectiles
                pos = 0;
                while (pos < eprojectiles->len){
                    projectile p = eprojectiles->data[pos];
                    if (projectileUpdate(p, map)){
                        remove_dynarray(eprojectiles, pos);
                        continue;
                    }
                    if (CheckCollisionRecs(player->rect, p->e->rect)){
                        if (playerAlive){
                            playerAlive = false;
                            transitioning = true; 
                            transitionRadius = GetScreenWidth();
                            transitionCenter = (Vector2) {GetScreenWidth() / 4.0f, GetScreenHeight() / 4.0f}; 
                        }
                    }
                    projectileDraw(p);
                    pos += 1;
                }
                // if ((enemies = hashFind(mData.enemies, enemyKey)) != NULL){
                //     for (int i = 0; i < enemies->len; i++){
                //         Enemy e = enemies->data[i];
                        
                //     }
                // }

                Impact_DrawParticles();

                // Crosshair
                if (Vector2Length(aim.value) > 0.1f) {
                    float crosshairDist = 80.0f; // distance from player
                    Vector2 crossPos = {
                        player->pos.x + aim.value.x * crosshairDist,
                        player->pos.y + aim.value.y * crosshairDist
                    };
                    
                    // Draw a simple crosshair
                    DrawLineV((Vector2){crossPos.x - 5, crossPos.y}, (Vector2){crossPos.x + 5, crossPos.y}, RED);
                    DrawLineV((Vector2){crossPos.x, crossPos.y - 5}, (Vector2){crossPos.x, crossPos.y + 5}, RED);
                }

            EndMode2D();


            DrawText(TextFormat("fps: %d", GetFPS()), 10, 10, 10, RED);
        EndTextureMode();
        double t_draw_end = GetTime();

        double t_present_start = GetTime();
        // --- Present ---
        SetShaderValueTexture(shader, GetShaderLocation(shader, "texture0"), target.texture);
        BeginDrawing();
            ClearBackground(BLACK);
            src = (Rectangle) { 0, 0, (float)target.texture.width, -(float)target.texture.height };
            dst = (Rectangle) { 0, 0, (float)SCREEN_WIDTH * 2, (float)SCREEN_HEIGHT * 2 };
            DrawTexturePro(target.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
            // EndShaderMode();i

            DrawText(TextFormat("Hacked: %d/%d", computersHacked, mData.noOfComputers), 100, 60, 10, RED);

            if (reloading) {
                DrawText("Reloading...", 100, 30, 10, RED);
            } else {
                DrawText(TextFormat("Ammo: %d/%d", ammo, g.maxAmmo), 100, 30, 10, RED);
            }

            // Draw Hack Button
            if (collidingComputer && currComputer && !currComputer->hacked) {
                Vector2 hackButtonCenter = {
                    aim.basePos.x,
                    aim.basePos.y - aim.baseRadius - 50
                };
                float hackButtonRadius = 40;

                Vector2 mouse = GetMousePosition();
                bool hovering = Vector2Distance(mouse, hackButtonCenter) <= hackButtonRadius;

                // Draw button background
                DrawCircleV(hackButtonCenter, hackButtonRadius,
                            isHacking ? RED : (hovering ? DARKGREEN : GREEN));

                // Draw border
                DrawCircleLines(hackButtonCenter.x, hackButtonCenter.y,
                                hackButtonRadius, BLACK);

                // Label changes depending on state
                const char *label = isHacking ? "STOP" : "HACK";
                int fontSize = 20;
                int textWidth = MeasureText(label, fontSize);
                DrawText(label,
                        hackButtonCenter.x - textWidth / 2,
                        hackButtonCenter.y - fontSize / 2,
                        fontSize, WHITE);

                // Handle click
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hovering) {
                    isHacking = !isHacking;  // toggle hack mode
                    printf("Hack button %s!\n", isHacking ? "started" : "stopped");
                }
            }

            if (isHacking && currComputer) {
                float progress = (100 - currComputer->amountLeftToHack) / 100.0f;
                DrawRectangle(50, SCREEN_HEIGHT - 40, 200, 20, DARKGRAY);
                DrawRectangle(50, SCREEN_HEIGHT - 40, (int)(200 * progress), 20, GREEN);
                DrawRectangleLines(50, SCREEN_HEIGHT - 40, 200, 20, BLACK);
            }


            
            if (transitioning) {
                BeginBlendMode(BLEND_ALPHA);
                DrawRectangle(0, 0, SCREEN_WIDTH*2, SCREEN_HEIGHT*2, BLACK); // fill screen
                DrawCircleV(
                    (Vector2){ transitionCenter.x * 2, transitionCenter.y * 2 }, 
                    transitionRadius * 2, 
                    WHITE
                );
                EndBlendMode();
                
            }

            DrawJoystick(joy);
            DrawJoystick(aim);



        EndDrawing();
        double t_present_end = GetTime();

        double t_frame_end = GetTime();

        // --- Print timings ---
        // TraceLog(LOG_INFO,
        //     "Frame: total=%.3fms anim=%.3fms input=%.3fms logic=%.3fms draw=%.3fms present=%.3fms\n",
        //     (t_frame_end - t_frame_start) * 1000.0,
        //     (t_anim_end - t_anim_start) * 1000.0,
        //     (t_input_end - t_input_start) * 1000.0,
        //     (t_logic_end - t_logic_start) * 1000.0,
        //     (t_draw_end - t_draw_start) * 1000.0,
        //     (t_present_end - t_present_start) * 1000.0
        // );
    }

    UnloadRenderTexture(target);
    mapFree(map);
    CloseWindow();
    return 0;
}

