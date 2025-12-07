#version 450 core
#include "../common/common.glsl"
#include "../common/TAA.glsl"
#include "../common/math.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1,binding = 0,rgba32f) uniform image2D out_texture;
layout(set = 1,binding = 1) uniform texture2D velocityTexture;
layout(set = 1,binding = 2) uniform texture2D historyTexture;
layout(set = 1,binding = 3) uniform texture2D curTexture;
layout(set = 1,binding = 4) uniform texture2D depthTexture;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{
    ivec2 invocID = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imgSize = ivec2(imageSize(out_texture));
    if (invocID.x >= imgSize.x || invocID.y >= imgSize.y) 
        return;
    if(GetPostprocessSetting().TaaSetting.enable == 0){
        vec2 texCoords = (vec2(invocID) + 0.5f) / vec2(imgSize);
        vec3 curColor = texture(sampler2D(curTexture,SAMPLER[0]), texCoords).rgb;
        imageStore(out_texture, invocID, vec4(curColor, 1.0));
        return;
    }
    vec2 texelSize = 1.0 / vec2(imgSize);
    vec2 baseTexCoords = (vec2(invocID) + 0.5f) / vec2(imgSize);
    vec2 jitterUV = GetPostprocessSetting().TaaSetting.UVjetter * texelSize;
    vec2 texCoordsWithJitter = baseTexCoords + jitterUV;

    vec2 ClosestUv = GetClosestFragment(baseTexCoords, texelSize, depthTexture);
    vec2 velocity = texture(sampler2D(velocityTexture,SAMPLER[0]), ClosestUv).rg;
    vec2 historyTexCoords = baseTexCoords - velocity; 

    // 采样当前帧（带抖动）、历史帧（无抖动）颜色
    vec3 historyColor = texture(sampler2D(historyTexture,SAMPLER[0]), historyTexCoords).rgb;
    vec3 curColor = texture(sampler2D(curTexture,SAMPLER[0]), texCoordsWithJitter).rgb;

    const vec2 kOffsets3x3[9] = {
        vec2(-1.0, -1.0), vec2( 0.0, -1.0), vec2( 1.0, -1.0),
        vec2(-1.0,  0.0), vec2( 0.0,  0.0), vec2( 1.0,  0.0),
        vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0)
    };

    // fix 拖影：3x3邻域使用带抖动的UV，且钳位边界
    vec3 AABBMin = RGBToYCoCg(curColor);
    vec3 AABBMax = AABBMin;
    for(int k = 0; k < 9; k++)
    {
        vec2 uvOffset = kOffsets3x3[k] * texelSize;
        vec2 neighborUV = texCoordsWithJitter + uvOffset;
        neighborUV = clamp(neighborUV, 0.0, 1.0);
        vec3 C = RGBToYCoCg(texture(sampler2D(curTexture, SAMPLER[0]), neighborUV).rgb);
        AABBMin = min(AABBMin, C);
        AABBMax = max(AABBMax, C);
    }

    // Clip处理核心逻辑（无问题）
    vec3 HistoryYCoCg = RGBToYCoCg(historyColor);
    vec3 Filtered = (AABBMin + AABBMax) * 0.5f;
    vec3 RayOrigin = HistoryYCoCg;
    vec3 RayDir = Filtered - RayOrigin;
    float epsilon = 1.0 / 65536.0;
    RayDir = mix(vec3(epsilon), RayDir, greaterThan(abs(RayDir), vec3(epsilon)));
    vec3 InvRayDir = 1.0 / RayDir;
    vec3 MinIntersect = (AABBMin - RayOrigin) * InvRayDir;
    vec3 MaxIntersect = (AABBMax - RayOrigin) * InvRayDir;
    vec3 EnterIntersect = min(MinIntersect, MaxIntersect);
    float ClipBlend = max(EnterIntersect.x, max(EnterIntersect.y, EnterIntersect.z));
    ClipBlend = clamp(ClipBlend, 0.0, 1.0);
    vec3 ResultYCoCg = mix(HistoryYCoCg, Filtered, ClipBlend);
    historyColor = YCoCgToRGB(ResultYCoCg);

    float motionLength = length(velocity);
    float blendFactor = Saturate(0.05 + motionLength * 100.0);
    bool valid = (historyTexCoords.x >= 0.0 && historyTexCoords.x <= 1.0) && 
                 (historyTexCoords.y >= 0.0 && historyTexCoords.y <= 1.0);
    if (!valid) {blendFactor = 1.0;}

    if (GetPostprocessSetting().TaaSetting.shaper == 1)
    {
        float strength = GetPostprocessSetting().TaaSetting.shaperStrength;
        vec2 topUV     = clamp(texCoordsWithJitter + vec2(0.0, -texelSize.y), 0.0, 1.0);
        vec2 leftUV    = clamp(texCoordsWithJitter + vec2(-texelSize.x, 0.0), 0.0, 1.0);
        vec2 rightUV   = clamp(texCoordsWithJitter + vec2(texelSize.x, 0.0), 0.0, 1.0);
        vec2 bottomUV  = clamp(texCoordsWithJitter + vec2(0.0, texelSize.y), 0.0, 1.0);

        vec3 topColor    = RGBToYCoCg(texture(sampler2D(curTexture, SAMPLER[0]), topUV).rgb);
        vec3 leftColor   = RGBToYCoCg(texture(sampler2D(curTexture, SAMPLER[0]), leftUV).rgb);
        vec3 centerColor = RGBToYCoCg(curColor);
        vec3 rightColor  = RGBToYCoCg(texture(sampler2D(curTexture, SAMPLER[0]), rightUV).rgb);
        vec3 bottomColor = RGBToYCoCg(texture(sampler2D(curTexture, SAMPLER[0]), bottomUV).rgb);

        vec3 sharpenSum = vec3(0.0);
        sharpenSum += (-1.0 * strength) * topColor;
        sharpenSum += (-1.0 * strength) * leftColor;
        sharpenSum += (5.0 * strength) * centerColor;
        sharpenSum += (-1.0 * strength) * rightColor;
        sharpenSum += (-1.0 * strength) * bottomColor;

        vec3 sharpenedColor = YCoCgToRGB(sharpenSum);
        sharpenedColor = max(sharpenedColor, vec3(0.0));
        curColor = sharpenedColor;
    }

    // HDR需要先转为LDRmix后再恢复， 如果不这样，目前测试发现会出现一个黑点，然后移动后越来越大
    historyColor = ToneMapping(max(historyColor, 0.0f));
    curColor = ToneMapping(max(curColor, 0.0f));
    vec3 mixColor = mix(historyColor, curColor, blendFactor);
    mixColor = InverseToneMapping(max(mixColor, 0.0f));

    imageStore(out_texture, invocID, vec4(mixColor, 1.0));
}

#endif