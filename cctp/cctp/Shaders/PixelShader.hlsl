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

SamplerState pointSampler : register(s0);
Texture2D<float4> shadowMap : register(t0);

float CalculateShadow(float4 lightSpacePosition)
{
    // Perspective division (Unnecessary with an orthographic projection)
    float3 coord = lightSpacePosition.xyz / lightSpacePosition.w;
    
    // Transform from [-1, 1] range to [0, 1]
    coord = mul(coord, 0.5) + 0.5;
    
    float depth = shadowMap.Sample(pointSampler, coord.xy).r;
    return coord.z > depth ? 1.0 : 0.0;
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
    
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;

    float4 finalColor = float4(0.0, 0.0, 0.0, 1.0);

    if(input.Lit)
    {
        finalColor = float4(baseColor.xyz * Lighting(input.VertexNormalWS, input.LightVectorWS, input.CameraVectorWS, CalculateShadow(input.LightSpacePosition)), 
                            baseColor.a);
    }
    else
    {
        finalColor = baseColor;
    }
    
    return finalColor;
}