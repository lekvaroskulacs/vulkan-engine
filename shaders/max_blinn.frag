#version 450

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform Light {
    vec4 position;
    vec4 powerDensity;
    mat4 shadowViewProj;
} light;

layout(binding = 3) uniform Camera {
    mat4 rayDir;
    vec4 position;
} camera;

layout(binding = 4) uniform sampler2D shadowMap;

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

float linearizeDepth(float depth)
{
  float n = 0.1;
  float f = 1000.0;
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    vec3 normal = normalize(worldNormal.xyz);
    vec3 x = worldPos.xyz / worldPos.w;
    vec3 lightDir = normalize(light.position.xyz - x);
    vec3 lightDiff = light.position.xyz - x;
    float lightDiff2 = dot(lightDiff, lightDiff);
    vec3 viewDir = normalize(camera.position.xyz - x);

    vec3 radiance = shade(light.powerDensity.xyz / lightDiff2, normal, lightDir, viewDir);

    vec4 lightSpacePos = light.shadowViewProj * worldPos;
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    // We need xy to be in NDC, but z is already in [0, 1] range in vulkan
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.005 * (1.0 - dot(normal, -lightDir)), 0.001);

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        outColor = vec4(radiance, 1.0);
    }
    else if (currentDepth > closestDepth + bias)
    {
        float shadowFactor = 0.8;
        vec3 shadowColor = vec3(0.0, 0.0, 0.0);
        vec3 color = shadowColor * shadowFactor + radiance * (1 - shadowFactor);
        outColor = vec4(radiance * 0.2, 1.0);
    }
    else
    {
        outColor = vec4(radiance, 1.0);
    }
    
    //outColor = vec4(vec3(linearizeDepth(currentDepth)), 1.0);

}