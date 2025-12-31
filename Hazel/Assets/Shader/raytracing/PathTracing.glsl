#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common/common.glsl"
#include "../common/Rand.glsl"
#include "../common/math.glsl"
#include "../common/light.glsl"
#include "../common/brdf.glsl"

struct Payload {
	vec3 albedo;
	vec3 normal;
	vec3 worldPostion;
	float roughness;
	float metallic;
	float hitT;
};
layout(push_constant) uniform setting {
    uint numSamples;
    uint totalNumSamples;
    uint numBounce;
    uint sampleSkyBox;
    uint indirectOnly;
	uint historyActive;
} SETTING;
#ifdef RAYGEN_SHADER
#include "../common/shadow.glsl"
#include "../common/postprocess.glsl"

layout(set = 1, binding = 0, rgba32f) uniform image2D OUT_COLOR;
layout(set = 1, binding = 1, rgba32f) uniform image2D HISTORY_COLOR;
layout(set = 1, binding = 3) readonly buffer EXPOSURE
{
    ExposureSetting setting;     
    float luminance;                     // 计算得到的平均亮度
    float adaptedLuminance;              // 多帧渐进的亮度
    float _padding[2];   
    uint histogramBuffer[256];           // 直方图数组 
    uint readBackHistogramBuffer[256];   // 回读用的数组 
}EXPOSURE_DATA;

layout(location = 0) rayPayloadEXT Payload payload; 

void main() 
{
	//PathTracingSetting SETTING = GetPathTracingSetting();
	ivec2 pixel     = ivec2(gl_LaunchIDEXT.xy);
	// 为每个像素设置一个随机生成器
	Rand rand = SeedRand(GetCamera().totalTick, pixel.y * GetCamera().totalTick + pixel.x);

	vec3 outColor = vec3(0.0f);
	for(int i = 0; i < SETTING.numSamples; i++){
		vec2 jetterPiexl = pixel + vec2(RandFloat(rand), RandFloat(rand)); // 随机数是0-1，刚好不用挪到像素中心了
		ivec2 imageSize = imageSize(OUT_COLOR);
		vec2 jetterUV = jetterPiexl / vec2(imageSize);
		vec2 ndc = jetterUV * 2.0 - 1.0;
		vec3 origin = GetCamera().position;
		vec4 target = inverse(GetCamera().projNoJetter) * vec4(ndc.x, ndc.y, 1, 1);
		vec3 direction = normalize(GetCamera().invView * vec4(normalize(target.xyz), 0)).xyz;
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
				origin,     				// ray origin
				tmin,           				// ray min range
				direction,  				// ray direction
				tmax,           				// ray max range
				0               				// payload (location = 0)
			);

			vec3 albedo = payload.albedo;
			vec3 N = payload.normal;
			vec3 worldPosition = payload.worldPostion;
			float roughness = payload.roughness;
			float metallic = payload.metallic;
			float hitT = payload.hitT;
			vec3 V = normalize(origin - worldPosition);
			
			// MISS
			if(hitT == -1.0f){
				if(SETTING.sampleSkyBox == 1){
					outColor += throughput * albedo;
				}
				break;
			}
			
			/////////////////////////////////////////////// 直接光 ///////////////////////////////////////////////
			if(SETTING.indirectOnly == 0 || b > 0) 			// 本轮的光照
			{
				vec3 directionLightContribution = CalculateDirectionalLight(albedo, roughness, metallic, N, V) * RT_DirectionShadow(worldPosition,0.0f);

				vec3 pointLightContribution = vec3(0);
				for(uint i = 0; i < GetPointLightCount(); i++){
					pointLightContribution += CalculatePointLight(albedo, roughness, metallic,worldPosition, N, V, i) * RT_PointShadow(i,worldPosition); 
				}
				vec3 spotLightContribution = vec3(0);
				for(uint i = 0; i < GetSpotLightCount(); i++){
					spotLightContribution += CalculateSpotLight(albedo, roughness, metallic,worldPosition, N, V, i) * RT_SpotShadow(i,worldPosition); 
				}
				outColor += (directionLightContribution + pointLightContribution + spotLightContribution) * throughput;
			}		
			

			/////////////////////////////////////////////// 开赌 ///////////////////////////////////////////////
			if (b >= 3){	
				float p = max(throughput.x, max(throughput.y, throughput.z));
				if (RandFloat(rand) > p) break;
				throughput *= 1 / p;
			}
			/////////////////////////////////////////////// 更新光线 ///////////////////////////////////////////////
			origin = worldPosition;
			// Select random directions on the hemisphere with a cos(theta) distribution and then compute throughput
        	direction = GetRandomCosineDirectionOnHemisphere(N, rand);   // 在镜面场景下，余弦重要性采样收敛慢

			/////////////////////////////////////////////// 更新throughput ///////////////////////////////////////////////
			vec3 f_r = ResolveBRDF(albedo, roughness, metallic, N, V, direction);
			float NoL = saturate(dot(N, direction));
			float pdf = NoL / PI;

			throughput *= f_r * NoL / max(pdf, 1e-4);
		}
	}
	if(any(isnan(outColor))) outColor = vec3(0.0f);

	bool accumulateHistory 	= SETTING.numSamples < SETTING.totalNumSamples && SETTING.historyActive == 1;
	vec3 historyColor 		= accumulateHistory ? imageLoad(HISTORY_COLOR, pixel).xyz : vec3(0.0f);
	vec3 accumulatedColor 	= (historyColor + outColor);
	if(SETTING.historyActive == 1){
		outColor = accumulatedColor / SETTING.totalNumSamples;
	}

	ColorSetting ColorSETTING = GetColorSetting();
	float finalExposure = ColorSETTING.exposure / EXPOSURE_DATA.adaptedLuminance;

	// Tone Mapping
	if(ColorSETTING.toneMappingMode == 0) outColor = CEToneMapping(outColor, finalExposure); 
	else if(ColorSETTING.toneMappingMode == 1) outColor = Uncharted2ToneMapping(outColor, finalExposure); 
	else if(ColorSETTING.toneMappingMode == 2) outColor = ACESToneMapping(outColor, finalExposure); 
	
	//饱和度
	outColor = SaturationColor(outColor, ColorSETTING.saturation);

	// 对比度
	outColor = ContrastColor(outColor, ColorSETTING.contrast);

	// TODO: Grading

	// Gamma矫正
	const float gamma = 2.2;
	outColor = GammaCorrect(outColor, gamma);
	imageStore(OUT_COLOR, pixel, vec4(outColor, 1.0f));
	imageStore(HISTORY_COLOR, pixel, vec4(accumulatedColor, 1.0f));
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


	payload.albedo = albedo.rgb;
	payload.normal = normal;
	payload.worldPostion = worldPos.xyz;
	payload.roughness = roughness;
	payload.metallic = metallic;
	payload.hitT = gl_HitTEXT;
}
#endif

#ifdef RAYMISS_SHADER

layout(location = 0) rayPayloadInEXT Payload payload;
layout(set = 1, binding = 2) uniform textureCube skyCube;
void main()
{
	payload.albedo = texture(samplerCube(skyCube,SAMPLER[0]),gl_WorldRayDirectionEXT).rgb;  // 可以选择是否采样天空颜色
	payload.hitT = -1;
}
#endif