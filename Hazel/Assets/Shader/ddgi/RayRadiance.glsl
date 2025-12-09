#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
#include "../common/gizmo.glsl"
struct Payload {
    vec3 albedo;
	float roughness; 
	vec3 worldPosition;
	float metallic;
	vec3 normal;
	float  hitT;

};
#ifdef RAYGEN_SHADER


layout(location = 0) rayPayloadEXT Payload payload;

layout(set = 1, binding = 0, rgba32f) uniform image2DArray  out_RAYDATA; // radiance(3) + hitT(1)
layout(set = 1, binding = 1) uniform texture2DArray u_DirShadowMapTexture;
layout(set = 1, binding = 2) uniform textureCube u_PointShadowMapTexture;
struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
} m_Params;

#include "../common/shadow.glsl"
#include "../common/light.glsl" 


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
	vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe));

	// // 可视化射线
	// if(volume.visulaize == 1 && probeCoords == uvec3(4,4,4)){
	// 	AddGizmoLine(probeWorldPosition, probeWorldPosition + rayDirection,vec4(1));
	// }

	// 获取这条光线最终在纹理中的存储位置 x: RayIndex y: probeIndexInLayer z:layerIndex
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

	// 计算每条光线的Radiance
	if(payload.hitT< 0.f){
		// 直接存储采样天空盒的结果
		imageStore(out_RAYDATA, ivec3(outputCoords), vec4(payload.albedo.xyz, 1));
		return;
	}

	// 计算击中点的直接光
	float shadowScale = 1.0;
	uint cascadeIndex = 0;
	m_Params.Albedo = payload.albedo;
	m_Params.Metalness = payload.metallic;
    m_Params.Roughness = payload.roughness;
    m_Params.Normal = payload.normal;
	m_Params.View = normalize(probeWorldPosition - payload.worldPosition); 
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	const vec3 Fdielectric = vec3(0.04);
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	//直接光应该是只需要计算漫反射分量，不需要镜面反射和阴影  // TODO:但是RTXGI好像做了阴影判断
	vec3 diffuse = CalculateDirLightsOnlyDiffuse(F0) + CalculatePointLightsOnlyDiffuse(F0, payload.worldPosition) + CalculateSpotLightsOnlyDiffuse(F0, payload.worldPosition);
	
	// 读取探针信息，获取间接Irrandiance
	vec3 irradiance = vec3(0);
	// float3 surfaceBias = DDGIGetSurfaceBias(payload.normal, ray.Direction, volume);  // TODO:???

	// 离Volume越远，权重越小
	float volumeBlendWeight = DDGIGetVolumeBlendWeight(payload.worldPosition, volume);
	if (volumeBlendWeight > 0){

		// TODO: 就差在这里计算间接光
        // irradiance = DDGIGetVolumeIrradiance(
        //     payload.worldPosition,
        //     surfaceBias,
        //     payload.normal,
        //     volume,
        //     resources);
	}
	// Perfectly diffuse reflectors don't exist in the real world.
    // Limit the BRDF albedo to a maximum value to account for the energy loss at each bounce.
    float maxAlbedo = 0.9f;
	
    vec3 radiance = diffuse + ((min(payload.albedo, vec3(maxAlbedo, maxAlbedo, maxAlbedo)) / PI) * irradiance * volumeBlendWeight);

	// 最终存储rayData radiance(3) + hitT(1)
	imageStore(out_RAYDATA, ivec3(outputCoords), vec4(radiance, payload.hitT));
}

#endif

#ifdef RAYCLOSEST_HIT_SHADER
layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT vec2 attribs;
void main()
{
	uint instanceID       = gl_InstanceCustomIndexEXT;   // 在构建TLAS时，给每个实例分配的ID
	uint primitiveID     = gl_PrimitiveID;  // 击中的三角形索引

	vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y); 

	// 收集MeshInfo
	mat4 model = GetModelMatrix(instanceID);
	uvec3 triangleIndex = GetTriangleIndex(instanceID, primitiveID);
	vec4 position = GetTrianglePosition(instanceID,triangleIndex,barycentrics);
	vec3 meshNormal = GetTriangleMeshNormal(instanceID,triangleIndex,barycentrics);
	vec4 tangent = GetTriangleTangent(instanceID,triangleIndex,barycentrics);

	vec3 worldNormal = GetWorldNormal(meshNormal,model);
	vec4 worldTangent = GetWorldTangent(tangent,model);
	// vec3 color = FetchTriangleColor(objectID, index, barycentrics);   不使用顶点颜色
	vec2 texCoord  = GetTriangleTexCoord(instanceID, triangleIndex, barycentrics);    
    vec4 worldPos       = model * position; 
	MaterialInfo material   = GetMaterialInfo(instanceID);
	vec4 albedo = GetDiffuse(material,texCoord);
	vec3 normal = GetNormal(material, texCoord, worldNormal, worldTangent);
    vec4 emission = GetEmission(material,texCoord);
	albedo += emission;
	float roughness = GetRoughness(material,texCoord);
    float metallic = GetMetallic(material, texCoord);

	payload.worldPosition = worldPos.xyz;
	payload.normal = normal;
	payload.roughness = roughness;
	payload.metallic = metallic;
	payload.albedo = albedo.rgb;
	payload.hitT = gl_HitTEXT;
}
#endif

#ifdef RAYMISS_SHADER
layout(location = 0) rayPayloadInEXT Payload payload;    // rayPayloadInEXT 注意是InEXT
layout(set = 1, binding = 3) uniform textureCube skyCube;
void main()
{
	vec3 rayDir = normalize(gl_WorldRayDirectionEXT);
    payload.albedo = texture(samplerCube(skyCube,SAMPLER[0]),rayDir).rgb;
	payload.hitT = -1;
}
#endif