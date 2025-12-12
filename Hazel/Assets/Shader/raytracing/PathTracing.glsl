#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common/common.glsl"
#include "../common/Rand.glsl"
struct Payload {
    vec3 color;
	float distance;
    vec3 reflectDir;

	vec3 throughput;
	vec3 lightColor;
	float pdf;
	Rand rand;

};

#ifdef RAYGEN_SHADER
layout(set = 1, binding = 0, rgba32f) uniform image2D OUT_COLOR;
layout(set = 1, binding = 1, rgba32f) uniform image2D HISTORY_COLOR;

layout(push_constant) uniform setting {
    int numSamples;
    int totalNumSamples;
    int numBounce;

	int sampleSkyBox;
    int indirectOnly;
    int mode;
} SETTING;


layout(location = 0) rayPayloadEXT Payload payload; 

void main() 
{
	ivec2 pixel     = ivec2(gl_LaunchIDEXT.xy);
	payload.rand = SeedRand(GetCamera().totalTick, pixel.y * GetCamera().totalTick + pixel.x);

	vec3 outColor = vec3(0.0f);
	for(int i = 0; i < SETTING.numSamples; i++){
		vec2 jettePiexl = pixel + vec2(RandFloat(payload.rand), RandFloat(payload.rand)) - vec2(0.5f);
		vec2 jetteUV         = ScreenPixToUV(jettePiexl,1920,1600); 
		vec2 d = ditterUV * 2.0 - 1.0;
		vec4 origin = GetCamera().position;
		vec4 target = inverse(GetCamera().projNoJetter) * vec4(d.x, d.y, 1, 1) ;
		vec4 direction = normalize(GetCamera().invView * vec4(normalize(target.xyz), 0));
		float tmin = MIN_RAY_TRACING_DISTANCE;
		float tmax = MAX_RAY_TRACING_DISTANCE;

		vec3 throughput = vec3(1.0f);
		for(uint b = 0; b <= SETTING.numBounce; b++){
			traceRayEXT(TLAS, 					// acceleration structure
				gl_RayFlagsOpaqueEXT,       	// rayFlags
				0xFF,           				// cullMask
				0,              				// sbtRecordOffset
				0,              				// sbtRecordStride
				0,              				// missIndex
				origin.xyz,     				// ray origin
				tmin,           				// ray min range
				direction.xyz,  				// ray direction
				tmax,           				// ray max range
				0               				// payload (location = 0)
			);

			vec3 hitThroughput 	= payload.throughput;
			vec3 hitLightColor	= payload.lightColor;
			float hitDistance 	= payload.distance;
			float hitPdf		= payload.pdf;

			if(hitDistance == MAX_RAY_TRACING_DISTANCE)	
			{
				if(SETTING.sampleSkyBox > 0 || b == 0)  outColor += throughput * hitLightColor;	
				break;
			}

			if(SETTING.indirectOnly == 0 || b > 0) 			// 本轮的光照
			{
				outColor += throughput * hitLightColor;
			}			
			if(hitPdf == 0.0f)								// 下一轮反射的采样无效
			{
				break;
			}
			throughput *= hitThroughput / hitPdf;

			origin = origin + hitDistance * direction; // 更新光线方向
			direction = vec4(payload.reflectDir, 0);
			if (b >= 3) 									// Russian Roulette
			{	
				float p = max(throughput.x, max(throughput.y, throughput.z));
				if (RandFloat(payload.rand) > p) break;

				throughput *= 1 / p;
			}
		}
	}
	if(any(isnan(outColor))) outColor = vec3(0.0f);

	bool accumulateHistory 	= SETTING.numSamples < SETTING.totalNumSamples;
	vec3 historyColor 		= accumulateHistory ? imageLoad(HISTORY_COLOR, pixel).xyz : vec3(0.0f);
	vec3 accumulatedColor 	= (historyColor + outColor);
	outColor = accumulatedColor / SETTING.totalNumSamples;

	imageStore(OUT_COLOR, pixel, vec4(outColor, 0.0f));
	imageStore(HISTORY_COLOR, pixel, vec4(accumulatedColor, 0.0f));

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






}
#endif

#ifdef RAYMISS_SHADER

layout(location = 0) rayPayloadInEXT Payload payload;
layout(set = 1, binding = 1) uniform textureCube skyCube;
void main()
{
	vec3 skyColor = texture(samplerCube(skyCube,SAMPLER[0]),gl_WorldRayDirectionEXT).rgb;
    payload.distance = MAX_RAY_TRACING_DISTANCE;
	payload.pdf = 1.0f;
	payload.throughput = vec3(0.0);
}
#endif