#version 450

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 2) uniform Light {
     vec4 position;
     vec4 powerDensity;
} light;

layout(set = 0, binding = 0) uniform Camera {
     mat4 rayDir;
     vec4 position;
} camera;

struct SceneLight {
     vec4 position;
     vec4 colorIntensity;
};

layout(set = 0, binding = 1) buffer LightList {
     uint count;
     SceneLight lights[64];
} sceneLights;

const float pi = 3.1415;

vec3 brdf(
     vec3 normal,
     vec3 lightDir,
     vec3 viewDir) {
     float cosa = dot(lightDir, normal);
     float cosb = dot(viewDir, normal);  
     
     return
          vec3(0.6, 0.6, 0.2) / pi +
          pow(clamp(dot(normalize(viewDir + lightDir), normal), 0.0, 1.0), 10.0 * 128.0) * vec3(1.0, 1.0, 1.0) / max(cosa, cosb)
          ;
}

vec3 shade(
     vec3 powerDensity,
     vec3 normal,
     vec3 lightDir,
     vec3 viewDir) {

     float cosa = clamp(dot(lightDir, normal), 0.0, 1.0);
     return powerDensity * cosa * brdf(normal, lightDir, viewDir);
}

void main() {
     vec3 normal = normalize(worldNormal.xyz);
     vec3 x = worldPos.xyz / worldPos.w;
     vec3 lightDir = normalize(light.position.xyz - x);
     vec3 lightDiff = light.position.xyz - x;
     float lightDiff2 = dot(lightDiff, lightDiff);
     vec3 viewDir = normalize(camera.position.xyz - x);

     vec3 texColor = texture(texSampler, fragTexCoord).xyz;
     vec3 radiance = shade(light.powerDensity.xyz / lightDiff2, normal, lightDir, viewDir);
     outColor = vec4(texColor * radiance, 1.0);

}