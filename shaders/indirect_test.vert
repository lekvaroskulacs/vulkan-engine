#version 450

#include "common/scene_light.glsl"

vec3 verts[3] = vec3[](vec3(0.0, 0.0, 0.0), vec3(0.5, 1.0, 0.5), vec3(1.0, 0.0, 1.0));
uint indices[3] = uint[](0, 1, 2);


void main() 
{
    vec4 worldPos = vec4(verts[indices[gl_VertexIndex]], 1.0);
    
    worldPos.y += gl_InstanceIndex;
    gl_Position = camera.proj * camera.view * worldPos;
}