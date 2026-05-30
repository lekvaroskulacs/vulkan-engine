#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec3 outLightPos;

layout(binding = 0) uniform GameObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 2) uniform Light {
    vec4 position;
    vec4 powerDensity;
    mat4 shadowView;
    mat4 shadowProj;
} light;

layout(push_constant) uniform PushConsts 
{
	mat4 view;
} pushConsts;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    //gl_Position = vec4(1.0, 0.0, 0.0, 1.0); 
    gl_Position =  light.shadowProj * pushConsts.view * worldPos;  

    outPos = worldPos;
    outLightPos = light.position.xyz;
}