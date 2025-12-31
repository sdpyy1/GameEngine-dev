#ifndef SHADOW_GLSL
#define SHADOW_GLSL

/////////////////////////////////////////////
// Directional Light Common
/////////////////////////////////////////////
float GetDirShadowBias(vec3 N)
{
	const float MINIMUM_SHADOW_BIAS = 0.0001;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(N, GetDirectionLight().direction)), MINIMUM_SHADOW_BIAS);
	return bias;
}
// Penumbra
// this search area estimation comes from the following article: 
// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float uvLightSize, float receiverDistance)
{
	const float NEAR = 0.1;
	return uvLightSize * (receiverDistance - NEAR) / GetCamera().position.z;
}

float SearchRegionRadiusUV(float uvLightSize,float zWorld)
{
	const float light_zNear = 0.1;
	return uvLightSize * (zWorld - light_zNear) / zWorld;
}

vec2 PoissonDistribution[64] ={
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790),
	vec2(-0.413923, -0.439757),
	vec2(-0.979153, -0.201245),
	vec2(-0.865579, -0.288695),
	vec2(-0.243704, -0.186378),
	vec2(-0.294920, -0.055748),
	vec2(-0.604452, -0.544251),
	vec2(-0.418056, -0.587679),
	vec2(-0.549156, -0.415877),
	vec2(-0.238080, -0.611761),
	vec2(-0.267004, -0.459702),
	vec2(-0.100006, -0.229116),
	vec2(-0.101928, -0.380382),
	vec2(-0.681467, -0.700773),
	vec2(-0.763488, -0.543386),
	vec2(-0.549030, -0.750749),
	vec2(-0.809045, -0.408738),
	vec2(-0.388134, -0.773448),
	vec2(-0.429392, -0.894892),
	vec2(-0.131597, 0.065058),
	vec2(-0.275002, 0.102922),
	vec2(-0.106117, -0.068327),
	vec2(-0.294586, -0.891515),
	vec2(-0.629418, 0.379387),
	vec2(-0.407257, 0.339748),
	vec2(0.071650, -0.384284),
	vec2(0.022018, -0.263793),
	vec2(0.003879, -0.136073),
	vec2(-0.137533, -0.767844),
	vec2(-0.050874, -0.906068),
	vec2(0.114133, -0.070053),
	vec2(0.163314, -0.217231),
	vec2(-0.100262, -0.587992),
	vec2(-0.004942, 0.125368),
	vec2(0.035302, -0.619310),
	vec2(0.195646, -0.459022),
	vec2(0.303969, -0.346362),
	vec2(-0.678118, 0.685099),
	vec2(-0.628418, 0.507978),
	vec2(-0.508473, 0.458753),
	vec2(0.032134, -0.782030),
	vec2(0.122595, 0.280353),
	vec2(-0.043643, 0.312119),
	vec2(0.132993, 0.085170),
	vec2(-0.192106, 0.285848),
	vec2(0.183621, -0.713242),
	vec2(0.265220, -0.596716),
	vec2(-0.009628, -0.483058),
	vec2(-0.018516, 0.435703),
	vec2(0.089012, 0.546478)
	};

const vec2 poissonDisk[16] = {
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(-0.38277543, 0.27676845),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790)
	};

vec2 SamplePoisson(int index)
{
	return PoissonDistribution[index % 64];
}

/////////////////////////////////////////////
// Directional Shadows
/////////////////////////////////////////////
float DirectionShadow_Hard(texture2DArray shadowMap, uint cascade, vec3 shadowCoords,vec3 N)
{
	float bias = GetDirShadowBias(N);
	float shadowMapDepth = texture(sampler2DArray(shadowMap,SAMPLER[0]), vec3(shadowCoords.xy * 0.5 + 0.5, cascade)).x;
	return step(shadowCoords.z, shadowMapDepth + bias);
}

float DirectionShadow_PCF(texture2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvRadius,vec3 N)
{
	float bias = GetDirShadowBias(N);
	int numPCFSamples = 64;

	float sum = 0;
	for (int i = 0; i < numPCFSamples; i++)
	{
		vec2 offset = SamplePoisson(i) * uvRadius;
		float z = textureLod(sampler2DArray(shadowMap,SAMPLER[0]), vec3((shadowCoords.xy * 0.5 + 0.5) + offset, cascade), 0).r;
		sum += step(shadowCoords.z, z + bias);
	}
	return sum / numPCFSamples;
}	
float FindBlockerDistance_DirectionalLight(texture2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvLightSize,vec3 N)
{
	float bias = GetDirShadowBias(N);

	int numBlockerSearchSamples = 64;
	int blockers = 0;
	float avgBlockerDistance = 0;

	float searchWidth = SearchRegionRadiusUV(uvLightSize,shadowCoords.z);
	for (int i = 0; i < numBlockerSearchSamples; i++)
	{
		float z = textureLod(sampler2DArray(shadowMap,SAMPLER[0]), vec3((shadowCoords.xy * 0.5 + 0.5) + SamplePoisson(i) * searchWidth, cascade), 0).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / float(blockers);

	return -1;
} 

float DirectionShadow_PCSS(texture2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvLightSize,vec3 N)
{
	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowMap, cascade, shadowCoords, uvLightSize,N);
	if (blockerDistance == -1) // No occlusion
		return 1.0f;

	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance * uvLightSize;

	float NEAR = 0.1;
	float uvRadius = penumbraWidth * NEAR / shadowCoords.z;
	uvRadius = min(uvRadius, 0.002f);
	return DirectionShadow_PCF(shadowMap, cascade, shadowCoords, uvRadius,N);
} 

/////////////////////////////////////////////
// Directional Shadows EntryPoint
/////////////////////////////////////////////
vec2 DirectionShadow(texture2DArray shadowMap,vec3 WorldPosition,vec3 N){
	float shadowScale = 1.0;
	uint cascadeIndex = 0;
	DirectionLight dirLight = GetDirectionLight();
	if(dirLight.radiance != vec3(0.0)){
		vec3 position = GetCamera().position;
		float dis = length(WorldPosition - position);
		for (uint i = 0; i < 4; i++)
		{
			if (dis < dirLight.SplitDepth[i])
			{
				cascadeIndex = i;
				break;
			}
		}
		vec4 shadowCoords = dirLight.viewProj[cascadeIndex] * vec4(WorldPosition, 1.0);
		vec3 shadowTex = shadowCoords.xyz / shadowCoords.w;
		vec3 shadowMapCoords = shadowTex;
		

		if(GetShadowSetting().ShadowType == 1) shadowScale = DirectionShadow_Hard(shadowMap, cascadeIndex, shadowMapCoords,N);
		else if(GetShadowSetting().ShadowType == 2) shadowScale = DirectionShadow_PCF(shadowMap, cascadeIndex, shadowMapCoords,10/4096,N);
		else if(GetShadowSetting().ShadowType == 3) shadowScale = DirectionShadow_PCSS(shadowMap, cascadeIndex, shadowMapCoords, 0.1,N);
	}
	return vec2(shadowScale, cascadeIndex);
}
/////////////////////////////////////////////
// Point Shadows
/////////////////////////////////////////////
float VSM(vec4 moments, float depth){
    float mean = moments.x; 
    float moment = moments.y;
	float depth_variance = max(moment - pow(mean, 2), 0.0001);
    
    float depth_diff = depth - mean;

    float shadow_factor = depth_variance / (depth_variance + pow(depth_diff, 2));
    if (depth_diff <= 0.0) {
        shadow_factor = 1.0;
    }

    return shadow_factor;
}
float EVSM(vec4 moments, float depth, float c1, float c2)
{
	float x = exp(c1 * depth);
	float variance_x = max(moments.y - pow(moments.x, 2), 0.0001);
	float d_x = x - moments.x;  

	float p_x = variance_x / (variance_x + pow(d_x, 2)); 

	float y = exp(-c2 * depth);
	float variance_y = max(moments.w - pow(moments.z, 2), 0.0001);
	float d_y = y - moments.z;

	float p_y = variance_y / (variance_y + pow(d_y, 2));

	return min(p_x, p_y);
}
float PointShadow(textureCube shadowMap, vec3 worldPos, uint lightID)
{
	PointLight light = GetPointLight(lightID);
    vec3 lightToFrag = worldPos - light.position;
    vec3 sampleDir = normalize(lightToFrag);
    float actualDepth = length(lightToFrag) / light.sphere.radius;
	uint shadowType = GetShadowSetting().PointShadowType;
	if(shadowType == 0){   // 无阴影
		return 1;
	}else if(shadowType == 1){  // 硬阴影
		float storedDepth = texture(samplerCube(shadowMap, SAMPLER[0]), sampleDir).r;
    	float bias = 0.005;
    	bool inShadow = actualDepth > storedDepth + bias;
    	return inShadow ? 0.0 : 1.0;
	}else if(shadowType == 4){   // VSM
		vec4 moments = texture(samplerCube(shadowMap, SAMPLER[0]), sampleDir);
		float pointShadow = VSM(moments, actualDepth);
		return pointShadow;
	}else if(shadowType == 5){   // EVSM
		vec4 moments = texture(samplerCube(shadowMap, SAMPLER[0]), sampleDir);
		float pointShadow = EVSM(moments, actualDepth,5,0.5);
		pointShadow = smoothstep(0.0f, 1.0f, min(1.0, pointShadow / 0.8));  // 不重映射阴影周围会有一个大白边
		return pointShadow;
	}
	return 1;

}


/////////////////////////////////////////////
// Spot Shadows
/////////////////////////////////////////////
float SpotShadow(textureCube shadowMap, vec3 worldPos, uint lightID){
	return 1.0f; // TODO: 没写
}



//////////////////////////////////////////////
// RayTracing Shadow
//////////////////////////////////////////////
#ifdef RAYGEN_SHADER

bool RayQueryVisibility(vec3 from, vec3 to) 
{
	float tmin = MIN_RAY_TRACING_DISTANCE;
    //float tmax = MAX_RAY_TRACING_DISTANCE;  
	float tmax = length(to - from);
	vec3 dir = normalize(to - from);

    rayQueryEXT query;
    rayQueryInitializeEXT(
        query, 
        TLAS, 
        gl_RayFlagsTerminateOnFirstHitEXT, 
        0xFF, 
        from, 
        tmin, 
        dir, 
        tmax);

    rayQueryProceedEXT(query);

    float dist = tmax;
    if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        return true;
    }
    return false;
}

float RT_DirectionShadow(vec3 worldPos, float bias)
{
    float dirShadow = 1.0f;
	DirectionLight dirLight = GetDirectionLight();
	if(dirLight.radiance == vec3(0)) return dirShadow;
	vec3 L = -normalize(dirLight.direction);
	vec3 origin = worldPos.xyz + bias * L;
	dirShadow = RayQueryVisibility(origin, origin + L * MAX_RAY_TRACING_DISTANCE) ? 0.0f : 1.0f; 
    return dirShadow;
}

float RT_PointShadow(uint lightID, vec3 worldPos)
{
	PointLight pointLight = GetPointLight(lightID);
    float pointShadow = RayQueryVisibility(worldPos.xyz, pointLight.position) ? 0.0f : 1.0f; 
    return pointShadow;
}

float RT_SpotShadow(uint lightID, vec3 worldPos)
{
	SpotLight spotLight = GetSpotLight(lightID);
    float spotShadow = RayQueryVisibility(worldPos.xyz, spotLight.position) ? 0.0f : 1.0f; 
    return spotShadow;
}
#endif // RAYGEN_SHADER










#endif // SHADOW_GLSL