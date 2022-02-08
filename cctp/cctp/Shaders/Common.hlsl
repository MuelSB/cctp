#ifndef COMMON
#define COMMON
#include "Octahedral.hlsl"

#ifndef PI
#define PI 3.14159274
#endif // PI

struct RayPayload
{
    float3 HitIrradiance;
    float HitDistance;
};

// The number of probes in the probe field
#define MAX_PROBE_COUNT 350
// The number of rays traced from a probe. McGuire uses up to 256 rays
#define PROBE_RAY_COUNT 32
// The amount of texels in a square side to use to store a probes irradiance data in
#define IRRADIANCE_PROBE_SIDE_LENGTH 8 
// The amount of texels in a square side to use to store a probes visibility data in
#define VISIBILITY_PROBE_SIDE_LENGTH 16 
// Border size in pixels around each probe's data pack
#define PROBE_PADDING 0

float2 GetProbeTextureCoord(float3 direction, int probeIndex, float singleProbeResultsWidth, int resultPadding)
{
    // Encode the direction to oct texture coordinate in [0, 1] range
    float2 normalizedOctCoordZeroOne = (OctEncode(direction) + 1.0) * 0.5;

    // Calculate the oct coordinate in the dimensions of the probe output texture
    float2 normalizedOctCoordTextureDimensions = (normalizedOctCoordZeroOne * singleProbeResultsWidth);

    // Calculate the top left texel of this probe's output in the texture
    float2 probeTopLeftPosition = float2(
                                         (float) resultPadding + ((float) probeIndex * singleProbeResultsWidth),
                                         (float) resultPadding
                                        );

    return probeTopLeftPosition + normalizedOctCoordTextureDimensions;
}

#endif // COMMON_INCLUDE