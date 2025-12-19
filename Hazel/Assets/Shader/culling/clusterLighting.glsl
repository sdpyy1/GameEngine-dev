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
        imageStore(u_Clusters, ivec3(globalID), uvec4(uvec2(888, 888), 0, 0));

        return;
    }

    //////////////////////////////////////// 计算每个簇的世界坐标（先计算平面坐标下8个顶点位置，转到世界空间即可） ///////////////////////////////////////////
    float pixelMinX = w / clusterX * globalID.x;
    float pixelMinY = h / clusterY * globalID.y;
    float pixelMaxX = pixelMinX + w / clusterX;
    float pixelMaxY = pixelMinY + h / clusterY;

    // 像素坐标转0~1的归一化UV
    vec2 uvMin = vec2(pixelMinX / w, pixelMinY / h);
    vec2 uvMax = vec2(pixelMaxX / w, pixelMaxY / h);

    float near = camera.Near;
    float far = camera.Far;
	// float minZ 	= (far - near) / LIGHT_CLUSTER_DEPTH * float(globalID.z) + near;
    // float maxZ 	= (far - near) / LIGHT_CLUSTER_DEPTH * float(globalID.z + 1) + near;
    float nz0 = float(globalID.z)     / float(LIGHT_CLUSTER_DEPTH);
    float nz1 = float(globalID.z + 1) / float(LIGHT_CLUSTER_DEPTH);

    float minZ = camera.Near * pow(camera.Far / camera.Near, nz0);
    float maxZ = camera.Near * pow(camera.Far / camera.Near, nz1);
    // 近平面4个点
    vec3 p0 = SceenToWorld(uvMin, minZ, camera);
    vec3 p1 = SceenToWorld(vec2(uvMax.x, uvMin.y), minZ, camera);
    vec3 p2 = SceenToWorld(vec2(uvMin.x, uvMax.y), minZ, camera);
    vec3 p3 = SceenToWorld(uvMax, minZ, camera);

    // 远平面4个点
    vec3 p4 = SceenToWorld(uvMin, maxZ, camera);
    vec3 p5 = SceenToWorld(vec2(uvMax.x, uvMin.y), maxZ, camera);
    vec3 p6 = SceenToWorld(vec2(uvMin.x, uvMax.y), maxZ, camera);
    vec3 p7 = SceenToWorld(uvMax, maxZ, camera);
    vec3 clusterCenter = (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / 8.0;

    // 构建视锥
    Frustum frustum;
    frustum.planes[0] = calculatePlane(p0, p1, p3, clusterCenter); // 近平面
    frustum.planes[1] = calculatePlane(p4, p6, p5, clusterCenter); // 远平面
    frustum.planes[2] = calculatePlane(p0, p2, p6, clusterCenter); // 左平面
    frustum.planes[3] = calculatePlane(p1, p5, p7, clusterCenter); // 右平面
    frustum.planes[4] = calculatePlane(p0, p1, p5, clusterCenter); // 下平面
    frustum.planes[5] = calculatePlane(p2, p7, p6, clusterCenter); // 上平面

    // 先缓存所有相交的灯光ID
	uint lightIDs[MAX_LIGHTS_PER_CLUSTER];
    uint lightCount = 0;
    for(int i = 0; i < GetPointLightCount(); i++){
	    BoundingSphere sphere = GetPointLight(i).sphere;
        bool isVisiable = FrustumIntersectSphere(frustum, sphere);
        if(isVisiable){
            lightIDs[lightCount++] = i;
            AddGizmoLine(clusterCenter,sphere.center ,vec4(1.0, 0.0, 0.0, 1.0));
        }
	}

    // 记录数据
    uint startOffset = atomicAdd(LIGHTINFO.data.clusterAtomicOffset,lightCount);   // 它返回的是操作前的数据
    for(uint i = 0; i < lightCount; i++){
        u_lightIDs.lightID[startOffset + i] = lightIDs[i];
    }
    if(lightCount > 0){
        imageStore(u_Clusters, ivec3(globalID), uvec4(uvec2(lightCount, startOffset), 0, 0));
    }
}











#endif