#pragma once
#include <glm_includes.h>

// Deterministic 1D noise function
float random1D(glm::vec2 in);

// Deterministic 2D noise function
glm::vec2 random2D(glm::vec2 in);

// Deterministic 3D noise function
glm::vec3 random3D(glm::vec3 in);

float fade(float t);

// 2D Perlin Noise
float perlin(float x, float z, int gridSize = 16);

//3D Perlin Noise
float perlin3D(float x, float y, float z, int gridSize = 16);

float simpleFBM(float x, float z);

// 2D Voronoi Noise
float voronoi(float x, float z, int gridSize = 16);

float fbm(float x, float z, int gridSize, int octaves = 8,
          float (*noise)(float, float, int) = nullptr,
          float (*transform)(float) = nullptr);



float minusValue(float x);
