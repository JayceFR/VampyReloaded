#include "raylib.h"
#include "raymath.h"
// #include <math.h>

typedef struct {
    Vector2 basePos;     // Center of joystick base
    float baseRadius;
    Vector2 thumbPos;    // Position of joystick thumb
    float thumbRadius;
    Vector2 value;       // Normalized [-1,1] x/y output
    bool active;         // Is joystick currently touched?
    float offvalue 
} Joystick;

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

int main() {
    InitWindow(800, 450, "On-Screen Joystick Example");
    SetTargetFPS(60);

    Joystick joy = CreateJoystick((Vector2){100, 350}, 60);
    Joystick aim = CreateJoystick((Vector2){300, 350}, 60);

    Vector2 playerPos = {400, 225};

    while (!WindowShouldClose()) {
        UpdateJoystick(&joy);

        // Apply joystick value to player movement
        playerPos.x += joy.value.x * 5;
        playerPos.y += joy.value.y * 5;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawJoystick(joy);
        DrawCircleV(playerPos, 20, RED);

        DrawText(TextFormat("Joystick: (%.2f, %.2f) JoyValue : (%.2f)", joy.value.x, joy.value.y, joy.offvalue), 10, 10, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
