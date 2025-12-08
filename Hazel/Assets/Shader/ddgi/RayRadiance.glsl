#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
struct Payload {
    vec3 albedo;
	vec3 worldPosition;
	vec3 normal;
};
#ifdef RAYGEN_SHADER

layout(set = 1, binding = 0, rgba32f) uniform image2DArray  out_RAYDATA; // radiance(3) + hitT(1)

layout(location = 0) rayPayloadEXT Payload payload ;   // 必须有rayPayloadEXT前缀

void main() 
{
    // 获取要处理的Volume信息，这里简化一下，全局只有一个Volume
	DDGISetting volume = GetDDGISetting();
    uint rayIndex = gl_LaunchIDEXT.x; // 哪条光线
    uint probePlaneIndex = gl_LaunchIDEXT.y; // 一层中的哪个
    uint planeIndex = gl_LaunchIDEXT.z; // 哪一层
	uint probeCountPrePlane = DDGIGetProbesPerPlane(volume.probeCount); // 获取一层有几个的probe
	uint probeIndex = (planeIndex * probeCountPrePlane) + probePlaneIndex; // 当前处理的probe的全局索引
	uvec3 probeCoords = DDGIGetProbeCoords(probeIndex,volume); // 获取探针在探针网格的3D坐标
	// probeIndex = DDGIGetScrollingProbeIndex(probeCoords, volume); // TODO: 滚动探针

	// 获取探针的世界坐标和射线方向
	vec3 probeWorldPosition = DDGIGetProbeWorldPosition(probeCoords, volume);
	vec3 rayDirection = RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe);
	// 获取这条光线的存储位置（TODO: 还没理清楚后续需要如何存储数据,先抄的代码）
	uvec3 outputCoords = DDGIGetRayDataTexelCoords(rayIndex,probeIndex,volume);

	// 启动射线
	traceRayEXT(TLAS, 					// acceleration structure
		gl_RayFlagsOpaqueEXT,       	// rayFlags 控制光线的行为，比如是否忽略背面、是否启用 any-hit、是否可用 conservative tracing 等
		0xFF,           				// cullMask 0xFF → 匹配所有实例
		0,              				// sbtRecordOffset 索引到 Shader Binding Table (SBT) 的起始记录位置
		1,              				// sbtRecordStride SBT 中每条记录的间隔（单位是记录数，不是字节）
		0,              				// missIndex 当光线没有击中任何几何体时，使用 SBT 中 miss shader 的索引
		probeWorldPosition.xyz,     	// ray origin
		0,       						// ray min range
		rayDirection.xyz,  				// ray direction
		MAX_RAY_TRACING_DISTANCE,       // TODO:这个应该换成DDGI自己的参数设置
		0               				// payload (location = 0) payload的位置
  	);



	// 最终存储rayData radiance(3) + hitT(1)
	imageStore(out_RAYDATA, ivec3(outputCoords), vec4(payload.albedo.xyz, 1));
}

#endif

#ifdef RAYCLOSEST_HIT_SHADER
/*

	Hit是具体到Mesh的某一个三角形以及击中点，并且会给hitAttributeEXT来表示重心坐标，需要手动插值来计算击中点的信息
	gl_WorldRayTmaxEXT 可以获得击中的时间 t 
*/
layout(location = 0) rayPayloadInEXT Payload payload;    // rayPayloadInEXT 注意是InEXT

// Hit shader 可以访问击中的 geometry 信息
hitAttributeEXT vec2 attribs;   // 用来求重心坐标，表示击中点对于击中三角形的三个顶点的权重
void main()
{
	uint instanceID       = gl_InstanceCustomIndexEXT;   // 在构建TLAS时，给每个实例分配的ID
	uint primitiveID     = gl_PrimitiveID;  // 击中的三角形索引

	vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);   // 插值需要手动进行

	// 需要根据实例ID和三角形索引去Bindless找对应三个顶点的信息，再根据barycentrics进行插值


	// 根据插值后的结果计算着色信息
	MaterialInfo material   = GetMaterialInfo(instanceID);


	// 返回颜色
	payload.albedo = material.diffuse.xyz;

	// 或者继续递归
}
#endif

#ifdef RAYMISS_SHADER
/*
	MissShader是当Ray没有击中任何几何体时，会调用MissShader，MissShader可以返回一个颜色，或者继续递归
*/
layout(location = 0) rayPayloadInEXT Payload payload;    // rayPayloadInEXT 注意是InEXT
layout(set = 1, binding = 1) uniform textureCube skyCube;
void main()
{
	// TODO: 这里要改成探针的位置方向，而不是摄像机的位置方向
	// 1. 计算屏幕坐标，并转到NDC坐标
	const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);  // 移动到像素中心
	const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);  // UV坐标
	vec2 ndc = inUV * 2.0 - 1.0; // 屏幕坐标转NDC[-1,1]
	// 2. Ray参数
	vec3 origin = CAMERAINFO.data.position;  // 相机位置
	vec4 target = CAMERAINFO.data.invProj * vec4(ndc.x, ndc.y, 0, 1) ; // 射线终点， 设置在像素NDC坐标，深度最近的位置  并转到View空间
	target /= target.w;   // 透视除法
	target = CAMERAINFO.data.invView * target;
	vec3 direction = normalize(target.xyz - origin.xyz);
    payload.albedo = texture(samplerCube(skyCube,SAMPLER[0]),direction).rgb;
}
#endif