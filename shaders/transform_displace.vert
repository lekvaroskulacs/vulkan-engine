#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform GameObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D heightMap;

layout(set = 1, binding = 1) uniform sampler2D heightMapNormals
;
void main() {
    vec3 displacedPos = inPosition;
    displacedPos.y += texture(heightMap, inTexCoord).r * 100.0;

    vec4 worldPos = ubo.model * vec4(displacedPos, 1.0);
    vec4 worldNormal = vec4(inNormal, 0.0) * inverse(ubo.model);
    gl_Position = ubo.proj * ubo.view * worldPos;

    fragTexCoord = inTexCoord;
    outPos = worldPos;
    vec3 normal = texture(heightMapNormals, inTexCoord).xyz;
    outNormal = vec4(normal, 1.0);
}