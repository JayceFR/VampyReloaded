#include "raylib.h"
#include "raymath.h"
#include "stdio.h"
// #include <math.h>

#define MAX_BOIDS 50
#define SCREEN_WIDTH 800 
#define SCREEN_HEIGHT 450 

#define MAX_FORCE 0.04
#define MAX_SPEED 4 

typedef struct {
    Vector2 basePos;     // Center of joystick base
    float baseRadius;
    Vector2 thumbPos;    // Position of joystick thumb
    float thumbRadius;
    Vector2 value;       // Normalized [-1,1] x/y output
    bool active;         // Is joystick currently touched?
    float offvalue;
} Joystick;

typedef struct{
    Vector2 pos; 
    Vector2 velocity; 
    Vector2 acceleration; 
} Boid; 

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


void updateBoids(Boid *flock, Vector2 *averageVels){
    for (int i = 0; i < MAX_BOIDS; i++){
        Boid* b = &flock[i];

        b->acceleration.x = averageVels[i].x;
        b->acceleration.y = averageVels[i].y;

        Vector2 offvel = Vector2Add(b->velocity, b->acceleration);
        b->velocity.x = offvel.x;
        b->velocity.y = offvel.y; 
        
        Vector2 off = Vector2Add(b->pos, b->velocity);
        b->pos.x = off.x;
        b->pos.y = off.y;

        if (b->pos.x > SCREEN_WIDTH){
            b->pos.x = 0; 
        } 
        else if (b->pos.x < 0){
            b->pos.x = SCREEN_WIDTH;
        }

        if (b->pos.y > SCREEN_HEIGHT){
            b->pos.y = 0;
        }
        else if (b->pos.y < 0){
            b->pos.y = SCREEN_HEIGHT;
        }
    }
}

void align(Boid *flock, Vector2 *averageVels){
    int perceptionRadius = 50; 
    Boid *boid; 
    // Get a snapshot and get the average of each boid 
    for (int i = 0; i < MAX_BOIDS && (boid = &flock[i]); i++){
        Vector2 avg = {0, 0}; // Average velocity
        int total = 0;
        for (int j = 0; j < MAX_BOIDS; j++){
            if (j != i){
                // Compute the distance between them 
                float d = Vector2Distance(boid->pos, flock[j].pos);
                if (d <= perceptionRadius){
                    total += 1; 
                    avg = Vector2Add(avg, flock[j].velocity);
                }
            }
        }
        if (total > 0){
            avg = Vector2Scale(avg, 1.0f / (float) total);
            Vector2 normalised = Vector2Normalize(avg);
            avg = (Vector2) {normalised.x * MAX_SPEED, normalised.y * MAX_SPEED};
            avg = Vector2Subtract(avg, boid->velocity);
            Vector2ClampValue(avg, -100000000, MAX_FORCE);
        }
        averageVels[i] = avg;
    }

}

void DrawBoids(Boid *flock){
    for (int i = 0; i < MAX_BOIDS; i++){
        float x = flock[i].pos.x;
        float y = flock[i].pos.y;
        DrawCircleV(
            (Vector2) {x, y},
            5, 
            BLUE
        );
    }
}

Vector2 randomVelocity(float min, float max) {
    Vector2 v;
    do {
        v = (Vector2){ randRange(min, max), randRange(min, max) };
    } while (Vector2Length(v) == 0); // retry if zero-length
    return v;
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "On-Screen Joystick Example");
    SetTargetFPS(60);

    Joystick joy = CreateJoystick((Vector2){100, 350}, 60);
    Joystick aim = CreateJoystick((Vector2){300, 350}, 60);

    Vector2 playerPos = {400, 225};

    Boid flock[MAX_BOIDS];
    for (int i = 0; i < MAX_BOIDS; i++){
        flock[i].pos = (Vector2) {GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT)};
        flock[i].velocity = randomVelocity(-2,2);
        flock[i].acceleration = (Vector2) {randRange(-0.5, 0.5), randRange(-0.5, 0.5)};
    }

    Vector2 averageVels[MAX_BOIDS];

    while (!WindowShouldClose()) {
        UpdateJoystick(&joy);

        // Apply joystick value to player movement
        playerPos.x += joy.value.x * 5;
        playerPos.y += joy.value.y * 5;

        align(flock, averageVels);
        updateBoids(flock, averageVels);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawJoystick(joy);
        DrawCircleV(playerPos, 20, RED);

        DrawBoids(flock);

        DrawText(TextFormat("Joystick: (%.2f, %.2f) JoyValue : (%.2f)", joy.value.x, joy.value.y, joy.offvalue), 10, 10, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
