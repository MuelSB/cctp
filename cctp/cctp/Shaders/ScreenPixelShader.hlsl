struct VertexOut
{
    float4 FinalPosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};

struct GBuffer
{
    float4 SceneColor;
    float SceneDepth;
};

SamplerState linearSampler : register(s0, space0);
SamplerState pointSampler : register(s0, space1);
Texture2D<float4> shaderResources[2] : register(t0);

void SetupGBuffer(inout GBuffer gbuffer, in float2 textureCoordinate)
{
    gbuffer.SceneColor = shaderResources[0].Sample(linearSampler, textureCoordinate);
    gbuffer.SceneDepth = shaderResources[1].Sample(linearSampler, textureCoordinate).r;
}

float4 main(VertexOut input) : SV_TARGET
{
    GBuffer gbuffer;
    SetupGBuffer(gbuffer, input.TextureCoordinate);
    
    return gbuffer.SceneColor;

    //return float4(gbuffer.SceneDepth, gbuffer.SceneDepth, gbuffer.SceneDepth, 1.0);
}