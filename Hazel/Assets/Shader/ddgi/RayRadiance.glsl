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

	uint hitKind;
	uint _padding[3];
};
struct ShadowPayLoad{
	float hitT;
	uint _padding[3];
};
#ifdef RAYGEN_SHADER
layout(location = 0) rayPayloadEXT Payload payload;
layout(location = 1) rayPayloadEXT ShadowPayLoad shadowPayload;  // 定义可以定义好几个，但是接收每个Shader只能有一个，要在group中多添加shader，然后rayTraces时指定用哪一个

layout(set = 1, binding = 0, rgba32f) uniform image2DArray  out_RAYDATA; // radiance(3) + hitT(1)
layout(set = 1, binding = 1) uniform texture2DArray u_DirShadowMapTexture;
layout(set = 1, binding = 2) uniform textureCube u_PointShadowMapTexture;
layout(set = 1, binding = 4) uniform texture2DArray ddgi_Irrandiance;
layout(set = 1, binding = 5) uniform texture2DArray ddgi_Distance;
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
/**
 * gl_LaunchIDEXT: x: RayIndex y:probeCountPreLayer z:LayerCount
 */

void main() 
{
	DDGISetting volume = GetDDGISetting();
    uint rayIndex = gl_LaunchIDEXT.x;
    uint probePlaneIndex = gl_LaunchIDEXT.y;
    uint planeIndex = gl_LaunchIDEXT.z;
	uint probeCountPrePlane = DDGIGetProbesPerPlane(volume.probeCount);
	uint probeIndex = (planeIndex * probeCountPrePlane) + probePlaneIndex; 
	uvec3 probeCoords = DDGIGetProbeCoords(probeIndex,volume);
	vec3 probeWorldPosition = DDGIGetProbeWorldPosition(probeCoords, volume);
	vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe));

	// 获取这条光线最终在纹理中的存储位置 x: RayIndex y: probeIndexInLayer z:layerIndex
	uvec3 outputCoords = DDGIGetRayDataTexelCoords(rayIndex,probeIndex,volume);
	// uvec3 outputCoords = uvec3(rayIndex,probePlaneIndex,planeIndex);
	// 启动射线
	traceRayEXT(TLAS, 					// acceleration structure
		gl_RayFlagsOpaqueEXT,       	// rayFlags 控制光线的行为，比如是否忽略背面、是否启用 any-hit、是否可用 conservative tracing 等
		0xFF,           				// cullMask 0xFF → 匹配所有实例
		0,              				// sbtRecordOffset 索引到 Shader Binding Table (SBT) 的起始记录位置
		1,              				// sbtRecordStride SBT 中每条记录的间隔（单位是记录数，不是字节）
		0,              				// missIndex 当光线没有击中任何几何体时，使用 SBT 中 miss shader 的索引
		probeWorldPosition.xyz,     	// ray origin
		MIN_RAY_TRACING_DISTANCE,       						// ray min range
		rayDirection.xyz,  				// ray direction
		1e27f,       // TODO:这个应该换成DDGI自己的参数设置
		0               				// payload (location = 0) payload的位置
  	);

	// Miss
	if(payload.hitT == -1.f){
		if(volume.getSkyLight == 1){
			imageStore(out_RAYDATA, ivec3(outputCoords), vec4(payload.albedo, 1e27f));
		}else{
			imageStore(out_RAYDATA, ivec3(outputCoords), vec4(vec3(0), 1e27f));
		}
		return;
	}

	// 击中背面
	if(payload.hitKind == gl_HitKindBackFacingTriangleEXT){
		imageStore(out_RAYDATA, ivec3(outputCoords), vec4(vec3(0), -payload.hitT * 0.2));
		return;
	}

	// 如果开启probeRelocationEnabled或者probeClassificationEnabled 就只存储payload.hitT 待研究
	// Early out: a "fixed" ray hit a front facing surface. Fixed rays are not blended since their direction
    // is not random and they would bias the irradiance estimate. Don't perform lighting for these rays.
    // if((volume.probeRelocationEnabled || volume.probeClassificationEnabled) && rayIndex < RTXGI_DDGI_NUM_FIXED_RAYS)
    // {
    //     // Store the ray front face hit distance (only)
    //     DDGIStoreProbeRayFrontfaceHit(RayData, outputCoords, volume, payload.hitT);
    //     return;
    // }



/////////////////////////////////////////////
// Directional Light
/////////////////////////////////////////////
	DirectionLight dirLight = GetDirectionLight();
	// 硬件在找到第一个 hit 后立即停止 | 跳过ClosestHitShader，直接返回rayGen | 所有模型都被视为不透明
	const uint rayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT;

	float shadowScale = 1.0;

	// // 发射光线计算遮挡
	traceRayEXT(TLAS,
		rayFlags,
		0xFF, 
		1, 
		1,
		1, 
		payload.worldPosition,
		MIN_RAY_TRACING_DISTANCE,   
		-dirLight.direction,
		1e27f,
		1
  	);
	if(shadowPayload.hitT != -1.f){
		shadowScale = 0.0;
	}

	// 计算击中点的直接光
	// m_Params.Albedo = payload.albedo;
	// m_Params.Metalness = payload.metallic;
    // m_Params.Roughness = payload.roughness;
    // m_Params.Normal = payload.normal;
	// m_Params.View = normalize(probeWorldPosition - payload.worldPosition); 
	// m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	// const vec3 Fdielectric = vec3(0.04);
	// vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);


	vec3 albedo = payload.albedo;
	float roughness = payload.roughness;
	float metallic = payload.metallic;
	vec3 N = payload.normal;
	vec3 V = normalize(probeWorldPosition - payload.worldPosition);

	//直接光应该是只需要计算漫反射分量，不需要镜面反射
	vec3 directionLightContribution = CalculateDirectionalLight(albedo, roughness, metallic, N, V) * RT_DirectionShadow(payload.worldPosition,0.0f);
	vec3 pointLightContribution = vec3(0);
	for(int i = 0; i < GetPointLightCount(); i++){
		pointLightContribution += CalculatePointLightOnlyDiffuse(albedo, roughness, metallic,payload.worldPosition, N, V, i) * PointShadow(u_PointShadowMapTexture,payload.worldPosition,i); 
	}
	vec3 spotLightContribution = vec3(0);
	for(int i = 0; i < GetSpotLightCount(); i++){
		spotLightContribution += CalculateSpotLightOnlyDiffuse(albedo, roughness, metallic,payload.worldPosition, N, V, i) * SpotShadow(u_PointShadowMapTexture,payload.worldPosition,i); 
	}
	vec3 diffuse = directionLightContribution + pointLightContribution + spotLightContribution;
	
/////////////////////////////////////////////
// Indirection Light
/////////////////////////////////////////////
	vec3 irradiance = vec3(0);
	float volumeBlendWeight = DDGIGetVolumeBlendWeight(payload.worldPosition, volume);
	if (volumeBlendWeight > 0 && volume.infineBounds == 1){
        irradiance = DDGIGetIrrandianceByWorldPosition(
            payload.worldPosition,
            payload.normal,
            volume,
            ddgi_Irrandiance,ddgi_Distance);
		irradiance *= volumeBlendWeight;
	}
    vec3 radiance = diffuse + ((payload.albedo / PI) * irradiance);
	// if(volume.visulaize == 1 && probeIndex == 164){
	// 	if(payload.hitT < 1e27f){ 
	// 		uvec3 prebeCoords = DDGIGetProbeCoords(probeIndex,volume);
	// 		vec3 probePosition = DDGIGetProbeWorldPosition(prebeCoords,volume);
	// 		AddGizmoLine(probePosition, probePosition + (rayDirection * payload.hitT), vec4(saturate(radiance),1));
	// 	}
	// }
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
	payload.hitKind = gl_HitKindEXT;
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