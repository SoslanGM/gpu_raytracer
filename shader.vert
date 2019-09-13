#version 450

layout (location = 0) in vec3 coord;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec2 _uv;
layout (location = 1) out vec3 _normal;

layout (set = 0, binding = 0) uniform matrix
{
    mat4 proj;
    mat4 model;
};

void main()
{
    gl_Position = proj * model * vec4(coord, 1.0);
    _uv = uv;
    _normal = normal;
}