#version 450

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec4 color;

void main()
{
    vec3 light_dir = vec3(-1.0, 0.0, 0.0);
    
    float diff = max(dot(normal, light_dir), 0.0);
    
    vec3 bunny_color = vec3(0.988, 0.729, 0.012);
    color = diff * vec4(bunny_color, 1.0);
    
    //color = vec4(normal, 1.0);
}