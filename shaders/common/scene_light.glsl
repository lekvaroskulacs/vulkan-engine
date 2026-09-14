 struct SceneLight {
    vec4 position;
    vec4 colorIntensity;
    float radius;
};

// Keep in sync with LightBuffer::MAX_LIGHTS in src/include/engine/lights/lights.h.
const uint maxLights = 256;

layout(set = 0, binding = 0) uniform Camera {
    mat4 rayDir;
    vec4 position;
    mat4 view;
    mat4 proj;
    vec4 nearFar;
    vec4 screenSize;
} camera;

layout(set = 0, binding = 1) readonly buffer LightList {
    uint count;
    SceneLight lights[maxLights];
} sceneLights;
