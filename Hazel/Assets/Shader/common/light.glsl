#ifndef LIGHT_GLSL
#define LIGHT_GLSL
#include "constant.glsl"
#include "PBR.glsl"
#include "BRDF.glsl"
/*
	漫反射项：Lambert 模型
	镜面反射项：Cook-Torrance 模型
*/
/////////////////////////////////////////////
// Directional Light
/////////////////////////////////////////////
vec3 CalculateDirectionalLight(vec3 albedo, float roughness, float metallic , vec3 N, vec3 V){
	DirectionLight dirLight = GetDirectionLight();
	if(dirLight.radiance == vec3(0.0)){
		return vec3(0.0f);
	}		
	vec3 L = -normalize(dirLight.direction);    
	float NoL = saturate(dot(N, L));
    vec3 radiance = dirLight.radiance * dirLight.intensity;   

    vec3 f_r = ResolveBRDF(albedo, roughness, metallic, N, V, L);

    return max(vec3(0.0f), f_r * radiance * NoL);	
}
vec3 CalculateDirectionalLightOnlyDiffuse(vec3 albedo, float roughness, float metallic , vec3 N, vec3 V){
	DirectionLight dirLight = GetDirectionLight();
	if(dirLight.radiance == vec3(0.0)){
		return vec3(0.0f);
	}		
	vec3 L = -normalize(dirLight.direction);    
	float NoL = saturate(dot(N, L));
    vec3 radiance = dirLight.radiance * dirLight.intensity;   

    vec3 f_r = ResolveDiffuseBRDF(albedo, roughness, metallic, N, V, L);

    return max(vec3(0.0f), f_r * radiance * NoL);	
}

/////////////////////////////////////////////
// Point Light
/////////////////////////////////////////////
float PointLightFalloff(float dist, float radius) 
{ 
	float attenuation = clamp(1.0 - (dist * dist) / (radius * radius), 0.0, 1.0);
	float falloff = 1.0;   // TODO：参数
	return attenuation *= mix(attenuation, 1.0, falloff);	
}
vec3 CalculatePointLight(vec3 albedo, float roughness, float metallic,
    vec3 worldPos, vec3 N, vec3 V, uint lightID)
{
	if(lightID >= GetPointLightCount()){
		return vec3(0.0f);
	}
	PointLight light = GetPointLight(lightID);
	float lightDistance = length(light.position - worldPos);
	vec3 L = normalize(light.position - worldPos.xyz);
	float NoL = saturate(dot(N, L));

	float attenuation   = PointLightFalloff(lightDistance, light.sphere.radius);
	vec3 f_r = ResolveBRDF(albedo.xyz, roughness, metallic, N, V, L);
    vec3 radiance = light.radiance * light.intensity * attenuation;   
    return max(vec3(0.0f), f_r * radiance * NoL);	
}
vec3 CalculatePointLightOnlyDiffuse(vec3 albedo, float roughness, float metallic,
    vec3 worldPos, vec3 N, vec3 V, uint lightID)
{
	if(lightID >= GetPointLightCount()){
		return vec3(0.0f);
	}
	PointLight light = GetPointLight(lightID);
	float lightDistance = length(light.position - worldPos);
	vec3 L = normalize(light.position - worldPos.xyz);
	float NoL = saturate(dot(N, L));

	float attenuation   = PointLightFalloff(lightDistance, light.sphere.radius);
	vec3 f_r = ResolveDiffuseBRDF(albedo.xyz, roughness, metallic, N, V, L);
    vec3 radiance = light.radiance * light.intensity * attenuation;   
    return max(vec3(0.0f), f_r * radiance * NoL);	
}
/////////////////////////////////////////////
// Spot Light
/////////////////////////////////////////////


float CalculateSpotLightAttenuation(SpotLight light,vec3 worldPos,vec3 L,float lightDistance )
{
    float attenuation =
        clamp(1.0 - (lightDistance * lightDistance) /
                      (light.range * light.range),
              0.0, 1.0);

    attenuation *= mix(attenuation, 1.0, light.falloff);

    float cutoff = cos(radians(light.angle * 0.5));
    float scos   = max(dot(L, normalize(-light.direction)), cutoff);

    float rim = (1.0 - scos) / (1.0 - cutoff);

    float AngleAttenuation = 1.0; // TODO: 可作为参数
    attenuation *= 1.0 - pow(max(rim, 0.001), AngleAttenuation);

    return attenuation;
}
vec3 CalculateSpotLight(vec3 albedo,float roughness,float metallic,
    vec3 worldPos,vec3 N,vec3 V,uint lightID)
{
    if (lightID >= GetSpotLightCount())
        return vec3(0.0);

    SpotLight light = GetSpotLight(lightID);

    vec3  toLight       = light.position - worldPos;
    float lightDistance = length(toLight);
    vec3  L              = toLight / lightDistance;

    float NoL = saturate(dot(N, L));
    if (NoL <= 0.0)
        return vec3(0.0);

    float attenuation = CalculateSpotLightAttenuation(light, worldPos, L, lightDistance);

    vec3 f_r = ResolveBRDF(albedo, roughness, metallic, N, V, L);
    vec3 radiance = light.radiance * light.intensity * attenuation;

    return max(vec3(0.0), f_r * radiance * NoL);
}
vec3 CalculateSpotLightOnlyDiffuse(vec3 albedo,float roughness,float metallic,
    vec3 worldPos,vec3 N,vec3 V,uint lightID)
{
    if (lightID >= GetSpotLightCount())
        return vec3(0.0);

    SpotLight light = GetSpotLight(lightID);

    vec3  toLight       = light.position - worldPos;
    float lightDistance = length(toLight);
    vec3  L              = toLight / lightDistance;

    float NoL = saturate(dot(N, L));
    if (NoL <= 0.0)
        return vec3(0.0);

    float attenuation =CalculateSpotLightAttenuation(light, worldPos, L, lightDistance);

    vec3 f_r = ResolveDiffuseBRDF(albedo, roughness, metallic, N, V, L);
    vec3 radiance = light.radiance * light.intensity * attenuation;

    return max(vec3(0.0), f_r * radiance * NoL);
}

/////////////////////////////////////////////
// IBL
/////////////////////////////////////////////
vec3 CalculateIBLLight(
    vec3 N,
    vec3 V,
    vec3 albedo,
    float metallic,
    float roughness,
    textureCube tex_env_irradiance,
    textureCube tex_env_radiance,
    texture2D tex_brdf_lut
) {

	vec3 lr = reflect(-V, N);


    BxDFContext context;
    Init(context, N, V, vec3(0));

    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    vec3 irradiance = texture(samplerCube(tex_env_irradiance, SAMPLER[0]), N).rgb;
    vec3 fresnel = F_Schlick(f0, context.NoV, roughness);
    vec3 kd = (1.0 - fresnel) * (1.0 - metallic);
    vec3 diffuse_ibl = albedo * irradiance;

    int env_radiance_tex_levels = textureQueryLevels(tex_env_radiance);
    vec3 specular_irradiance = textureLod(
        samplerCube(tex_env_radiance, SAMPLER[0]),
        lr,
        roughness * float(env_radiance_tex_levels)
    ).rgb;

    vec2 specular_brdf = texture(sampler2D(tex_brdf_lut, SAMPLER[0]), vec2(context.NoV, roughness)).rg;
    vec3 specular_ibl = specular_irradiance * (f0 * specular_brdf.x + specular_brdf.y);

    return kd * diffuse_ibl + specular_ibl;
}



#endif