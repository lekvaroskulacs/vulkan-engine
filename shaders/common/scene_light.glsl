 struct SceneLight {
    vec4 position;
    vec4 colorIntensity;
};

layout(set = 0, binding = 0) uniform Camera {
    mat4 rayDir;
    vec4 position;
} camera;

layout(set = 0, binding = 1) readonly buffer LightList {
    uint count;
    SceneLight lights[64];
} sceneLights;
