#version 330 core
// ^ Change this to version 130 if you have compatibility issues



uniform int inWater;
uniform int inLava;
in vec4 fs_UV;
uniform int u_Time;
uniform sampler2D u_Texture;

out vec4 out_Col;


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


//From notes
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


void main()
{
    // // Material base color (before shading)

    vec2 uv = vec2(fs_UV);
    vec4 diffuseColor = texture(u_Texture, uv);
    if (inWater == 1) {
        vec3 waterTint = vec3(0.0, 0.5, 1.0);  // Light blue color for the water tint


        float noise = WorleyNoise(uv);
        vec3 col = vec3((noise) * diffuseColor.r,(noise) * diffuseColor.g, (noise) * diffuseColor.b);

        out_Col = vec4(mix(col, waterTint, 0.35), diffuseColor.a);


    } else if (inLava == 1) {
        vec3 lavaTint = vec3(1.0, 0.25, 0.0);  // Light orange color for the lava tint
        vec2 distortedUV = uv + (vec2(fbm(uv.x, uv.y), fbm(uv.x + 12, uv.y + 12)));

        float noise = WorleyNoise(distortedUV);
        vec3 col = vec3(diffuseColor.r,  diffuseColor.g,  diffuseColor.b);
          // Blend the water tint with the base diffuseColor
        out_Col = vec4(mix(col, lavaTint, noise), diffuseColor.a);
    } else {
        out_Col = vec4(diffuseColor.r,  diffuseColor.g, diffuseColor.b , diffuseColor.a);
    }

}
