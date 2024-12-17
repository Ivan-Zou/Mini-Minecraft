//#version 150
#version 330 core
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform sampler2D u_Texture;
uniform int u_Time;
uniform vec4 u_Color; // The color with which to render this instance of geometry.
uniform vec3 u_CamPos;

uniform int switchBiome;
uniform int inLava;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
// in vec4 fs_Col;
in vec4 fs_UV;

in float fs_dayCycleSpeed;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.

// Compute sky color for the fog
// values used for ray casting
uniform vec3 eye; // camera's position vector
uniform vec3 R; // camera's right vector
uniform vec3 U; // camera's up vector
uniform vec3 F; // camera's forward vector
uniform float aspect;
const float fovy = 45;


float random1( vec2 p ) {
    return fract(sin((dot(vec3(p, 0.5), vec3(127.1,
                                  311.7,
                                  191.999)))) *
                 43758.5453);
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                 dot(p, vec2(269.5,183.3))))
                 * 43758.5453);
}


//From notes for bubbly effect
float WorleyNoise(vec2 uv) {
    uv *= 7.0;
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0;

    // Scale down u_Time to control animation speed
    float timeScale = 0.05;
    float animatedTime = u_Time * timeScale;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            // Neighbor cell coordinates
            vec2 neighbor = vec2(float(x), float(y));

            // Random point within the neighbor cell with time-based animation
            vec2 point = random2(uvInt + neighbor);
            point += vec2(
                sin(animatedTime + point.x * 10.0),
                cos(animatedTime + point.y * 10.0)
            ) * 0.25;

            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }

    // Smooth the edges
    return smoothstep(0.0, 1.0, minDist);
}


float interpNoise2D(float x, float y) {
    int intX = int(floor(x));
    float fractX = fract(x);
    int intY = int(floor(y));
    float fractY = fract(y);

    float v1 = random1(vec2(intX, intY));
    float v2 = random1(vec2(intX + 1, intY));
    float v3 = random1(vec2(intX, intY + 1));
    float v4 = random1(vec2(intX + 1, intY + 1));

    float i1 = mix(v1, v2, fractX);
    float i2 = mix(v3, v4, fractX);
    return mix(i1, i2, fractY);
}

//FBM Noise for lava effect
float fbm(float x, float y) {
    float total = 0;
    float persistence = 0.5f;
    int octaves = 5;
    float freq = 2.f;
    float amp = 0.5f;
    for(int i = 1; i <= octaves; i++) {
        total += interpNoise2D(x * freq,
                               y * freq) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return total;
}


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

const vec3 daySkyCol = vec3(0.3686, 0.741, 1);
const vec3 nightSkyCol = vec3(0.05, 0.05, 0.1);

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
    return finalSkyCol;
}


void main()
{
    vec2 uv = vec2(fs_UV);

    // Animate Water and Lava
    if (fs_UV.z == 1) {
        float range = 2.0 / 16.0;
        float speed = 0.005;
        float offset = mod(u_Time * speed, 1.0) * range;
        float startX = uv.x;
        uv.x = startX + offset;
    }

    // Material base color (before shading)
    vec4 diffuseColor = texture(u_Texture, uv); // fs_Col;

    if (diffuseColor.a < 0.5f ) {
        discard; // Skip rendering this fragment
    }

    //SNOW GRASS PATCH
    if (fs_UV.w == -7) {
            vec2 snow = vec2(uv.x + 1.f/16.f, uv.y + 5.f/16.f);
            vec4 snowColor = texture(u_Texture, snow);

              vec2 pos2D = fs_Pos.xy;
              float radius = 8;

              // Center of the patch
              vec2 centerPos = floor(pos2D / radius) * radius + radius * 0.5f; // Center in the grid

              // Distance-based falloff
              float distanceToCenter = length(pos2D - centerPos);
              float falloff = 1.0 - smoothstep(0.0, radius, distanceToCenter);

             vec4 finalColor = mix(diffuseColor, snowColor, falloff);
             diffuseColor = finalColor;
    } else if (fs_UV.w == -8) {
        //DIRT GRASS PATCH
         vec2 grass = vec2(uv.x + 6.f/16.f, uv.y + 8.f/16.f);
         vec4 grassColor = texture(u_Texture, grass);


          vec2 pos2D = fs_Pos.xy;
          float radius = 5;

          // Center of the patch
          vec2 centerPos = floor(pos2D / radius) * radius + radius * 0.5f; // Center in the grid

          // Distance-based falloff
          float distanceToCenter = length(pos2D - centerPos);
          float falloff = 1.0 - smoothstep(0.0, radius, distanceToCenter);

          vec4 finalColor = mix(diffuseColor, grassColor, falloff);
          diffuseColor = finalColor;
    }  else if (fs_UV.w == -9) {
        //DIRT GRASS PATCH
         vec2 sand = vec2(uv.x, uv.y + 2.f/16.f);
         vec4 sandColor = texture(u_Texture, sand);

          vec2 pos2D = fs_Pos.xy;
          float radius = 5;

          // Center of the patch
          vec2 centerPos = floor(pos2D / radius) * radius + radius * 0.5f; // Center in the grid

          // Distance-based falloff
          float distanceToCenter = length(pos2D - centerPos);
          float falloff = 1.0 - smoothstep(0.0, radius, distanceToCenter);

          vec4 finalColor = mix(diffuseColor, sandColor, falloff);
          diffuseColor = finalColor;
    }


    // Add lighting to water waves
    float specularTerm = 0.f;
    if (fs_UV.w == 1) {
        // Use Blinn-Phong shading model
        vec3 lightDir = normalize(vec3(fs_LightVec));
        vec3 viewDir = normalize(u_CamPos-fs_Pos.xyz);
        vec3 halfDir = normalize((lightDir + viewDir) / 2.f);
        vec3 normal = normalize(vec3(fs_Nor));

        specularTerm = max(pow(dot(normal, halfDir), 128), 0) + 0.1;
    }

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(fs_Nor), normalize(fs_LightVec));

    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm + specularTerm;   //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.

    // Compute shaded color
    vec4 shadedColor = vec4(diffuseColor.rgb * lightIntensity, diffuseColor.a);

    // Add fog
    Ray ray = raycast();
    vec4 fogColor = vec4(skyColor(ray.direction, vec3(fs_LightVec)), 1); //vec4(0.3686, 0.741, 1, 1);
    float blockDistance = length(fs_Pos.xyz - u_CamPos);
    float fogStart = 128.f;
    float fogEnd = 144.f;
    float fogFactor = clamp((blockDistance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    shadedColor = mix(shadedColor, fogColor, fogFactor);
    out_Col = shadedColor;






}
