#version 450 core
#include "common/common.glsl"
#include "common/Gbuffer.glsl"
#include "include/Common.glslh"
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
struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
} m_Params;
#include "common/PBR.glsl"  // 必须放在这里，需要上边的这些参数



void main()
{
    vec3 WorldPosition = FetchGBufferPosition(TexCoord);
	if (WorldPosition == vec3(0.0)) { // 这部分无模型，后续天空盒渲染
		o_Color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	// 阴影
	float shadowScale = 1.0;
	uint cascadeIndex = 0;
	DirLightInfo dirLight = FetchDirLightInfo();
	Camera u_CameraData = FetchCamera();
	if(dirLight.radiance != vec3(0.0)){
		vec3 CameraPosition = FetchCamera().CameraPosition;
		float dis = length(WorldPosition - CameraPosition);
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

	// 直接光照
	m_Params.Albedo = FetchGBufferAlbedo(TexCoord);
	m_Params.Metalness = FetchGBufferMetalness(TexCoord);
    m_Params.Roughness = FetchGBufferRoughness(TexCoord);
    m_Params.Normal = FetchGBufferNormal(TexCoord);
	m_Params.View = normalize(u_CameraData.CameraPosition - WorldPosition); 
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	const vec3 Fdielectric = vec3(0.04);
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);
	vec3 lightContribution = CalculateDirLights(F0) * shadowScale;
	
	// IBL
	vec3 iblContribution = IBL(F0, Lr);   // TODO environment Intensity Setting

	vec3 finalColor = lightContribution + iblContribution;

	o_Color = vec4(finalColor,1);
	// Debug
	if(GetShadowSetting().DebugCSM == 1)
	{
		vec3 cascadeColor;
			switch(cascadeIndex) {
			case 0: cascadeColor = vec3(1.0, 0.0, 0.0); break; // 红色 - 级联0
			case 1: cascadeColor = vec3(0.0, 1.0, 0.0); break; // 绿色 - 级联1
			case 2: cascadeColor = vec3(0.0, 0.0, 1.0); break; // 蓝色 - 级联2
			case 3: cascadeColor = vec3(1.0, 1.0, 0.0); break; // 黄色 - 级联3
			default: cascadeColor = vec3(1.0, 0.0, 1.0); // 紫色 - 异常
		}
		o_Color = vec4(mix(o_Color.xyz,cascadeColor,0.5), 1.0);
	}
}
#endif
