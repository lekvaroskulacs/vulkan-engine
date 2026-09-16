#include "common/scene_light.glsl"
#include "common/cluster.glsl"

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
        vec3 lightDiff = sceneLights.lights[i].position.xyz - worldPosition * sceneLights.lights[i].position.w;
        vec3 lightDir = normalize(lightDiff);
        float lightDist2 = dot(lightDiff, lightDiff);

        float radius = sceneLights.lights[i].radius;
        float window = clamp(1.0 - pow(lightDist2 / (radius * radius), 2.0), 0.0, 1.0);
        float atten = (window * window) / lightDist2;
        if (sceneLights.lights[i].position.w <= 0.01)
        {
            atten = 1.0;
        }

        radiance += shade(sceneLights.lights[i].colorIntensity.xyz * atten, normal, lightDir, viewDir);
    }
    return radiance;
}

// Find the cluster the given worldPosition falls in. This is a reverse
// of the steps made in cluster_build.comp
uint fragmentClusterIndex(vec3 worldPosition)
{
    vec2 screenUV = gl_FragCoord.xy / camera.screenSize.xy;
    uint tileX = min(uint(screenUV.x * float(gridX)), gridX - 1u);
    uint tileY = min(uint(screenUV.y * float(gridY)), gridY - 1u);

    vec4 viewPosition = camera.view * vec4(worldPosition, 1.0);
    float viewZ = max(-viewPosition.z, camera.nearFar.x);
    float near = camera.nearFar.x;
    float far = camera.nearFar.y;
    float sliceF = log(viewZ / near) * float(numSlices) / log(far / near);
    uint tileZ = uint(clamp(sliceF, 0.0, float(numSlices - 1u)));

    return clusterIndex(uvec3(tileX, tileY, tileZ));
}

vec3 iterateLightsClustered(vec3 worldPosition, vec3 normal)
{
    LightPerClusterProperties props = lightGrid.lightsPerCluster[fragmentClusterIndex(worldPosition)];

    vec3 viewDir = normalize(camera.position.xyz - worldPosition);
    vec3 radiance = vec3(0.0);
    for (uint i = 0u; i < props.count; ++i)
    {
        SceneLight light = sceneLights.lights[lightIndices.indices[props.offset + i]];
        vec3 lightDiff = light.position.xyz - worldPosition * light.position.w;
        vec3 lightDir = normalize(lightDiff);
        float lightDist2 = dot(lightDiff, lightDiff);

        float window = clamp(1.0 - pow(lightDist2 / (light.radius * light.radius), 2.0), 0.0, 1.0);
        float atten = (window * window) / lightDist2;
        if (light.position.w <= 0.01)
        {
            atten = 1.0;
        }

        radiance += shade(light.colorIntensity.xyz * atten, normal, lightDir, viewDir);
    }
    return radiance;
}
