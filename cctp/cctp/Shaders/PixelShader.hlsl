struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};

float4 main(VertexOut input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}