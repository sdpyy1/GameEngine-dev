#ifndef LIGHT_GLSL
#define LIGHT_GLSL
#include "constant.glsl"
#include "PBR.glsl"
/*
	漫反射项：Lambert 模型
	镜面反射项：Cook-Torrance 模型
*/
/////////////////////////////////////////////
// Directional Light
/////////////////////////////////////////////

vec3 CalculateDirLights(vec3 F0)
{
	vec3 result = vec3(0.0);
	DirLightInfo dirLight = FetchDirLightInfo();
	if(dirLight.radiance == vec3(0.0)){
		return result;
	}		
	vec3 Li = normalize(-dirLight.direction);  // 指向光源
	vec3 Lh = normalize(Li + m_Params.View); // 半程向量
	vec3 Lradiance = dirLight.radiance;

	// Calculate angles between surface normal and various light vectors.
	float cosLi = max(0.0, dot(m_Params.Normal, Li));
	float cosLh = max(0.0, dot(m_Params.Normal, Lh));

	vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_Params.View)), m_Params.Roughness);
	float D = NdfGGX(cosLh, m_Params.Roughness);
	float G = GaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);   
	vec3 diffuseBRDF = kd * m_Params.Albedo; // TODO: /PI

	// Cook-Torrance
	vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);
	result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	return result;
}


/////////////////////////////////////////////
// Point Light
/////////////////////////////////////////////

vec3 CalculatePointLights(in vec3 F0, vec3 worldPos)
{
	vec3 result = vec3(0.0);

	for (uint i = 0; i < FetchPointLightCount(); i++)
	{
		PointLightInfo light = FetchPointLightInfo(i);
		vec3 Li = normalize(light.position - worldPos);
		float lightDistance = length(light.position - worldPos);
		vec3 Lh = normalize(Li + m_Params.View);

		float attenuation = clamp(1.0 - (lightDistance * lightDistance) / (light.sphere.radius * light.sphere.radius), 0.0, 1.0);
		float falloff = 1.0;   // TODO：参数
		attenuation *= mix(attenuation, 1.0, falloff);

		vec3 Lradiance = light.radiance  * attenuation;

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_Params.View)), m_Params.Roughness);
		float D = NdfGGX(cosLh, m_Params.Roughness);
		float G = GaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		float pointScale = 1.0;
		// TODO: 阴影计算简单写在这里,只实现了支持一个阴影
		if(i==0){
			vec3 lightToFrag = worldPos.xyz - light.position; 
			vec3 sampleDir = normalize(lightToFrag);   
			float actualDepth = length(lightToFrag) / light.sphere.radius;
			float storedDepth = texture(samplerCube(u_PointShadowMapTexture, SAMPLER[0]), sampleDir).r;
			float bias = 0.005; 
			bool inShadow = actualDepth > storedDepth + bias;

			// 5. 阴影系数：在阴影中则为 0，否则为 1
			pointScale = inShadow ? 0.0f : 1.0f;
		}



		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi * pointScale;



	}
	return result;
}
/////////////////////////////////////////////
// Spot Light
/////////////////////////////////////////////

vec3 CalculateSpotLights(in vec3 F0, vec3 worldPos)
{
	vec3 result = vec3(0.0);
	for (uint i = 0; i < FetchSpotLightCount(); i++)
	{
		float angle = 60; // TODO:参数
		float falloff = 1.0; // TODO:参数
		vec3 Direction = vec3(1,0,0); // 参数
		float AngleAttenuation = 1.0; // TODO:参数

		
		SpotLightInfo light = FetchSpotLightInfo(i);
		vec3 Li = normalize(light.position - worldPos);
		float lightDistance = length(light.position - worldPos);
		float cutoff = cos(radians(angle * 0.5f));

		float scos = max(dot(Li, Direction), cutoff);
		float rim = (1.0 - scos) / (1.0 - cutoff);

		float attenuation = clamp(1.0 - (lightDistance * lightDistance) / (light.range * light.range), 0.0, 1.0);

		attenuation *= mix(attenuation, 1.0, falloff);

		attenuation *= 1.0 - pow(max(rim, 0.001), AngleAttenuation);

		vec3 Lradiance = light.radiance  * attenuation;
		vec3 Lh = normalize(Li + m_Params.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_Params.View)), m_Params.Roughness);
		float D = NdfGGX(cosLh, m_Params.Roughness);
		float G = GaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;

	}
	return result;
}





/////////////////////////////////////////////
// IBL Light
/////////////////////////////////////////////

vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(samplerCube(u_EnvIrradianceTex,SAMPLER[0]), m_Params.Normal).rgb;
	vec3 F = FresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;

	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
	vec3 specularIrradiance = textureLod(samplerCube(u_EnvRadianceTex,SAMPLER[0]), Lr, m_Params.Roughness * envRadianceTexLevels).rgb;

	vec2 specularBRDF = texture(sampler2D(u_BRDFLUTTexture,SAMPLER[0]), vec2(m_Params.NdotV, m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

#endif