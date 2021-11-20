struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 SurfaceColor : SURFACE_COLOR;
};

float4 main(VertexOut input) : SV_TARGET
{
    return input.SurfaceColor;
}