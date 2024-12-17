#include "noise.h"



float random1D(glm::vec2 in) {
    return glm::fract(sin(glm::dot(in, glm::vec2(127.1, 311.7))) *
                      43758.5453);

}

glm::vec2 random2D(glm::vec2 in) {
    return glm::fract(glm::sin(glm::vec2(
                          glm::dot(in, glm::vec2(127.1, 311.7)),
                          glm::dot(in, glm::vec2(269.5,183.3))))
                      * 43758.5453f);
}


glm::vec3 random3D(glm::vec3 in) {
    return glm::fract(glm::sin(glm::vec3(
                          glm::dot(in, glm::vec3(127.1, 311.7, 1)),
                          glm::dot(in, glm::vec3(269.5, 183.3, 1)),
                          glm::dot(in, glm::vec3(113.5, 271.9, 1))))
                      *= 43758.5453f);
}

float fade(float t) {
    return 1 - (t * t * t * (t * (t * 6 - 15) + 10));
}

glm::vec3 quinticFade(const glm::vec3& t2) {
    // Compute each component of the fall-off separately
    glm::vec3 t2Cubed = t2 * t2 * t2;        // t2^3
    glm::vec3 t2Fourth = t2Cubed * t2;       // t2^4
    glm::vec3 t2Fifth = t2Fourth * t2;       // t2^5

    // Correct quintic interpolation formula: 6*t^5 - 15*t^4 + 10*t^3
    glm::vec3 result = glm::vec3(1.f) - (6.f * t2Fifth + 15.f * t2Fourth - 10.f * t2Cubed);

    return result;
}


float interpNoise2D(float x, float y) {
    int intX = int(floor(x));
    float fractX = glm::fract(x);
    int intY = int(floor(y));
    float fractY = glm::fract(y);

    float v1 = random1D(glm::vec2(intX, intY));
    float v2 = random1D(glm::vec2(intX + 1, intY));
    float v3 = random1D(glm::vec2(intX, intY + 1));
    float v4 = random1D(glm::vec2(intX + 1, intY + 1));

    float i1 = glm::mix(v1, v2, fractX);
    float i2 = glm::mix(v3, v4, fractX);
    return glm::mix(i1, i2, fractY);
}


float simpleFBM(float x, float z) {
    float total = 0;
    float persistence = 0.5f;
    int octaves = 5;
    float freq = 2.f;
    float amp = 0.5f;
    for(int i = 1; i <= octaves; i++) {
        total += interpNoise2D(x * freq,
                               z * freq) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return total;
}


float perlin(float x, float z, int gridSize) {
    x /= gridSize;
    z /= gridSize;
    // Cell the point is in
    int ix = int(floor(x));
    int iz = int(floor(z));
    float s = 0.f;
    for (int dx = 0; dx <= 1; ++dx) {
        for (int dz = 0; dz <= 1; ++dz) {
            // Get neighbor
            int gridX = ix + dx;
            int gridZ = iz + dz;
            // Compute distance
            float distX = abs(x - gridX);
            float distZ = abs(z - gridZ);
            // Compute falloff
            float tx = fade(distX);
            float tz = fade(distZ);
            // Get gradient
            glm::vec2 gradient = 2.f * random2D(glm::vec2(gridX, gridZ)) - glm::vec2(1.f);
            // Compute surflet and add to sum
            glm::vec2 diff = glm::vec2(x, z) - glm::vec2(gridX, gridZ);
            s += glm::dot(diff, gradient) * tx * tz;
        }
    }
    return glm::clamp(s, -1.f, 1.f);
}




float sufletHelper(glm::vec3 p, glm::vec3 gridPoint) {
    //Find the dist vector to original point
    float distX = abs(p.x - gridPoint.x);
    float distY = abs(p.y - gridPoint.y);
    float distZ = abs(p.z - gridPoint.z);

    glm::vec3 t2 = glm::vec3(distX, distY, distZ);

    //Quintic fall off
    glm::vec3 t = quinticFade(t2);


    //Random vector from gridPoint
    glm::vec3 gradient = random3D(gridPoint) * 2.f - glm::vec3(1.f, 1.f, 1.f);

    //Surflet computation
    glm::vec3 diff = p - gridPoint;

    //Running surfletSum
    return (glm::dot(diff, gradient) * t.x * t.y * t.z);
}

//3D Perlin Noise
float perlin3D(float x, float y, float z, int gridSize) {
    float surfletSum = 0.f;
    // Cell the point is in
    int ix = int(floor(x));
    int iy = int(floor(y));
    int iz = int(floor(z));

    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += sufletHelper(glm::vec3(x,y,z), glm::vec3(ix, iy, iz) + glm::vec3(dx, dy,dz));
            }
        }
    }
    return surfletSum;
}



float voronoi(float x, float z, int gridSize) {
    x /= gridSize;
    z /= gridSize;
    glm::vec2 xzInt = glm::vec2(floor(x), floor(z));
    glm::vec2 xzFract = glm::fract(glm::vec2(x, z));
    float res = 0.f;
    // Iterate through neighbors
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            glm::vec2 d = glm::vec2(dx, dz);
            glm::vec2 diff = d + random2D(xzInt + d) - xzFract;
            res += 1.f / pow(glm::dot(diff, diff), 8.f);
        }
    }
    return pow(1.f / res, 1.f / 16.f);
}


float fbm(float x, float z, int gridSize, int octaves,
          float (*noise)(float, float, int),
          float (*transform)(float)) {
    float res = 0;
    float amp = 1.f;
    float persistance = 0.5f;
    float freq = 1.f;
    float s = 0;
    for (int i = 0; i < octaves; ++i) {
        float inX = x * freq;
        float inZ = z * freq;
        float n = (noise == nullptr ? random1D(glm::vec2(inX, inZ)) : noise(inX, inZ, gridSize));
        if (transform != nullptr) {
            n = transform(n);
        }
        n *= amp;
        res += n;
        s += amp;
        freq /= persistance;
        amp *= persistance;
    }
    return res / s;
}

float minusValue(float x) {
    return 1 - x;
}
