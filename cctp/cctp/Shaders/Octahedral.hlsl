#ifndef OCTAHEDRAL
#define OCTAHEDRAL

float2 signNotZero(float2 v)
{
    return float2((v.x >= 0.0f) ? 1.0f : -1.0f, (v.y >= 0.0f) ? 1.0f : -1.0f);
}

/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
float2 octEncode(in float3 v)
{
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    float2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0)
    {
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    }
    return result;
}


/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
float3 octDecode(float2 o)
{
    float3 v = float3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0)
    {
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    }
    return normalize(v);
}

float SignNotZero(float f)
{
    return (f >= 0.0f) ? 1.0f : -1.0f;
}

float2 OctEncode(float3 v)
{
    float2 result = v.xy * (1.0f / (abs(v.x) + abs(v.y) + abs(v.z)));

    if (v.z <= 0.0f)
    {
        result.x = (1.0f - abs(result.y)) * SignNotZero(result.x);
        result.y = (1.0f - abs(result.x)) * SignNotZero(result.y);
    }
    
    return result;
}

float3 OctDecode(float2 c)
{
    float cx = c.x;
    float cy = c.y;
    float sumXY = (abs(cx) + abs(cy));
    float cz = 1.0f - sumXY;
    
    if (sumXY > 1)
    {
        cx = sign(cx) * (1.0f - abs(cy));
        cy = sign(cy) * (1.0f - abs(cx));
        cz = -cz;
    }

    return normalize(float3(cx, cy, cz));
}

#endif