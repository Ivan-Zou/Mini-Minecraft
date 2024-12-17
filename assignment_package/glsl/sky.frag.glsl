#version 150

// values used for ray casting
uniform vec3 eye; // camera's position vector
uniform vec3 R; // camera's right vector
uniform vec3 U; // camera's up vector
uniform vec3 F; // camera's forward vector
uniform float aspect;

uniform int u_Time;

in vec2 fs_UV;

out vec4 out_Col;

const float fovy = 45;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// 24 minute Day-Night Cycles
const float dayCycleSpeed = (TWO_PI / (60 * 60 * 24));

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                                vec3(254, 192, 81) / 255.0,
                                vec3(255, 137, 103) / 255.0,
                                vec3(253, 96, 81) / 255.0,
                                vec3(57, 32, 51) / 255.0);

// Dusk palette
const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
                                vec3(96, 72, 120) / 255.0,
                                vec3(72, 48, 120) / 255.0,
                                vec3(48, 24, 96) / 255.0,
                                vec3(0, 24, 72) / 255.0);

const vec3 sunCol = vec3(255, 255, 190) / 255.0;
const vec3 moonCol = vec3(180, 180, 180) / 255.0;
const vec3 daySkyCol = vec3(0.3686, 0.741, 1);
const vec3 nightSkyCol = vec3(0.05, 0.05, 0.1);

// Function to compute the sun's direction based on u_Time
vec3 computeSunDirection() {
    // Sun starting position
    float start = PI / 2.f;
    // Do a full rotation
    float angle = mod(u_Time * dayCycleSpeed + start, TWO_PI);
    float y = sin(angle);
    float x = cos(angle);
    return normalize(vec3(x, y, 0));
}

// Noise function that returns a random 3D point given a 3D point
vec3 random3(vec3 p) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

// Generate worley noise given a 3D point
float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int z = -1; z <= 1; z++)
    {
        for(int y = -1; y <= 1; y++)
        {
            for(int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

float fractalWorley(vec3 rayDir) {
    float amp = 0.5;
    float freq = 1.0;
    float sum = 0;
    for (int i = 0; i < 4; ++i) {
        sum += WorleyNoise3D(rayDir * freq) * amp;
        amp *= 0.5;
        freq *= 2;
    }
    return sum;
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

// formula for ray casting
Ray raycast() {
    vec3 ref = eye + F;
    float len = 1;
    vec3 V = U * len * tan(radians(fovy / 2.f));
    vec3 H = R * len * tan(radians(fovy / 2.f)) * aspect;

    float sx = fs_UV.x * 2 - 1;
    float sy = fs_UV.y * 2 - 1;

    vec3 p = ref + sx * H + sy * V;
    Ray ray;
    ray.origin = eye;
    ray.direction = normalize(p - eye);

    return ray;
}

vec3 sunsetLerp(float t) {
    t *= 4;
    float tFract = fract(t);
    int tLow = int(floor(t));
    int tHigh = tLow + 1;
    return mix(sunset[tLow], sunset[tHigh], tFract);
}

vec3 duskLerp(float t) {
    t *= 4;
    float tFract = fract(t);
    int tLow = int(floor(t));
    int tHigh = tLow + 1;
    return mix(dusk[tLow], dusk[tHigh], tFract);
}

vec3 skyColor(vec3 rayDir, vec3 sunDir) {
    // compute the sun's elevation
    float sunElevation = clamp(dot(sunDir, vec3(0, 1, 0)), -1.0, 1.0);

    // Transition between day and night colors based on sun elevation
    vec3 skyBaseCol = mix(nightSkyCol, daySkyCol, max(sunElevation, 0.0));

    // Use Worley Noise to create clouds
    float t = clamp(dot(rayDir, vec3(0, 1, 0)), 0, 1);
    float worley = fractalWorley(rayDir * 10);
    worley = fract(worley) * 0.2;
    t = clamp(t + worley, 0, 1);

    // Compute the sunset and dusk colors
    float t2 = clamp(dot(rayDir, sunDir), 0, 1);
    vec3 settingCol = mix(duskLerp(t), sunsetLerp(t), t2);

    // Transition between skyBaseCol and settingCol based on the sun's elevation
    float transitionFactor = smoothstep(-1, 1, sunElevation);
    vec3 finalSkyCol;
    if (sunElevation > 0) {
        finalSkyCol = mix(settingCol, skyBaseCol, transitionFactor);
    } else {
        finalSkyCol = mix(settingCol, skyBaseCol, 1 - transitionFactor);
    }

    // Draw the sun
    if (t2 > 0.995) {
        finalSkyCol = sunCol;
    } else if (t2 > 0.99) {
        finalSkyCol = mix(finalSkyCol, sunCol, (t2 - 0.99) / 0.009);
    }

    // Draw the moon
    vec3 moonDir = -sunDir;
    float moonVisibility = clamp(dot(rayDir, moonDir), 0.0, 1.0);
    float moonGlow = smoothstep(0.97, 1.0, moonVisibility);
    float moonFalloff = smoothstep(0.997, 1.0, moonVisibility);

    // Add moon to the final color
    vec3 moon = moonCol * moonGlow * moonFalloff * 2;
    finalSkyCol = mix(finalSkyCol, moon, moonFalloff);
    return finalSkyCol;
}

void main(void) {
    Ray ray = raycast();
    vec3 sunDir = computeSunDirection();
    out_Col = vec4(skyColor(ray.direction, sunDir), 1);
}
