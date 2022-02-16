#include "Octahedral.hlsl"
#include "Common.hlsl"

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count in x and probe spacing in y
}

struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 BaseColor : BASE_COLOR;
    float3 CameraVectorWS : CAMERA_VECTOR_WS;
    float3 VertexNormalWS : NORMAL_WS;
    float3 LightVectorWS : LIGHT_VECTOR_WS;
    uint Lit : Lit;
    float4 LightSpacePosition : POSITION_LS;
    float3 WorldPosition : POSITION_WS;
};

SamplerState pointSampler : register(s0, space0);
SamplerState linearSampler : register(s0, space1);
Texture2D<float4> textureResources[3] : register(t0);

void GetAdjacentProbeIndices(in float3 position, out uint indices[8])
{
    uint currentAdjacentProbeIndex = 0;
    for (int p = 0; p < packedData.x /* Probe count */; ++p)
    {
        float3 probePosition = ProbePositionsWS[p].xyz;
        float3 dir = ProbePositionsWS[p].xyz - position;
        if (length(dir) <= packedData.y /* Probe spacing */)
        {
            indices[currentAdjacentProbeIndex] = p;
            ++currentAdjacentProbeIndex;
            if (currentAdjacentProbeIndex == 8)
                break;
        }
    }
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    [branch]
    if (input.Lit)
    {
        // Light and shadow the point
        finalColor = float4(baseColor.xyz * Lighting(
                                                input.VertexNormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.VertexNormalWS)), textureResources[0], pointSampler)),
                            baseColor.a);

        //// Global illumination
        //float3 shadingPoint = input.WorldPosition;
        
        //// Get 8 adjacent probes to the shading point
        //uint adjacentProbeIndices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        //GetAdjacentProbeIndices(shadingPoint, adjacentProbeIndices);

        //// For each adjacent probe around the shading point
        //float3 irradiance = float3(0.0, 0.0, 0.0);
        //float3 irradianceNoCheb = float3(0.0, 0.0, 0.0);
        //for (uint i = 0; i < 8; ++i)
        //{
        //    uint probeIndex = adjacentProbeIndices[i];
        //    float3 dir = ProbePositionsWS[i].xyz - shadingPoint;
        //    float r = length(dir);
        //    dir *= 1.0 / r;
            
        //    // Smooth backface
        //    float weight = pow((dot(dir, input.VertexNormalWS) + 1.0) * 0.5, 2.0) + 0.2;

        //    // Adjacency
        //    // TODO Trilinear interpolation
        //    // Weight probe contribution that is nearer to the shaded point higher
            
        //    // Visibility
        //    const float threshold = 0.2;
        //    if (weight < threshold)
        //    {
        //        weight *= pow(weight, 2.0) / pow(threshold, 2.0);
        //    }
        //    //float2 temp = textureResources[2].SampleLevel(pointSampler, GetProbeTextureCoord(dir, probeIndex, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING), 0).rg;
        //    float2 temp = textureResources[2][GetProbeTextureCoord(dir, probeIndex, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING)].rg;
        //    float mean = temp.r;
        //    float mean2 = temp.g;
        //    if (r > mean)
        //    {
        //        float variance = abs(pow(mean, 2) - mean2);
        //        weight *= variance / (variance + pow(r - mean, 2));
        //    }
            
        //    //irradiance += sqrt(textureResources[1].SampleLevel(pointSampler, GetProbeTextureCoord(dir, probeIndex, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING), 0).rgb * weight);
        //    irradiance += sqrt(textureResources[1][GetProbeTextureCoord(dir, probeIndex, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)].rgb * weight);
        //}

        //return float4(irradiance, 1.0);

        //finalColor += lerp(float4(irradianceNoCheb, 0.0),
        //                   float4(pow(irradiance, 2.0), 0.0),
        //                   1.0);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}