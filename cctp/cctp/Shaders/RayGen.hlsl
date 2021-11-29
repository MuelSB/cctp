RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> Output : register(u0);

[shader("raygeneration")]
void RayGen()
{
    uint2 currentPixel = DispatchRaysIndex().xy;
    Output[currentPixel] = float4(1.0f, 1.0f, 1.0f, 1.0f);
}