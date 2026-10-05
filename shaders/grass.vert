#version 450

#define GRASS_BUFFER_QUALIFIER readonly
#include "common/scene_light.glsl"
#include "common/grass.glsl"

const int numSegments = 6;
const int numVerticesLOD0 = 15;
const int vertsMaxIndexLOD0 = numVerticesLOD0 - 1;

// Two triangles per quad (rows 0-1 through 5-6), plus one closing triangle for the apex.
// 6 quads * 2 + 1 = 13 triangles = 39 indices. Winding is CCW as seen from +Z; flip if
// backface culling shows the wrong side once this is wired into the actual pipeline.
const int bladeIndices[39] = int[39](
    0, 1, 2,    1, 3, 2,
    2, 3, 4,    3, 5, 4,
    4, 5, 6,    5, 7, 6,
    6, 7, 8,    7, 9, 8,
    8, 9, 10,   9, 11, 10,
    10, 11, 12, 11, 13, 12,
    12, 13, 14
);

const int numRows = numSegments + 1; // 7 rows (0..6), one per quad boundary

mat3 tiltRotation(float tilt)
{
    // Explicit columns, not 9 bare scalars: mat3(a,b,c, d,e,f, g,h,i) fills column-major in
    // GLSL, so typing the values in the usual row-major textbook layout silently builds the
    // transpose of the intended rotation (i.e. rotates by -tilt instead of +tilt).
    return mat3(
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, cos(tilt), sin(tilt)),
        vec3(0.0, -sin(tilt), cos(tilt))
    );
}

// Always exactly (0,0,1): every flat (untilted) blade vertex lies in the z = 0 plane by
// construction, so the cross product of any two of its edges is a pure +z vector regardless
// of the actual per-instance width/height - this used to be derived from the static
// bladeVertices table via cross(), but GLSL doesn't allow calling a user function (like the
// bladeVertexLOD0 below) from a const initializer, so the now-provably-constant result is
// just hardcoded instead.
const vec3 bladeNormal = vec3(0.0, 0.0, 1.0);
const float maxRoundAngle = radians(75.0);

// Rotates flatNormal around the width axis (1,0,0) toward +-x by up to maxRoundAngle,
// simulating a rounded (half-cylinder) cross-section instead of a flat blade. flatNormal
// must already be perpendicular to the width axis, which holds for both the untilted
// (0,0,1) case and the bezier-tangent-derived normal below.
vec3 roundedBladeNormal(vec3 flatNormal, float localX, float halfWidth)
{
    float t = (halfWidth > 0.0) ? clamp(localX / halfWidth, -1.0, 1.0) : 0.0;
    float theta = t * maxRoundAngle;
    return normalize(cos(theta) * flatNormal + sin(theta) * vec3(1.0, 0.0, 0.0));
}

void getBezierControlPoints(GrassInstanceData data, out vec3 p0, out vec3 p1, out vec3 p2)
{
    p0 = tiltRotation(data.tilt) * vec3(0.0, 0.0, 0.0);
    p1 = tiltRotation(data.tilt) * vec3(0.0, data.height * 0.5, -data.bend);
    p2 = tiltRotation(data.tilt) * vec3(0.0, data.height, 0.0);
}

vec3 getBezierPoint(vec3 p0, vec3 p1, vec3 p2, float t)
{
    float u = 1.0 - t;
    return u * u * p0 + 2.0 * u * t * p1 + t * t * p2;
}

void getBezierWeights(float t, out float w0, out float w1, out float w2)
{
    float u = 1.0 - t;
    w0 = u * u;
    w1 = 2.0 * u * t;
    w2 = t * t;
}

// d/dt of the quadratic bezier B(t) = u^2 p0 + 2ut p1 + t^2 p2.
vec3 getBezierTangent(vec3 p0, vec3 p1, vec3 p2, float t)
{
    float u = 1.0 - t;
    return 2.0 * u * (p1 - p0) + 2.0 * t * (p2 - p1);
}

vec3 bladeVertexLOD0(GrassInstanceData data, uint index)
{
    int vertIdx = bladeIndices[index];
    float halfWidth = data.width * 0.5;
    float rowHeight = data.height / float(numSegments);

    vec3 vert;
    if (vertIdx == vertsMaxIndexLOD0)
    {
        vert = vec3(0.0, data.height, 0.0);
    }
    else
    {
        // 2 vertices per row (left/right), so the row is vertIdx/2, not vertIdx itself; and
        // the taper/height fractions need float division, or they truncate to 0 for every
        // row (since vertIdx < vertsMaxIndexLOD0 always here).
        int row = vertIdx / 2;
        float taper = 1.0 - float(row) / float(numRows);
        float x = halfWidth * taper;
        bool isRight = (vertIdx % 2) == 1;
        vert = vec3(isRight ? x : -x, float(row) * rowHeight, 0.0);
    }

    vec3 p0, p1, p2;
    float w0, w1, w2;
    getBezierControlPoints(data, p0, p1, p2);
    float t = vert.y / data.height;
    getBezierWeights(t, w0, w1, w2);

    vec3 bezierPoint = w0 * p0 + w1 * p1 + w2 * p2;
    vert.yz = bezierPoint.yz;

    return vert;
}

vec3 bladeNormalLOD0(GrassInstanceData data, uint index)
{
    int vertIdx = bladeIndices[index];
    float halfWidth = data.width * 0.5;

    float localX;
    float t;
    if (vertIdx == vertsMaxIndexLOD0)
    {
        localX = 0.0;
        t = 1.0;
    }
    else
    {
        int row = vertIdx / 2;
        float taper = 1.0 - float(row) / float(numRows);
        bool isRight = (vertIdx % 2) == 1;
        localX = isRight ? halfWidth * taper : -halfWidth * taper;
        t = float(row) / float(numSegments);
    }

    // p0/p1/p2 come back alread y tilt-rotated; since tiltRotation only mixes y/z (its
    // first column is (1,0,0)), the tangent stays in the y-z plane just like the untilted
    // curve would, so no separate tilt step is needed for the normal either.
    vec3 p0, p1, p2;
    getBezierControlPoints(data, p0, p1, p2);
    vec3 tangent = getBezierTangent(p0, p1, p2, t);

    // Rotate the tangent -90 degrees within the y-z plane to get the surface's flat normal
    vec3 flatNormal = normalize(vec3(0.0, -tangent.z, tangent.y));

    return roundedBladeNormal(flatNormal, localX, halfWidth);
}

layout(location = 0) out vec4 outWorldPos;
layout(location = 1) out vec4 outWorldNormal;

const float widenStrength = 1.5;

void main()
{
    GrassInstanceData data = grassInstanceBuffer.grassInstanceData[gl_InstanceIndex];

    vec3 localPos = bladeVertexLOD0(data, gl_VertexIndex);
    vec3 localNormal = bladeNormalLOD0(data, gl_VertexIndex);

    vec3 facing = vec3(data.facing.x, 0.0, data.facing.y);
    float angle = acos(dot(facing, bladeNormal));

    mat3 rotation = mat3(
        vec3(cos(angle), 0.0, -sin(angle)),
        vec3(0.0, 1.0, 0.0),
        vec3(sin(angle), 0.0, cos(angle))
    );

    localPos = rotation * localPos;
    vec4 worldPos = vec4(localPos + data.position, 1.0);
    vec3 worldNormal = rotation * localNormal;

    // vec3 worldWidthAxis = rotation * vec3(1.0, 0.0, 0.0);
    // mat3 viewRotation = mat3(camera.view);
    // // Get the width axis in view space
    // vec3 viewWidthAxis = normalize(viewRotation * worldWidthAxis);

    // // How much the width axis is foreshortened by pointing toward/away from the camera
    // // (view-space z) instead of lying across the screen - i.e. how edge-on the blade looks.
    // // This is the quantity that actually matters for "is the blade's width about to vanish
    // // on screen", which is subtly different from how perpendicular its normal is to viewDir.
    // float edgeOnFactor = viewWidthAxis.z;

    // // Push the vertex along view-space x (screen-horizontal) to make up for the width lost to
    // // foreshortening, then fold that offset back into worldPos - the view matrix's rotation
    // // part is orthonormal, so its transpose is its inverse, cheaper than a full matrix
    // // inverse - so the fragment shader's lighting sample point matches where the vertex
    // // actually ends up instead of its pre-widening position.
    // float extraOffset = localPos.x * edgeOnFactor * widenStrength;
    // //worldPos.xyz += transpose(viewRotation) * vec3(extraOffset, 0.0, 0.0);

    gl_Position = camera.proj * camera.view * worldPos;

    outWorldPos = worldPos;
    outWorldNormal = vec4(worldNormal, 0.0);
}