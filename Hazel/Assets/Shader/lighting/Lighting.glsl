#version 450 core
#include "../common/common.glsl"
#include "../common/Gbuffer.glsl"
#ifdef VERTEX_SHADER
vec3 kNdcPoints[3] = vec3[]( 
    vec3(-1.0, -1.0, 0.0), 
    vec3( 3.0, -1.0, 0.0), 
    vec3(-1.0,  3.0, 0.0) 
);
layout(location = 0) out vec2 TexCoord;
void main()
{
    gl_Position = vec4(kNdcPoints[gl_VertexIndex].xyz, 1.0);
    TexCoord = (kNdcPoints[gl_VertexIndex].xy + 1.0) * 0.5;
}
#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 o_Color;
layout(set = 1, binding = 0) uniform texture2DArray u_DirShadowMapTexture;
layout(set = 1, binding = 1) uniform textureCube u_EnvRadianceTex;
layout(set = 1, binding = 2) uniform textureCube u_EnvIrradianceTex;
layout(set = 1, binding = 3) uniform texture2D u_BRDFLUTTexture;
layout(set = 1, binding = 4) uniform textureCube u_PointShadowMapTexture;
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
void main()
{
    vec3 WorldPosition = GetGBufferPosition(TexCoord);
	if (WorldPosition == vec3(0.0)) {
		o_Color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	float shadowScale = 1.0;
	uint cascadeIndex = 0;
	DirectionLight dirLight = GetDirectionLight();
	Camera CAMERAINFO = GetCamera();
	
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
		

		if(GetShadowSetting().ShadowType == 1) shadowScale = HardShadows_DirectionalLight(u_DirShadowMapTexture, cascadeIndex, shadowMapCoords);
		else if(GetShadowSetting().ShadowType == 2) shadowScale = PCF_DirectionalLight(u_DirShadowMapTexture, cascadeIndex, shadowMapCoords,0.5);
		else if(GetShadowSetting().ShadowType == 3) shadowScale = PCSS_DirectionalLight(u_DirShadowMapTexture, cascadeIndex, shadowMapCoords, 0.5);
	}

	m_Params.Albedo = GetGBufferAlbedo(TexCoord);
	m_Params.Metalness = GetGBufferMetalness(TexCoord) ;
    m_Params.Roughness = GetGBufferRoughness(TexCoord);
    m_Params.Normal = GetGBufferNormal(TexCoord);
	m_Params.View = normalize(CAMERAINFO.position - WorldPosition); 
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	const vec3 Fdielectric = vec3(0.04);
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);
	vec3 lightContribution = CalculateDirLights(F0) * shadowScale + CalculatePointLights(F0, WorldPosition) + CalculateSpotLights(F0, WorldPosition);
	
	// IBL
	vec3 iblContribution = IBL(F0, Lr) * GetSkySetting().IbLScale;  

	vec3 finalColor = lightContribution + iblContribution;

	o_Color = vec4(finalColor,1);
	// Debug
	if(GetShadowSetting().DebugCSM == 1)
	{
		vec3 cascadeColor;
			switch(cascadeIndex) {
			case 0: cascadeColor = vec3(1.0, 0.0, 0.0); break; // ��ɫ - ����0
			case 1: cascadeColor = vec3(0.0, 1.0, 0.0); break; // ��ɫ - ����1
			case 2: cascadeColor = vec3(0.0, 0.0, 1.0); break; // ��ɫ - ����2
			case 3: cascadeColor = vec3(1.0, 1.0, 0.0); break; // ��ɫ - ����3
			default: cascadeColor = vec3(1.0, 0.0, 1.0); // ��ɫ - �쳣
		}
		o_Color = vec4(mix(o_Color.xyz,cascadeColor,0.5), 1.0);
	}
}
#endif
