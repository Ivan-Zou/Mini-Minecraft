#version 150

in vec4 vs_Pos;
in vec2 vs_UV;

out vec2 fs_UV;

void main(void)
{
    gl_Position = vs_Pos;
    fs_UV = vs_UV;
}
