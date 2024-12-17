// #version 150
#version 330 core
// ^ Change this to version 130 if you have compatibility issues

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_Model;       // The matrix that defines the transformation of the
                            // object we're rendering. In this assignment,
                            // this will be the result of traversing your scene graph.

uniform mat4 u_ModelInvTr;  // The inverse transpose of the model matrix.
                            // This allows us to transform the object's normals properly
                            // if the object has been non-uniformly scaled.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

uniform vec4 u_Color;       // When drawing the cube instance, we'll set our uniform color to represent different block types.

uniform int u_Time;

in vec4 vs_Pos;             // The array of vertex positions passed to the shader

in vec4 vs_Nor;             // The array of vertex normals passed to the shader

// in vec4 vs_Col;             // The array of vertex colors passed to the shader.

in vec4 vs_UV;

out vec4 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_LightVec;       // The direction in which our virtual light lies, relative to each vertex. This is implicitly passed to the fragment shader.
// out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.
out vec4 fs_UV;

out float fs_dayCycleSpeed;
// const vec4 lightDir = normalize(vec4(1, 0.1, 0, 0));  // The direction of our virtual light, which is used to compute the shading of
//                                         // the geometry in the fragment shader.

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// 24 minute Day-Night Cycles
const float dayCycleSpeed = (TWO_PI / (60 * 60 * 24));


// Function to compute the sun's direction based on u_Time
vec3 computeSunDirection() {
    // Sun starting position
    float start = PI / 2.f;
    // Do a full rotation
    float angle = mod(u_Time * dayCycleSpeed + start, TWO_PI);
    float y = sin(angle);
    float x = cos(angle);
    return vec3(x, y, 0);
}

void main()
{
    fs_dayCycleSpeed = dayCycleSpeed;
    fs_Pos = vs_Pos;
    // fs_Col = vs_Col;                         // Pass the vertex colors to the fragment shader for interpolation
    fs_UV = vs_UV;
    vec3 normal = vec3(vs_Nor);

    vec4 modelposition = vs_Pos;

    //Cactus
    if (vs_UV.w == 2) {
        modelposition.y -= 0.04f;
    }

    if (vs_UV.w == 3) {
        modelposition.x -= 0.08f;
    } else if (vs_UV.w == -3) {
        modelposition.x += 0.08f;
    }

    if (vs_UV.w == 4) {
        modelposition.z -= 0.08f;
    } else if (vs_UV.w == -4) {
        modelposition.z += 0.08f;
    }


    // Water Waves
    if (vs_UV.w == 1) {
        float waveAmplitude1 = 0.06;
        float waveFrequency1 = 4.0;
        float waveSpeed1 = 0.035;

        float waveAmplitude2 = 0.04;
        float waveFrequency2 = 2.0;
        float waveSpeed2 = 0.025;

        float waveAmplitude3 = 0.05;
        float waveFrequency3 = 3.0;
        float waveSpeed3 = 0.015;

        // start at a lower y
        modelposition.y -= 0.25;
        // Compute multi-directional wave distortion
        float wave1 = sin(-waveFrequency1 * modelposition.x - waveFrequency1 * modelposition.z - waveSpeed1 * u_Time);
        float wave2 = sin(waveFrequency2 * modelposition.x + waveFrequency2 * modelposition.z - waveSpeed2 * u_Time);
        float wave3 = sin(waveFrequency3 * modelposition.x + waveFrequency3 * modelposition.z + waveSpeed3 * u_Time);

        modelposition.y += waveAmplitude1 * wave1 + waveAmplitude2 * wave2 + waveAmplitude3 * wave3;

        // update normals
        float dx =
            -waveAmplitude1 * waveFrequency1 * (1 - wave1) +
            waveAmplitude2 * waveFrequency2 * (1 - wave2) +
            waveAmplitude3 * waveFrequency3 * (1 - wave3);

        float dz =
            -waveAmplitude1 * waveFrequency1 * (1 - wave1) +
            waveAmplitude2 * waveFrequency2 * (1 - wave2) +
            waveAmplitude3 * waveFrequency3 * (1 - wave3);

        normal = normalize(vec3(dx, 1.0, dz));
    }

    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = vec4(invTranspose * normal, 0);          // Pass the vertex normals to the fragment shader for interpolation.
                                                            // Transform the geometry's normals by the inverse transpose of the
                                                            // model matrix. This is necessary to ensure the normals remain
                                                            // perpendicular to the surface after the surface is transformed by
                                                            // the model matrix.


    modelposition = u_Model * modelposition;   // Temporarily store the transformed vertex positions for use below

    fs_LightVec = normalize(vec4(computeSunDirection(), 0));  // Compute the direction in which the light source lies

    gl_Position = u_ViewProj * modelposition;// gl_Position is a built-in variable of OpenGL which is
                                             // used to render the final positions of the geometry's vertices
}
