#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(binding = 0) uniform GameObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 2) uniform Light {
    vec4 position;
    vec4 powerDensity;
    mat4 shadowViewProj;
} light;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    //gl_Position = vec4(1.0, 0.0, 0.0, 1.0); 
    gl_Position = light.shadowViewProj * worldPos;  
}