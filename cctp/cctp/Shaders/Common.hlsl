#ifndef COMMON
#define COMMON

#ifndef PI
#define PI 3.14159274
#endif // PI

struct RayPayload
{
    float3 HitIrradiance;
    float HitDistance;
};

// The number of probes in the probe field
#define PROBE_COUNT 1
// The number of rays traced from a probe. McGuire uses up to 256 rays
#define PROBE_RAY_COUNT 1024
// The amount of texels in a square side to use to store a probes irradiance data in
#define PROBE_WIDTH_IRRADIANCE 8 
// The amount of texels in a square side to use to store a probes visibility data in
#define PROBE_WIDTH_VISIBILITY 16 
// Border size in pixels around each probe's data pack
#define PADDING 1

#endif // COMMON_INCLUDE