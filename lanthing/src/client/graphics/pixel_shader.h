#include <string>

namespace lt
{

const std::string d3d11_pixel_shader = { "\
Texture2D<min16float> luminancePlane : register(t0);\
Texture2D<min16float2> chrominancePlane : register(t1);\
SamplerState theSampler : register(s0);\
\
struct ShaderInput \
{ \
    float4 pos : SV_POSITION;\
    float2 tex : TEXCOORD0;\
};\
\
cbuffer CSC_CONST_BUF : register(b0)\
{ \
    min16float3x3 cscMatrix;\
    min16float3 offsets;\
};\
\
min16float4 main(ShaderInput input) : SV_TARGET \
{\
    min16float3 yuv = min16float3(luminancePlane.Sample(theSampler, input.tex),\
                                  chrominancePlane.Sample(theSampler, input.tex));\
\
    yuv -= offsets;\
\
    yuv = mul(yuv, cscMatrix);\
\
    return min16float4(yuv, 1.0);\
}" };

} // namespace lt