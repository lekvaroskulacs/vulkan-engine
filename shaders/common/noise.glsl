// Classic 2D Perlin gradient noise, no permutation texture needed - gradients come from a
// hash of the integer cell coordinate instead of a lookup table.

// Integer bit-hash (xorshift-multiply finalizer) - exact for any magnitude, unlike a
// sin()-based hash, whose precision degrades for larger inputs (GPU sin() range reduction
// gets less accurate as the argument grows), which then gets massively amplified by the
// usual "* 43758.5453"-style scaling into visibly wrong gradients. Since that degradation
// scales with input magnitude, and each fbmNoise octave samples at a proportionally larger
// coordinate, higher octaves hit it more often - exactly the "more tiles with more octaves"
// symptom this replaces.
uint noiseHash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

vec2 noiseGradient(vec2 cell)
{
    // Offset to keep coordinates non-negative before the float->uint cast below (a negative
    // float's conversion to uint is otherwise implementation-defined) - comfortably larger
    // than any cell coordinate this noise will realistically see.
    uvec2 c = uvec2(cell + vec2(100000.0));
    uint h = noiseHash(c.x * 0x9e3779b9u + noiseHash(c.y));
    // h is a full-range uint; normalizing it to [0, 2*pi) keeps this sin/cos call's argument
    // always small and precise, unlike the hash itself.
    float angle = float(h) * (6.2831853 / 4294967296.0);
    return vec2(cos(angle), sin(angle));
}

float noiseFade(float t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float perlinNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 f = fract(p);

    float n00 = dot(noiseGradient(cell + vec2(0.0, 0.0)), f - vec2(0.0, 0.0));
    float n10 = dot(noiseGradient(cell + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
    float n01 = dot(noiseGradient(cell + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
    float n11 = dot(noiseGradient(cell + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));

    float u = noiseFade(f.x);
    float v = noiseFade(f.y);

    return mix(mix(n00, n10, u), mix(n01, n11, u), v);
}

// Fractal Brownian motion: layers progressively higher-frequency, lower-amplitude noise on
// top of itself for a more natural, less uniformly-blobby result than a single noise octave.
float fbmNoise(vec2 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for(int i = 0; i < octaves; ++i)
    {
        value += perlinNoise(p * frequency) * amplitude;
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
