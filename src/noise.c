#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "noise.h"

// Hash function for pseudo-randomness
static unsigned int hash(int x, int y) {
    unsigned int h = x * 374761393u + y * 668265263u; // Large primes
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Linear interpolation
static float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Smoothstep curve (makes interpolation look natural)
static float smoothstep(float t) {
    return t * t * (3 - 2 * t);
}

// 2D Value Noise function
float noise2d(float x, float y) {
    int xi = (int)floorf(x);
    int yi = (int)floorf(y);

    float xf = x - xi;
    float yf = y - yi;

    // Hash corners
    float v00 = (hash(xi, yi) & 0xFFFF) / 65535.0f;
    float v10 = (hash(xi + 1, yi) & 0xFFFF) / 65535.0f;
    float v01 = (hash(xi, yi + 1) & 0xFFFF) / 65535.0f;
    float v11 = (hash(xi + 1, yi + 1) & 0xFFFF) / 65535.0f;

    // Smooth interpolation
    float u = smoothstep(xf);
    float v = smoothstep(yf);

    float nx0 = lerp(v00, v10, u);
    float nx1 = lerp(v01, v11, u);
    return lerp(nx0, nx1, v); // 0.0 → 1.0
}
