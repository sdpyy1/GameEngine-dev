#include "hzpch.h"
#include "DDGIPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine
{

	void DDGIPass::Init()
	{
		m_RayGenShader = std::make_shared<Shader>("ddgi/RayRadiance", SHADER_FREQUENCY_RAY_GEN)->GetRHIShader();
		m_MissShader = std::make_shared<Shader>("ddgi/RayRadiance", SHADER_FREQUENCY_RAY_MISS)->GetRHIShader();
		m_ClosestHitShader = std::make_shared<Shader>("ddgi/RayRadiance", SHADER_FREQUENCY_CLOSEST_HIT)->GetRHIShader();

		// SBT
		RHIShaderBindingTableInfo sbtInfo = {};
		sbtInfo.AddRayGenGroup(m_RayGenShader);
		sbtInfo.AddMissGroup(m_MissShader);
		sbtInfo.AddHitGroup(m_ClosestHitShader, nullptr, nullptr);
		RHIShaderBindingTableRef sbt = APP_DYNAMICRHI->CreateShaderBindingTable(sbtInfo);

		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
			.AddEntry({ 1, 0, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
			.AddEntry({ 1, 1, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE_CUBE });
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIRayTracingPipelineInfo pipelineInfo = {};
		pipelineInfo.rootSignature = m_RootSignature;
		pipelineInfo.shaderBindingTable = sbt;
		m_Pipeline = APP_DYNAMICRHI->CreateRayTracingPipeline(pipelineInfo);
	}

	void DDGIPass::Build(RDGBuilder& builder)
	{
		auto& [w, h] = APP_WINDOWSIZE;
		DDGISetting ddgiSetting = RENDER_RESOURCEMANAGER->GetGlobalSettingInfo().ddgiSetting;
		if (ddgiSetting.enable) {
			glm::uvec3 probeCount = ddgiSetting.probeCount;
			uint32_t raysPerProbe = ddgiSetting.raysPerProbe;
			uint32_t probeCountPreLayer = probeCount.x * probeCount.z;
			uint32_t volumeLayerCount = probeCount.y;  // y-up
			RDGTextureHandle rayTexture = builder.CreateTexture("RayRadiance")
				.Exetent({ w, h ,1 })
				.ArrayLayers(volumeLayerCount)
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.AllowRenderTarget()
				.AllowReadWrite()
				.Finish();

			RDGTextureHandle skyBox = builder.GetTexture("CubeMap");

			builder.CreateRayTracingPass(GetName())
				.PassIndex(raysPerProbe, probeCountPreLayer, volumeLayerCount)
				.RootSignature(m_RootSignature)
				.ReadWrite(1, 0, 0, rayTexture, VIEW_TYPE_2D_ARRAY)
				.Read(1, 1, 0, skyBox, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR ,0,1,0,6 })
				.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetRayTracingPipeline(m_Pipeline);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->BindDescriptorSet(context.descriptors[1], 1);
				command->TraceRays(context.passIndex[0], context.passIndex[1], context.passIndex[2]);
					})
				.Finish();
		}


	}
}