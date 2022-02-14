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

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    if (input.Lit)
    {
        // Light and shadow the point
        finalColor = float4(baseColor.xyz * Lighting(
                                                input.VertexNormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.VertexNormalWS)), textureResources[0], pointSampler)),
                            baseColor.a);

        // Global illumination
        float3 shadingPoint = input.WorldPosition;

        float3 irradiance = float3(0.0, 0.0, 0.0);
        float3 irradianceNoCheb = float3(0.0, 0.0, 0.0);
        for (int p = 0; p < packedData.x; ++p) // For eaach probe
        {
            float3 dir = ProbePositionsWS[p].xyz - shadingPoint;
            float r = length(dir);

            if (r < packedData.y /* Probe spacing */)
            {
                // This is one of the 8 probes around the shaded point
                dir = normalize(dir);
                
                // Smooth backface
                float weight = pow((dot(dir, input.VertexNormalWS) + 1.0) * 0.5, 2.0) + 0.2;
                
                // Adjacency
                // TODO Trilinear interpolation
                // Weight probe contribution that is nearer to the shaded point higher

                // Visibility
                float threshold = 0.2;
                if(weight < threshold)
                {
                    weight *= pow(weight, 2.0) / pow(threshold, 2.0);
                }
                float2 temp = textureResources[2].SampleLevel(linearSampler, GetProbeTextureCoord(dir, p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING), 0).rg;
                float mean = temp.r;
                float mean2 = temp.g;
                if (r > mean)
                {
                    float variance = abs(pow(mean, 2) - mean2);
                    weight *= variance / (variance + pow(r - mean, 2));
                }

                irradiance += sqrt(textureResources[1].SampleLevel(linearSampler, GetProbeTextureCoord(dir, p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING), 0).rgb * weight);
            }
        }
        
        finalColor += lerp(float4(irradianceNoCheb, 0.0),
                            float4(pow(irradiance, 2.0), 0.0),
                             1.0);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}