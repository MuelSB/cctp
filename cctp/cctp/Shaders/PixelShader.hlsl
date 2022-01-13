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
};

SamplerState pointBorderSampler : register(s0);
Texture2D<float4> shadowMap : register(t0);

float CalculateShadow(float4 lightSpacePosition, float bias, float LdotN)
{
    // Transform from [-1, 1] range to [0, 1]
    float3 position = mul(lightSpacePosition.xyz, 0.5f) + 0.5f;
    
    if (position.z > 1.0)
        position.z = 1.0;
    
    float depth = shadowMap.Sample(pointBorderSampler, position.xy).r;
    bias = max(bias * (1.0 - LdotN), 0.005);
    return (depth + bias) < position.z ? 0.0f : 1.0f;
}

float3 Lighting(float3 vertexNormalWS, float3 lightVectorWS, float3 cameraVectorWS, float shadow)
{
    // Ambient
    const float3 ambient = float3(0.1, 0.1, 0.1);
    
    // Diffuse
    const float3 lightColor = float3(1.0, 1.0, 1.0);
    const float3 diffuse = lightColor * saturate(dot(vertexNormalWS, lightVectorWS));
    
    // Specular
    const float3 H = normalize(lightVectorWS + cameraVectorWS);
    const float NoH = saturate(dot(vertexNormalWS, H));
    const float gloss = 256.0;
    const float3 specColor = float3(0.4, 0.4, 0.4);
    const float3 specular = specColor * pow(NoH, gloss);
    
    return shadow * (diffuse + specular) + ambient;
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    if (input.Lit)
    {
        finalColor = float4(baseColor.xyz * Lighting(input.VertexNormalWS, 
                                                input.LightVectorWS, 
                                                input.CameraVectorWS, 
                                                CalculateShadow(input.LightSpacePosition, 0.05, saturate(dot(input.LightVectorWS, input.VertexNormalWS)))),
                            baseColor.a);
    }
    else
    {
        finalColor = baseColor;
    }
    
    return finalColor;
}