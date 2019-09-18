#version 450

layout (location = 0) in vec3 coord;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 _uv;

void main()
{
    gl_Position = vec4(coord, 1.0);
    _uv = uv;
}