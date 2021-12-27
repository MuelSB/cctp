#include "Common.hlsl"
#include "Octahedral.hlsl"

#define SECTORS 7
#define STACKS 7
#define PROBE_RAY_COUNT 64
#define RADIUS 1.0f // Using radius 1 to remove a normalize during direction generation
#define PROBE_WIDTH 8 // The amount of texels to use to store a probes data in
#define PADDING 1

RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> Output : register(u0);

cbuffer PerFrameConstants : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4 ProbePosition;
};

void GenerateRayDirections(out float3 rayDirections[PROBE_RAY_COUNT])
{
    static const float sectorStep = 2.0f * PI / SECTORS;
    static const float stackStep = PI / STACKS;
    
    float sectorAngle;
    float stackAngle;
    float xy;

    int directionIndex = 0;
    for (int i = STACKS; i >= 0; i--)
    {
        stackAngle = PI / 2 - i * stackStep;
        xy = RADIUS * cos(stackAngle);

        for (int j = 0; j <= SECTORS; j++)
        {
            sectorAngle = j * sectorStep;

            rayDirections[directionIndex] = float3(
                xy * cos(sectorAngle),
                xy * sin(sectorAngle),
                RADIUS * sin(stackAngle)
            );
            ++directionIndex;
        }
    }
}

float3 sphericalFibonacci(float i, float n)
{
    const float PHI = sqrt(5) * 0.5 + 0.5;
#define madfrac(A, B) ((A)*(B)-floor((A)*(B)))
    float phi = 2.0 * PI * madfrac(i, PHI - 1);
    float cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sinTheta = sqrt(saturate(1.0 - cosTheta * cosTheta));

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);

#undef madfrac
}

[shader("raygeneration")]
void RayGen()
{
    RayPayload payload =
    {
        float3(0.0f, 0.0f, 0.0f)
    };
    
    for (int i = 0; i < PROBE_WIDTH; ++i)
    {
        for (int j = 0; j < PROBE_WIDTH; ++j)
        {
            float2 texel = float2(i, j);
            Output[texel + PADDING] = float4(1.0f, 0.0f, 1.0f, 1.0f);
        }
    }
    
    // Generate ray directions
    float3 rayDirections[PROBE_RAY_COUNT];
    GenerateRayDirections(rayDirections);
    
    // Shoot rays from each probe. There is only 1 probe currently
    static const int probeCount = 1;
    for (int p = 0; p < probeCount; ++p)
    {
        for (int r = 0; r < PROBE_RAY_COUNT; ++r)
        {
            //float3 rayDirection = rayDirections[r];
            float3 rayDirection = sphericalFibonacci(r, (float)PROBE_RAY_COUNT);

            RayDesc ray;
            ray.Origin = ProbePosition.xyz;
            ray.Direction = rayDirection;
            ray.TMin = 0.1f;
            ray.TMax = 1e+38f;

            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            // Have texture coordinate in 0 - 1 range.
            // Have an output texture that is 8x8 for each probe.

            float2 normalizedOctCoordZeroOne = (octEncode(normalize(rayDirection)) + float2(1.0f, 1.0f)) * 0.5f;
            float2 normalizedOctCoordTextureDimensions = (normalizedOctCoordZeroOne * (float) PROBE_WIDTH);

            float2 probeTopLeftPosition = float2((float) PADDING, (float) PADDING);
            
            Output[probeTopLeftPosition + normalizedOctCoordTextureDimensions] = float4(payload.HitColor, 1.0f);
            //Output[probeTopLeftPosition + normalizedOctCoordTextureDimensions] = float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
    
    // Display green pixels in the corners of the output texture
    Output[int2(0, 0)] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    Output[int2(0, 31)] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    Output[int2(31, 31)] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    Output[int2(31, 0)] = float4(0.0f, 1.0f, 0.0f, 1.0f);
}