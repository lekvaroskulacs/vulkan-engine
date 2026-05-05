#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 sampleDir;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform samplerCube texSampler;

void main() {
    outColor = texture(texSampler, normalize(sampleDir));
    //outColor = vec4(sampleDir, 1.0);
}