#version 450 core
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
layout(set = 1, rgba32f, binding = 0) uniform image2DArray o_Texture;
layout(set = 1,rgba32f, binding = 1) uniform image2DArray  IN_RayData;
#ifdef COMPUTE_SHADER
layout(local_size_x = DDGI_PROBE_NUM_TEXELS_DISTANCE, local_size_y = DDGI_PROBE_NUM_TEXELS_DISTANCE, local_size_z = 1) in;  
void main(){
	DDGISetting volume = GetDDGISetting();
    uvec3 groupID = gl_WorkGroupID;  // 对于dispatch的ID
    uvec3 invocationID = gl_GlobalInvocationID; // 相对于全局的调用ID
    uvec3 LocalInvocationID = gl_LocalInvocationID; // 相对于组内的调用ID
    bool isBorderTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR + 1)) || (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR + 1));
    uint probeIndex = DDGIGetProbeIndex(invocationID, DDGI_PROBE_NUM_TEXELS_DISTANCE, volume);

    vec4 result = vec4(0.0);

    if(!isBorderTexel){
        uvec3 threadCoords = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR, invocationID.z) + LocalInvocationID - uvec3(1, 1, 0);
        vec2 probeOctantUV = DDGIGetNormalizedOctahedralCoordinates(uvec2(threadCoords.xy), DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR);
        vec3 probeRayDirection = DDGIGetOctahedralDirection(probeOctantUV);
        // 遍历当前探针的所有光线
        for (uint rayIndex = 0; rayIndex < volume.raysPerProbe; rayIndex++){
            vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex, volume.raysPerProbe));
            float weight = max(0.f, dot(probeRayDirection, rayDirection));
            float probeDistanceExponent = 50.f; // TODO:参数
             // Increase or decrease the filtered distance value's "sharpness"
            weight = pow(weight, probeDistanceExponent);
            uvec3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);
            // 光线最大能射到probe间距的1.5倍
            float probeMaxRayDistance = length(volume.gridStep) * 1.5f;
            vec4 storedData = imageLoad(IN_RayData, ivec3(rayDataTexCoords));
            float probeRayDistance = storedData.w;
            probeRayDistance = min(abs(probeRayDistance),probeMaxRayDistance);
            result += vec4(probeRayDistance * weight, (probeRayDistance * probeRayDistance) * weight, 0.f, weight);
        }
//////////////////////////////工程化修正/////////////////////////////////////////////
        float epsilon = float(volume.raysPerProbe);
        epsilon *= 1e-9f;
        result.rgb *= 1.f / (1.f * max(result.a, epsilon));

        // 时域加权混合
        vec4 history = imageLoad(o_Texture,ivec3(gl_GlobalInvocationID));
        float  hysteresis = 0.97; // TODO：混合系数是参数
        if (dot(history, history) == 0) hysteresis = 0.f;
        result = vec4(Lerp(result.rg, history.rg, hysteresis), 0.f, 1.f);
//////////////////////////////工程化修正/////////////////////////////////////////////
        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), result);
        return;

    }else{
        // 边界
        memoryBarrier();        // 所有全局内存类型的屏障
        memoryBarrierShared();  // 对 shared 内存屏障
        barrier();   //这里有一个同步点 


        bool isCornerTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_DISTANCE - 1)) && (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_DISTANCE - 1));
        bool isRowTexel = (LocalInvocationID.x > 0 && LocalInvocationID.x < (DDGI_PROBE_NUM_TEXELS_DISTANCE - 1));

        uvec3 copyCoordinates = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_DISTANCE, groupID.y * DDGI_PROBE_NUM_TEXELS_DISTANCE, invocationID.z);

        if(isCornerTexel)
        {
            copyCoordinates.x += LocalInvocationID.x > 0 ? 1 : DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR;
            copyCoordinates.y += LocalInvocationID.y > 0 ? 1 : DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR;
        }
        else if(isRowTexel)
        {
            copyCoordinates.x += (DDGI_PROBE_NUM_TEXELS_DISTANCE - 1) - LocalInvocationID.x;
            copyCoordinates.y += LocalInvocationID.y + ((LocalInvocationID.y > 0) ? -1 : 1);
        }
        else // Column Texel
        {
            copyCoordinates.x += LocalInvocationID.x + ((LocalInvocationID.x > 0) ? -1 : 1);
            copyCoordinates.y += (DDGI_PROBE_NUM_TEXELS_DISTANCE - 1) - LocalInvocationID.y;
        }

        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), imageLoad(o_Texture, ivec3(copyCoordinates)));
    }

}

#endif