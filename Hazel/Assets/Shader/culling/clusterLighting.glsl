#version 450 core
#include "../common/common.glsl"
#include "../common/constant.glsl"
#include "../common/intersection.glsl"
#include "../common/math.glsl"
#include "../common/gizmo.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1, binding = 0, rg32ui) uniform  uimage2DArray u_Clusters;   // FORMAT_R32G32_UINT
layout(set = 1, binding = 1) buffer lightIDs{

    uint lightID[];

}u_lightIDs;

#define THREAD_SIZE_X 8
#define THREAD_SIZE_Y 8
#define THREAD_SIZE_Z 1
layout (local_size_x = THREAD_SIZE_X, 
		local_size_y = THREAD_SIZE_Y, 
		local_size_z = THREAD_SIZE_Z) in;

void main()
{
    uvec3 globalID = gl_GlobalInvocationID;
    Camera camera = GetCamera();
    float w = camera.width;
    float h = camera.height;
	uint clusterX = uint((w + LIGHT_CLUSTER_GRID_SIZE - 1) / LIGHT_CLUSTER_GRID_SIZE);
    uint clusterY = uint((h + LIGHT_CLUSTER_GRID_SIZE - 1) / LIGHT_CLUSTER_GRID_SIZE);
    uint clusterZ = LIGHT_CLUSTER_DEPTH;
    uint clusterCount = clusterX * clusterY * clusterZ;


    if (globalID.x >= clusterX || 
        globalID.y >= clusterY || 
        globalID.z >= clusterZ) {
        return;
    }

    //////////////////////////////////////// 计算每个簇的世界坐标（先计算NDC坐标下8个顶点位置，转到世界空间即可） ///////////////////////////////////////////
    float ndcMinx = float(globalID.x) / float(clusterX) *2 -1;
    float ndcMiny = float(globalID.y) / float(clusterY) *2 -1;
    float ndcMaxx = (float(globalID.x) + 1) / float(clusterX) *2 -1;
    float ndcMaxy = (float(globalID.y) + 1) / float(clusterY) *2 -1;

    // 线性划分
    float minZ = float(globalID.z) / float(clusterZ);
    float maxZ = float((globalID.z + 1)) / float(clusterZ);

    float near = camera.Near;
    float far = camera.Far;

    // 一长条的簇
    vec3 p0 = SceenToWorld(vec2(ndcMinx, ndcMiny), 0, camera);
    vec3 p1 = SceenToWorld(vec2(ndcMinx, ndcMiny), 1, camera);
    vec3 p2 = SceenToWorld(vec2(ndcMinx, ndcMaxy), 0, camera);
    vec3 p3 = SceenToWorld(vec2(ndcMinx, ndcMaxy), 1, camera);
    vec3 p4 = SceenToWorld(vec2(ndcMaxx, ndcMiny), 0, camera);
    vec3 p5 = SceenToWorld(vec2(ndcMaxx, ndcMiny), 1, camera);
    vec3 p6 = SceenToWorld(vec2(ndcMaxx, ndcMaxy), 0, camera);
    vec3 p7 = SceenToWorld(vec2(ndcMaxx, ndcMaxy), 1, camera);


    // 最终簇的顶点坐标
    vec3 clusterP0 = p0 + minZ * (p1-p0);
    vec3 clusterP1 = p0 + maxZ * (p1-p0);
    vec3 clusterP2 = p0 + minZ * (p3-p2);
    vec3 clusterP3 = p0 + maxZ * (p3-p2);
    vec3 clusterP4 = p0 + minZ * (p5-p4);
    vec3 clusterP5 = p0 + maxZ * (p5-p4);
    vec3 clusterP6 = p0 + minZ * (p7-p6);
    vec3 clusterP7 = p0 + maxZ * (p7-p6);
    vec3 clusterCenter = (clusterP0 + clusterP1 + clusterP2 + clusterP3 + clusterP4 + clusterP5 + clusterP6 + clusterP7) / 8;

    // 构建视锥
    Frustum frustum;
    frustum.planes[0] = calculatePlane(clusterP0, clusterP4,clusterP2, clusterCenter); // 近平面
    frustum.planes[1] = calculatePlane(clusterP1, clusterP3, clusterP5, clusterCenter); // 远平面
    frustum.planes[2] = calculatePlane(clusterP0, clusterP2, clusterP1, clusterCenter); // 左平面
    frustum.planes[3] = calculatePlane(clusterP4, clusterP5,clusterP6, clusterCenter); // 右平面
    frustum.planes[4] = calculatePlane(clusterP0, clusterP1, clusterP4, clusterCenter); // 下平面
    frustum.planes[5] = calculatePlane(clusterP2, clusterP6, clusterP3, clusterCenter); // 上平面

    //////////////////////////////////////// 相加测试与记录数据 ///////////////////////////////////////////
	uint lightIDs[MAX_LIGHTS_PER_CLUSTER];
    uint lightCount = 0;
    for(int i = 0; i < GetPointLightCount(); i++){
	    BoundingSphere sphere = GetPointLight(i).sphere;
        bool isVisiable = FrustumIntersectSphere(frustum, sphere);
        if(isVisiable){
            lightIDs[lightCount++] = i;
            //DrawFrustumEdges(clusterP1, clusterP3,clusterP5, clusterP7, clusterP2, clusterP4,  clusterP6, clusterP0, vec4(1.0, 0.0, 0.0, 1.0));
            //AddGizmoLine(clusterCenter,sphere.center ,vec4(1.0, 0.0, 0.0, 1.0));
        }
	}

    // 记录数据
    uint startOffset = atomicAdd(LIGHTINFO.data.clusterAtomicOffset,lightCount);   // 它返回的是操作前的数据
    for(uint i = 0; i < lightCount; i++){
        u_lightIDs.lightID[startOffset + i] = lightIDs[i];
    }
    imageStore(u_Clusters, ivec3(globalID), uvec4(uvec2(lightCount, startOffset), 0, 0));

}
#endif