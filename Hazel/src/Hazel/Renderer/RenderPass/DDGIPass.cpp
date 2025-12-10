#include "hzpch.h"
#include "DDGIPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
#include "Hazel/Renderer/RenderSystem/LightCollector.h"

namespace GameEngine
{
	void DDGIPass::Init()
	{
		{
			m_RayGenShader = std::make_shared<Shader>("ddgi/RayRadiance", SHADER_FREQUENCY_RAY_GEN)->GetRHIShader();
			m_MissShader = std::make_shared<Shader>("ddgi/RayRadiance", SHADER_FREQUENCY_RAY_MISS)->GetRHIShader();
			m_ClosestHitShader = std::make_shared<Shader>("ddgi/RayRadiance", SHADER_FREQUENCY_CLOSEST_HIT)->GetRHIShader();

			// SBT
			RHIShaderBindingTableInfo sbtInfo = {};
			sbtInfo.AddRayGenGroup(m_RayGenShader);
			sbtInfo.AddMissGroup(m_MissShader);
			sbtInfo.AddHitGroup(m_ClosestHitShader);
			RHIShaderBindingTableRef sbt = APP_DYNAMICRHI->CreateShaderBindingTable(sbtInfo);

			RHIRootSignatureInfo rootSignatureInfo = {};
			rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddEntry({ 1, 0, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
				.AddEntry({ 1, 1, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE })
				.AddEntry({ 1, 2, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE_CUBE })
				.AddEntry({ 1, 3, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE_CUBE })
				.AddEntry({ 1, 4, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE })
				.AddEntry({ 1, 5, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE });
			m_VolumeTraceRootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

			RHIRayTracingPipelineInfo pipelineInfo = {};
			pipelineInfo.rootSignature = m_VolumeTraceRootSignature;
			pipelineInfo.shaderBindingTable = sbt;
			m_VolumeTracePipeline = APP_DYNAMICRHI->CreateRayTracingPipeline(pipelineInfo);
		}

		{
			m_ProbeIrrandianceBlendShader = std::make_shared<Shader>("ddgi/IrrandianceBlend", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
            RHIRootSignatureInfo rootSignatureInfo;
			rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddEntry({ 1, 0, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE })
				.AddEntry({ 1, 1, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_TEXTURE });
			m_ProbeIrrandianceBlendRootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);
			RHIComputePipelineInfo pipelineInfo;
            pipelineInfo.computeShader = m_ProbeIrrandianceBlendShader;
            pipelineInfo.rootSignature = m_ProbeIrrandianceBlendRootSignature;
            m_ProbeIrrandianceBlendPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}
		{
			m_ProbeDistanceBlendShader = std::make_shared<Shader>("ddgi/DistanceBlend", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo rootSignatureInfo;
			rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddEntry({ 1, 0, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE })
				.AddEntry({ 1, 1, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_TEXTURE });
			m_ProbeDistanceBlendRootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);
			RHIComputePipelineInfo pipelineInfo;
			pipelineInfo.computeShader = m_ProbeDistanceBlendShader;
			pipelineInfo.rootSignature = m_ProbeDistanceBlendRootSignature;
			m_ProbeDistanceBlendPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}
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
			RDGTextureHandle rayTexture = builder.CreateTexture("DDGI_RayRadiance")
				.Exetent({ raysPerProbe, probeCountPreLayer ,1 })
				.ArrayLayers(volumeLayerCount)
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.AllowRenderTarget()
				.AllowReadWrite()
				.Finish();

			RDGTextureHandle irrandiance = builder.CreateTexture("DDGI_Irrandiance")
				.Exetent({ probeCount.x * 8,probeCount.z * 8,1 })
				.ArrayLayers(volumeLayerCount)
				.AllowReadWrite()
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();

			RDGTextureHandle distance = builder.CreateTexture("DDGI_Distance")
				.Exetent({ probeCount.x * 16,probeCount.z * 16,1 })
				.ArrayLayers(volumeLayerCount)
				.AllowReadWrite()
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();
			RDGTextureHandle dirShadowMap = builder.GetTexture("CSMTextureArray");

			RDGTextureHandle skyBox = builder.GetTexture("CubeMap");

			// VolumeTrace
			{
				auto& build = builder.CreateRayTracingPass(GetName() + "_VolumeTrace")
					.PassIndex(raysPerProbe, probeCountPreLayer, volumeLayerCount)
					.RootSignature(m_VolumeTraceRootSignature)
					.ReadWrite(1, 0, 0, rayTexture, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR ,0,1,0,volumeLayerCount })
					.Read(1, 1, 0, dirShadowMap, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_DEPTH ,0,1,0,4 })
					// 点光源阴影单独处理
					.Read(1, 3, 0, skyBox, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR ,0,1,0,6 })
					.Read(1, 4, 0, irrandiance, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR,0,1,0,volumeLayerCount })
					.Read(1, 5, 0, distance, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR,0,1,0,volumeLayerCount })
					.Execute([&](RDGPassContext context) {
					RHICommandListRef command = context.command;
					command->SetRayTracingPipeline(m_VolumeTracePipeline);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
					command->BindDescriptorSet(context.descriptors[1], 1);
					command->TraceRays(context.passIndex[0], context.passIndex[1], context.passIndex[2]); // 注意这里传递的是射线数量、每层探针数量、探针层数
						});
				LightInfo& lightInfo = LightCollector::GetLightInfo();
				if (lightInfo.pointLightCount > 0) {
					RDGTextureHandle pointShadowMap = builder.GetTexture("Point Shadow Color[0]");
					build.Read(1, 2, 0, pointShadowMap, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR,0,1,0,6 });
				}
			}

			{
				// Irrandiance Blend
				builder.CreateComputePass(GetName() + "_ProbeIrrandianceBlend")
					.ReadWrite(1, 0, 0, irrandiance, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR ,0,1,0,volumeLayerCount })
					.Read(1, 1, 0, rayTexture, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR ,0,1,0,volumeLayerCount })
					.RootSignature(m_ProbeIrrandianceBlendRootSignature)
					.PassIndex(probeCount.x, probeCount.z, probeCount.y)// Y_up
					.Execute([&](RDGPassContext context) {
					RHICommandListRef command = context.command;
					command->SetComputePipeline(m_ProbeIrrandianceBlendPipeline);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
					command->BindDescriptorSet(context.descriptors[1], 1);
					command->Dispatch(context.passIndex[0], context.passIndex[1], context.passIndex[2]);
						})
					.Finish();
			}

			{
				// Distance Blend
				builder.CreateComputePass(GetName() + "_ProbeDistanceBlend")
					.ReadWrite(1, 0, 0, distance, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR ,0,1,0,volumeLayerCount })
					.Read(1, 1, 0, rayTexture, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR ,0,1,0,volumeLayerCount })
					.RootSignature(m_ProbeDistanceBlendRootSignature)
					.PassIndex(probeCount.x, probeCount.z, probeCount.y) // Y_up
					.Execute([&](RDGPassContext context) {
					RHICommandListRef command = context.command;
					command->SetComputePipeline(m_ProbeDistanceBlendPipeline);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
					command->BindDescriptorSet(context.descriptors[1], 1);
					command->Dispatch(context.passIndex[0], context.passIndex[1], context.passIndex[2]);
						})
					.Finish();
			}

		}


	}
}