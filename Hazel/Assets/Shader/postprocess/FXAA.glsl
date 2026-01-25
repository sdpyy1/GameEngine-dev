#version 450 core
#include "../common/common.glsl"
#include "../common/TAA.glsl"
#include "../common/math.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1,binding = 0,rgba32f) uniform image2D out_texture;
layout(set = 1,binding = 1) uniform texture2D historyTexture;


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
/* 1. 取 1、3、4、5、7 上下左右中间5个采样点来判断颜色差异
    +----+----+----+
    | NW |  N | NE |
    +----+----+----+
    | W  |  M |  E |
    +----+----+----+
    | SW |  S | SE |
    +----+----+----+
*/
vec2 Kernel_Map[] = vec2[9](
    vec2(-1.0, -1.0), vec2( 0.0, -1.0), vec2( 1.0, -1.0),
    vec2(-1.0,  0.0), vec2( 0.0,  0.0), vec2( 1.0,  0.0),
    vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0)
);
#define EDGE_THRESHOLD_MIN  0.0312
#define EDGE_THRESHOLD_MAX  0.125
void main()
{

    vec2 textureSize = imageSize(out_texture);
    ivec2 invocID = ivec2(gl_GlobalInvocationID.xy);
    vec2 stepUV = 1.0 / textureSize;
    vec2 texCoords = (vec2(invocID) + 0.5f) / vec2(textureSize);
    if(invocID.x >= textureSize.x || invocID.y >= textureSize.y){return;}
    if(GetFXAASetting().enable == 0) {
        vec4 outcolor = texture(sampler2D(historyTexture, SAMPLER[0]), texCoords);
        imageStore(out_texture, invocID, outcolor);
        return;
    }
    //////////////////////////////////////////// 1. 边缘检测 ////////////////////////////////////////////
    float lum[9];
    for (int i = 0; i < 9; i++)
    {
        vec2 uv = texCoords + stepUV * Kernel_Map[i];
        lum[i] = RGBtoLuminance(texture(sampler2D(historyTexture, SAMPLER[0]), uv).xyz);
    }
    float NW = lum[0];
    float N  = lum[1];
    float NE = lum[2];
    float W  = lum[3];
    float M  = lum[4];
    float E  = lum[5];
    float SW = lum[6];
    float S  = lum[7];
    float SE = lum[8];

    float maxLum = max(max(max(max(M, N), S), W), E);
    float minLum = min(min(min(min(M, N), S), W), E);

    float Contrast = maxLum - minLum;
    if (Contrast < max(EDGE_THRESHOLD_MIN, maxLum * EDGE_THRESHOLD_MAX))
    {
        vec4 outcolor = texture(sampler2D(historyTexture, SAMPLER[0]), texCoords);
        imageStore(out_texture, invocID, outcolor);
        return;
    }
    if(GetFXAASetting().showEdge == 1) {
        imageStore(out_texture, invocID, vec4(1.0, 0.0, 0.0, 1.0));
        return;
    }

    //////////////////////////////////////////// 2. 计算基于亮度的混合系数计算 ////////////////////////////////////////////
    
    float Filter = 2.0 * (N + E + S + W) + NE + NW + SE + SW;
    Filter = Filter / 12.0;

    Filter = abs(Filter - M);
    Filter = saturate(Filter / Contrast);

    float PixelBlend = smoothstep(0.0, 1.0, Filter);
    PixelBlend = PixelBlend * PixelBlend;


    //////////////////////////////////////////// 3. 计算锯齿方向并混合 ////////////////////////////////////////////
    float Vertical = abs(N + S - 2 * M) * 2+ abs(NE + SE - 2 * E) + abs(NW + SW - 2 * W);
    float Horizontal = abs(E + W - 2 * M) * 2 + abs(NE + NW - 2 * N) + abs(SE + SW - 2 * S);
    bool IsHorizontal = Vertical > Horizontal;  // 垂直方向上亮度变化大，说明锯齿是水平的
    vec2 PixelStep = IsHorizontal ? vec2(0, stepUV.y) : vec2(stepUV.x, 0);
    float Positive = abs((IsHorizontal ? N : E) - M);
    float Negative = abs((IsHorizontal ? S : W) - M);
    if(Positive < Negative) PixelStep = -PixelStep;   // PixelStep往亮度大方向移动

    vec4 outcolor = texture(sampler2D(historyTexture, SAMPLER[0]), texCoords + PixelStep * PixelBlend);

    imageStore(out_texture, invocID, outcolor);
}





#endif