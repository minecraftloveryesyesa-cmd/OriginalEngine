#include <metal_stdlib>
using namespace metal;

fragment float4 main_fragment(float3 color [[stage_in]])
{
    return float4(saturate(color), 1.0);
}
