#version 450 core
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
#include "../common/gizmo.glsl"
/*
每个线程处理一个探针的8*8区域的一个像素
*/
layout(set = 1, rgba32f, binding = 0) uniform image2DArray o_Texture;
layout(set = 1, rgba32f, binding = 1) uniform image2DArray  IN_RayData;
#ifdef COMPUTE_SHADER
layout(local_size_x = DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, local_size_y = DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, local_size_z = 1) in;
void main(){
	DDGISetting volume = GetDDGISetting();
    uvec3 groupID = gl_WorkGroupID;  // 对于dispatch的ID
    uvec3 invocationID = gl_GlobalInvocationID; // 相对于全局的调用ID
    uvec3 LocalInvocationID = gl_LocalInvocationID; // 相对于组内的调用ID
    bool isBorderTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR + 1)) || (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR + 1));
    uint probeIndex = DDGIGetProbeIndex(invocationID, DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, volume);

    // 不是边界，就需要计算了 
    if(!isBorderTexel){
        vec3 result = vec3(0.0);
        float sumWeight = 0.0;
        uvec3 threadCoords = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, invocationID.z) + LocalInvocationID - uvec3(1, 1, 0);
        vec2 probeOctantUV = DDGIGetNormalizedOctahedralCoordinates(uvec2(threadCoords.xy), DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR);
        vec3 probeRayDirection = DDGIGetOctahedralDirection(probeOctantUV);

        // 遍历当前探针的所有光线
        for (uint rayIndex = 0; rayIndex < volume.raysPerProbe; rayIndex++){
            // 获取ray的方向
            vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe));   // TODO: 优化点，因为每个探针计算这个都是一模一样的
            // 当前要存储的方向与这跟光线的cos，两个方向夹角越小，权重越大
            float weight = max(0.0, dot(probeRayDirection, rayDirection));
            // 解析RayData中的信息
            uvec3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);  
            vec4 storedData = imageLoad(IN_RayData, ivec3(rayDataTexCoords));
            vec3 probeRayRadiance = storedData.xyz;
            float probeRayDistance = storedData.w;
            if(probeRayDistance < 0){  // 击中的是背面
                continue;
            }
            result += probeRayRadiance * weight;
            sumWeight += weight;
        }

        // epsilon避免/0
        float epsilon = float(volume.raysPerProbe);
        epsilon *= 1e-9f;
        result *= 1.f / (2.f * max(sumWeight, epsilon));  // 蒙特卡洛积分应该是除以样本个数，为了减少方差这里除的是余弦权重，为了期望一致，还需要/2
        
        // 时域加权混合
        vec4 history = imageLoad(o_Texture,ivec3(gl_GlobalInvocationID));
        float  hysteresis = 0.97; // TODO：混合系数是参数
        if (dot(history, history) == 0) hysteresis = 0.f;
        result = mix(result, history.rgb, hysteresis);
        // 可视化探针颜色
        if(volume.visulaize == 1 && LocalInvocationID == uvec3(1,1,0)){
            uvec3 prebeCoords = DDGIGetProbeCoords(probeIndex, volume);
            vec3 probePosition = DDGIGetProbeWorldPosition(prebeCoords, volume);
            AddGizmoSphere(probePosition,0.5,vec4(result,1));
        }

        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), vec4(result,1));
    }else{
        // 边界
        memoryBarrier();        // 所有全局内存类型的屏障
        memoryBarrierShared();  // 对 shared 内存屏障
        barrier();   //这里有一个同步点 

        bool isCornerTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1)) && (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1));
        bool isRowTexel = (LocalInvocationID.x > 0 && LocalInvocationID.x < (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1));

        uvec3 copyCoordinates = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, invocationID.z);

        if(isCornerTexel)
        {
            copyCoordinates.x += LocalInvocationID.x > 0 ? 1 : DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR;
            copyCoordinates.y += LocalInvocationID.y > 0 ? 1 : DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR;
        }
        else if(isRowTexel)
        {
            copyCoordinates.x += (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1) - LocalInvocationID.x;
            copyCoordinates.y += LocalInvocationID.y + ((LocalInvocationID.y > 0) ? -1 : 1);
        }
        else // Column Texel
        {
            copyCoordinates.x += LocalInvocationID.x + ((LocalInvocationID.x > 0) ? -1 : 1);
            copyCoordinates.y += (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1) - LocalInvocationID.y;
        }

        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), imageLoad(o_Texture, ivec3(copyCoordinates)));
    }
}

#endif