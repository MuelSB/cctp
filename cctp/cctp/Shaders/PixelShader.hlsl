#include "Octahedral.hlsl"
#include "Common.hlsl"

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count (x), probe spacing (y), light intensity (z)
}

struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 BaseColor : BASE_COLOR;
    float3 CameraVectorWS : CAMERA_VECTOR_WS;
    float3 NormalWS : NORMAL_WS;
    float3 LightVectorWS : LIGHT_VECTOR_WS;
    uint Lit : Lit;
    float4 LightSpacePosition : POSITION_LS;
    float3 WorldPosition : POSITION_WS;
};

SamplerState pointSampler : register(s0, space0);
SamplerState linearSampler : register(s0, space1);
Texture2D<float> shadowMap : register(t0);
Texture2D<float3> irradianceData : register(t1);
Texture2D<float2> visibilityData : register(t2);

float Square(float x)
{
    return x * x;
}

int3 Get3DVolumeIndex(uint index)
{
    // TODO Read in from constant buffer. Currently hard coded probe volume size
    const uint width  = 6;
    const uint height = 6;
    const uint depth  = 6;

    uint z = index / (width * height);
    index -= (z * width * height);
    uint y = index / width;
    uint x = index % width;
    
    return int3(x, y, z);
}

int Get1DProbePositionIndex(const uint3 xyz)
{
    // TODO Read in from constant buffer. Currently hard coded probe volume size
    const uint width  = 6;
    const uint height = 6;
    const uint depth  = 6;

    return (xyz.z * width * height) + (xyz.y * width) + xyz.x;
}

int GetClosestProbe1DIndex(float3 pos)
{
    // Find probe closest to the shaded point
    float bestLength = 1e38f;
    int bestIndex = -1;
    for (int i = 0; i < (int) packedData.x; ++i) // Sample each probe in the field
    {
        float thisLength = length(ProbePositionsWS[i].rgb - pos);
        if (thisLength < bestLength)
        {
            bestLength = thisLength;
            bestIndex = i;
        }
    }
    return bestIndex;
}

float3 Trilinear(float3 X, int3 adjacentProbes[8])
{
    return (1 - adjacentProbes[0].x) * (1 - adjacentProbes[0].y) * (1 - adjacentProbes[0].z) +
    adjacentProbes[1].x * (1 - adjacentProbes[1].y) * (1 - adjacentProbes[1].z) +
    (1 - adjacentProbes[2].x) * adjacentProbes[2].y * (1 - adjacentProbes[2].z) +
    (1 - adjacentProbes[3].x) * (1 - adjacentProbes[3].y) * adjacentProbes[3].z +
    adjacentProbes[4].x * (1 - adjacentProbes[4].y) * adjacentProbes[4].z +
    (1 - adjacentProbes[5].x) * adjacentProbes[5].y * adjacentProbes[5].z +
    adjacentProbes[6].x * adjacentProbes[6].y * (1 - adjacentProbes[6].z) +
    adjacentProbes[7].x * adjacentProbes[7].y * adjacentProbes[7].z;

}

float3 Irradiance(float3 shadingPoint, float3 shadingPointNormal, float3 viewVector)
{
    float3 irradiance = float3(0.0, 0.0, 0.0);

    // Get closest probe index
    int closestProbe1DIndex = GetClosestProbe1DIndex(shadingPoint);
    
    int3 adjacentProbes[8];
    
    // Get closest probe index as 3D volume index
    adjacentProbes[0] = Get3DVolumeIndex(closestProbe1DIndex);
    
    // Determine what side of the closest probe the shading point is on
    float3 closestProbePos = ProbePositionsWS[closestProbe1DIndex].rgb;
    
    // Take upper right cell of probes. Might not capture the shaded point if the point is to the left, behind or below the closest probe
    // Generate neighbour probe indexes to get 8 probe cage around shaded point
    int neighbourProbeStep = 1;
    adjacentProbes[1] = adjacentProbes[0] + int3(neighbourProbeStep, 0, 0);
    adjacentProbes[2] = adjacentProbes[0] + int3(0, neighbourProbeStep, 0);
    adjacentProbes[3] = adjacentProbes[0] + int3(0, 0, neighbourProbeStep);
    adjacentProbes[4] = adjacentProbes[0] + int3(neighbourProbeStep, 0, neighbourProbeStep);
    adjacentProbes[5] = adjacentProbes[0] + int3(0, neighbourProbeStep, neighbourProbeStep);
    adjacentProbes[6] = adjacentProbes[0] + int3(neighbourProbeStep, neighbourProbeStep, 0);
    adjacentProbes[7] = adjacentProbes[0] + int3(neighbourProbeStep, neighbourProbeStep, neighbourProbeStep);
    
    // Calculate irradiance from 8 adjacent probes
    for (int i = 0; i < 8; ++i)
    {
        int probeIndex = Get1DProbePositionIndex(adjacentProbes[i]);

        float3 X = shadingPoint;
        float3 P = ProbePositionsWS[probeIndex].rgb;
        float3 N = shadingPointNormal;

        float bias = 0.1; // Tweaked for thickness of geometry in scene. Similiar to shadow bias
        X += bias * (N - viewVector);
        float3 dir = P - X;
        float r = length(dir);
        dir *= 1.0 / r; // Normalize dir
        float threshold = 0.2;

        if(dot(dir, N) > 0)
        {
            // Sample irradiance and visibility from this probe
            float2 irradianceTexelIndex = GetProbeTexelCoordinate(dir, probeIndex, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING);
            float2 visibilityTexelIndex = GetProbeTexelCoordinate(dir, probeIndex, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING);

            float weight = Square((dot(dir, shadingPointNormal) + 1.0) * 0.5) + 0.2;
        
            weight *= Trilinear(X, adjacentProbes) + 0.001;
            
            //float2 temp = visibilityData.SampleLevel(linearSampler, visibilityTexelIndex / float2(VISIBILITY_TEXTURE_WIDTH, VISIBILITY_TEXTURE_HEIGHT), 0).rg;
            //float mean = temp.r, mean2 = temp.g; // Not currently implemented as mean averages as the ray gen shader only stores the distance and distance squared result for a single ray rather than a bunch
            //if (r > mean)
            //{
            //    float variance = abs(Square(mean) - mean2);
            //    weight *= variance / (variance + Square(r - mean));
            //}

            //if (weight < threshold)
            //    weight *= Square(weight) / Square(threshold);

           irradiance += irradianceData.SampleLevel(linearSampler, irradianceTexelIndex / float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), 0) * weight;
        }
    }

    return irradiance;
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;

    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    [branch]
    if (input.Lit)
    {
        // Light and shadow the point
        float shadow = CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.NormalWS)), shadowMap, pointSampler);
        finalColor = float4(baseColor.xyz * Lighting(
                                                input.NormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                shadow,
                                                packedData.z),
                            baseColor.a);

        // Diffuse global illumination
        //finalColor.rgb = finalColor.rgb + Irradiance(input.WorldPosition, input.NormalWS);
        finalColor.rgb = Irradiance(input.WorldPosition, input.NormalWS, input.CameraVectorWS);
    }
    else
    {
        finalColor = baseColor;
    }

    // Gamma correct
    float gamma = 2.2;
    finalColor = pow(finalColor, 1.0 / gamma);
    
    return finalColor;
}