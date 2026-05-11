#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 sampleDir;

layout(binding = 0) uniform UniformBufferObject {
    mat4 rayDir;
} ubo;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    gl_Position.z = 0.9999;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    sampleDir = (ubo.rayDir * vec4(inPosition, 1.0)).xyz;
}