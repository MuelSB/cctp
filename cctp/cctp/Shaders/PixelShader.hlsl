#include "Octahedral.hlsl"
#include "Common.hlsl"

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    int ProbeCount;
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

SamplerState pointBorderSampler : register(s0);
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
                                                CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.VertexNormalWS)), textureResources[0], pointBorderSampler)),
                            baseColor.a);

        // Global illumination
        float3 irradiance = float3(0.0, 0.0, 0.0);

        for (int p = 0; p < ProbeCount; ++p)
        {
            float3 dir = input.WorldPosition - ProbePositionsWS[p].xyz;
            float r = length(dir);

            if (r < /* probe spacing */ 2.0)
            {
                // This is one of the 8 probes around the shaded point
                dir = normalize(dir);
                
                // Smooth backface
                float weight = (dot(dir, input.VertexNormalWS) + 1.0) * 0.5;
                
                // Adjacency
                // TODO Trilinear interpolation
                // Weight probe contribution that is nearer to the shaded point higher

                // Visibility
                float2 temp = textureResources[2][GetProbeTextureCoord(dir, p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING)].rg;
                float mean = temp.r;
                float mean2 = temp.g;
                if(r > mean)
                {
                    float variance = abs(pow(mean, 2) - mean2);
                    weight *= variance / (variance + pow(r - mean, 2));
                }

                irradiance += sqrt(textureResources[1][GetProbeTextureCoord(dir, p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)].rgb) * weight;
            }
        }
        
        finalColor += float4(irradiance, 0.0);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}