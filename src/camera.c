#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "physics.h"
#include "map.h"
#include "camera.h"

void UpdateCameraRoom(Camera2D *camera, entity player) {
    // Which room is player in?
    int roomX = (int)(player->pos.x / ROOM_SIZE);
    int roomY = (int)(player->pos.y / ROOM_SIZE);

    // Target is the center of the room
    Vector2 target = {
        roomX * ROOM_SIZE + ROOM_SIZE / 2,
        roomY * ROOM_SIZE + ROOM_SIZE / 2
    };

    // Smoothly move camera target toward room center
    float lerpSpeed = 5.0f * GetFrameTime();
    camera->target.x = Lerp(camera->target.x, target.x, lerpSpeed);
    camera->target.y = Lerp(camera->target.y, target.y, lerpSpeed);

    // Clamp so camera doesn't leave the room
    float halfW = GetScreenWidth() / 4.0f;
    float halfH = GetScreenHeight() / 4.0f;

    float minX = roomX * ROOM_SIZE + halfW;
    float maxX = (roomX + 1) * ROOM_SIZE - halfW;
    float minY = roomY * ROOM_SIZE + halfH;
    float maxY = (roomY + 1) * ROOM_SIZE - halfH;

    camera->target.x = Clamp(player->pos.x, minX, maxX);
    camera->target.y = Clamp(player->pos.y, minY, maxY);
}