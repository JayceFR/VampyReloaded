#include "raylib.h"
#include "raymath.h"
#include "stdio.h"
// #include <math.h>

#define MAX_BOIDS 700
#define SCREEN_WIDTH 800 
#define SCREEN_HEIGHT 450 

#define MAX_FORCE 0.2
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

        if (Vector2Length(b->velocity) > MAX_SPEED){
            Vector2 normalised = Vector2Normalize(b->velocity);
            b->velocity.x = normalised.x * MAX_SPEED;
            b->velocity.y = normalised.y * MAX_SPEED;
        }
        
        Vector2 off = Vector2Add(b->pos, b->velocity);
        b->pos.x = off.x;
        b->pos.y = off.y;

        if (b->pos.x > GetScreenWidth()){
            b->pos.x = 0; 
        } 
        else if (b->pos.x < 0){
            b->pos.x = GetScreenWidth();
        }

        if (b->pos.y > GetScreenHeight()){
            b->pos.y = 0;
        }
        else if (b->pos.y < 0){
            b->pos.y = GetScreenHeight();
        }
    }
}

Vector2 ClampMagnitude(Vector2 v, float maxLength) {
    float len = Vector2Length(v);
    if (len > maxLength) {
        v = Vector2Scale(Vector2Normalize(v), maxLength);
    }
    return v;
}

void calculateSteering(Boid *flock, Vector2 *steerings, Vector2 playerPos) {
    int perceptionRadius = 100;

    const float ALIGN_WEIGHT = 1.0f;
    const float COHESION_WEIGHT = 1.25f;
    const float SEPARATION_WEIGHT = 2.0f;
    const float FOLLOW_PLAYER_WEIGHT = 1.5f;  // Adjust weight as needed

    for (int i = 0; i < MAX_BOIDS; i++) {
        Boid *boid = &flock[i];

        Vector2 avgVel = {0, 0};
        Vector2 avgPos = {0, 0};
        Vector2 avgSep = {0, 0};
        int total = 0;

        for (int j = 0; j < MAX_BOIDS; j++) {
            if (j == i) continue;

            float d = Vector2Distance(boid->pos, flock[j].pos);
            if (d <= perceptionRadius && d > 0.0f) {
                total++;
                avgVel = Vector2Add(avgVel, flock[j].velocity);
                avgPos = Vector2Add(avgPos, flock[j].pos);

                Vector2 diff = Vector2Subtract(boid->pos, flock[j].pos);
                diff = Vector2Scale(diff, 1.0f / (d * d));
                avgSep = Vector2Add(avgSep, diff);
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
            Vector2 alignForce = Vector2Subtract(avgVel, boid->velocity);
            alignForce = ClampMagnitude(alignForce, MAX_FORCE);

            // Cohesion
            avgPos = Vector2Scale(avgPos, 1.0f / total);
            Vector2 cohVector = Vector2Subtract(avgPos, boid->pos);
            if (Vector2Length(cohVector) > 0.001f) {
                cohVector = Vector2Normalize(cohVector);
                cohVector = Vector2Scale(cohVector, MAX_SPEED);
            }
            cohVector = Vector2Subtract(cohVector, boid->velocity);
            cohVector = ClampMagnitude(cohVector, MAX_FORCE);

            // Separation
            avgSep = Vector2Scale(avgSep, 1.0f / total);
            Vector2 sepVector = avgSep;
            if (Vector2Length(sepVector) > 0.001f) {
                sepVector = Vector2Normalize(sepVector);
                sepVector = Vector2Scale(sepVector, MAX_SPEED);
            }
            sepVector = Vector2Subtract(sepVector, boid->velocity);
            sepVector = ClampMagnitude(sepVector, MAX_FORCE);

            steering = Vector2Add(
                Vector2Scale(sepVector, SEPARATION_WEIGHT),
                Vector2Add(
                    Vector2Scale(alignForce, ALIGN_WEIGHT),
                    Vector2Scale(cohVector, COHESION_WEIGHT)
                )
            );
        }

        const float FOLLOW_RADIUS = 100.0f;

        Vector2 toPlayer = Vector2Subtract(playerPos, boid->pos);
        float distToPlayer = Vector2Length(toPlayer);

        Vector2 followForce = {0, 0};

        if (distToPlayer > FOLLOW_RADIUS) {
            // If too far, move toward player
            Vector2 desired = Vector2Normalize(toPlayer);
            desired = Vector2Scale(desired, MAX_SPEED);

            followForce = Vector2Subtract(desired, boid->velocity);
            followForce = ClampMagnitude(followForce, MAX_FORCE);
            followForce = Vector2Scale(followForce, FOLLOW_PLAYER_WEIGHT);

        } else {
            // If inside the radius, apply a small force away from player (optional)
            Vector2 away = Vector2Normalize(Vector2Scale(toPlayer, -1)); // away vector
            away = Vector2Scale(away, MAX_SPEED);

            followForce = Vector2Subtract(away, boid->velocity);
            followForce = ClampMagnitude(followForce, MAX_FORCE * 0.7f); // weaker force pushing away
        }


        // Add follow player force
        steering = Vector2Add(steering, followForce);

        steerings[i] = steering;
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

Vector2 randomVelocity(float minSpeed, float maxSpeed) {
    float angle = ((float)GetRandomValue(0, 360)) * (PI / 180.0f);
    float speed = randRange(minSpeed, maxSpeed);
    return (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
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

        calculateSteering(flock, averageVels, playerPos);
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
