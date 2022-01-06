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

#endif // COMMON_INCLUDE