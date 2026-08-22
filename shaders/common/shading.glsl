#include "common/scene_light.glsl"

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

vec3 iterateLights(vec3 worldPosition, vec3 normal)
{
    vec3 viewDir = normalize(camera.position.xyz - worldPosition);
    vec3 radiance = vec3(0.0);
    for (int i = 0; i < sceneLights.count; i++)
    {
        vec3 lightDiff = sceneLights.lights[i].position.xyz - worldPosition;
        vec3 lightDir = normalize(lightDiff);
        float lightDist2 = dot(lightDiff, lightDiff);
        radiance += shade(sceneLights.lights[i].colorIntensity.xyz / lightDist2, normal, lightDir, viewDir);
    }
    return radiance;
}
