#version 150
in vec4 vs_Pos;
uniform mat4 u_Model;
uniform mat4 u_ViewProj;

in vec4 vs_UV;
out vec4 fs_UV;

void main()
{

    fs_UV = vs_UV;
    //vec4 modelposition = u_Model * vs_Pos;

    //built-in things to pass down the pipeline
    //gl_Position = u_ViewProj * modelposition;
    gl_Position = vs_Pos;
}
