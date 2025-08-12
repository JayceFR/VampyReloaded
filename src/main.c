#include "raylib.h"
#include "raymath.h"
#include "stdio.h"
// #include <math.h>

#define MAX_BOIDS 50 

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

void updateBoids(Boid *flock){
    for (int i = 0; i < MAX_BOIDS; i++){
        Boid* b = &flock[i];
        printf("old pos value %f, %f \n", b->pos.x, b->pos.y);
        Vector2 off = Vector2Add(b->pos, Vector2Add(b->velocity, b->acceleration));
        b->pos.x = off.x; 
        b->pos.y = off.y;
        printf("new pos value %f, %f \n", b->pos.x, b->pos.y);
        printf("--------------\n");
    }
}

void DrawBoids(Boid *flock){
    for (int i = 0; i < MAX_BOIDS; i++){
        DrawCircleV(flock[i].pos, 10, BLUE);
    }
}

#define SCREEN_WIDTH 800 
#define SCREEN_HEIGHT 450 

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "On-Screen Joystick Example");
    SetTargetFPS(60);

    Joystick joy = CreateJoystick((Vector2){100, 350}, 60);
    Joystick aim = CreateJoystick((Vector2){300, 350}, 60);

    Vector2 playerPos = {400, 225};

    Boid flock[MAX_BOIDS];
    for (int i = 0; i < MAX_BOIDS; i++){
        flock[i].pos = (Vector2) {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
        flock[i].velocity = (Vector2) {GetRandomValue(-5, 5), GetRandomValue(-5,5)};
        flock[i].acceleration = (Vector2) {GetRandomValue(-2,2), GetRandomValue(-2, 2)};
    }

    while (!WindowShouldClose()) {
        UpdateJoystick(&joy);

        // Apply joystick value to player movement
        playerPos.x += joy.value.x * 5;
        playerPos.y += joy.value.y * 5;

        updateBoids(flock);

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
