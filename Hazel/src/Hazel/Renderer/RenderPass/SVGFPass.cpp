#include "hzpch.h"
#include "SVGFPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine {
	void SVGFPass::Init()
	{
		m_Shader = std::make_shared<Shader>("postprocess/SVGF", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntryFromReflect(m_Shader);
		info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
			.AddPushConstant({4,SHADER_FREQUENCY_COMPUTE });
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIComputePipelineInfo pipelineInfo = {};
		pipelineInfo.computeShader = m_Shader;
		pipelineInfo.rootSignature = m_RootSignature;
		m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		auto& [w, h] = APP_WINDOWSIZE;

		{
			RHITextureInfo textureInfo;
			textureInfo.extent = { w, h, 1 };
			textureInfo.format = RHIFormat::FORMAT_R32G32B32A32_SFLOAT;
			textureInfo.mipLevels = 1;
			textureInfo.arrayLayers = 1;
			textureInfo.type |= RESOURCE_TYPE_RW_TEXTURE;
			m_directFilterHistory = APP_DYNAMICRHI->CreateTexture(textureInfo);
			m_indirectFilterHistory = APP_DYNAMICRHI->CreateTexture(textureInfo);
		}
	}

	void SVGFPass::Build(RDGBuilder& builder)
	{
		auto& [w, h] = APP_WINDOWSIZE;
		RDGTextureHandle pathTracingDirectRes = builder.GetTexture("PathTracingdirectRes");
		if (pathTracingDirectRes.ID() == UINT32_MAX) { return; }
		RDGTextureHandle pathTracingIndirectRes = builder.GetTexture("PathTracingIndirectRes");
		RDGTextureHandle velocity = builder.GetTexture("GBufferVelocity");
		RDGBufferHandle exposureData = builder.GetBuffer("ExposureData");

		RDGTextureHandle directFilterRes = builder.CreateTexture("PathTracing_SVGF_directFilterRes")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();
		RDGTextureHandle directFilterResHistory = builder.CreateTexture("PathTracing_SVGF_directFilterResHistory")
			.Import(m_directFilterHistory, RESOURCE_STATE_UNDEFINED).Finish();

		RDGTextureHandle forPinpongTexture = builder.CreateTexture("PathTracing_SVGF_forPinpongTexture")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();


		RDGTextureHandle inDirectFilterRes = builder.CreateTexture("PathTracing_SVGF_inDirectFilterRes")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		RDGTextureHandle inDirectFilterResHistory = builder.CreateTexture("PathTracing_SVGF_inDirectFilterResHistory")
			.Import(m_indirectFilterHistory, RESOURCE_STATE_UNDEFINED).Finish();

		RDGTextureHandle position = builder.GetTexture("GBufferPosition");
		RDGTextureHandle normal = builder.GetTexture("GBufferNormal");
		RDGTextureHandle material = builder.GetTexture("GBufferMaterial");
		RDGTextureHandle albedo = builder.GetTexture("GBufferAlbedo");
		// 直接光
		for (int i = 0; i < 5; i++) {
			builder.CreateComputePass(GetName() + "_DirectRes" + std::to_string(i))
				.RootSignature(m_RootSignature)
				.PassIndex(i)
				.ReadWrite(1, 0, 0, i == 0 ? directFilterRes : i % 2 == 1 ? forPinpongTexture : directFilterRes)
				.ReadWrite(1, 1, 0, i == 0 ? pathTracingDirectRes : i % 2 == 1 ? directFilterRes : forPinpongTexture)
				.ReadWrite(1, 2, 0, velocity)
				.ReadWrite(1, 3, 0, directFilterResHistory)
				.ReadWrite(1, 4, 0, position)
				.ReadWrite(1, 5, 0, normal)
				.ReadWrite(1, 6, 0, material)
				.ReadWrite(1, 7, 0, albedo)
				.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_Pipeline);
				command->BindDescriptorSet(context.descriptors[1], 1);
				int curIndex = context.passIndex[0];
				command->PushConstants(&curIndex, sizeof(uint32_t), SHADER_FREQUENCY_COMPUTE);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Dispatch((w + 15) / 16, (h + 15) / 16, 1);
					})
				.Finish();
		}



		// 间接光
		for (int i = 0; i < 5; i++) {
			builder.CreateComputePass(GetName() + "_InDirectRes" + std::to_string(i))
				.RootSignature(m_RootSignature)
				.PassIndex(i)
				.ReadWrite(1, 0, 0, i == 0 ? inDirectFilterRes : i % 2 == 1 ? forPinpongTexture : inDirectFilterRes)
				.ReadWrite(1, 1, 0, i == 0 ? pathTracingIndirectRes : i % 2 == 1 ? inDirectFilterRes : forPinpongTexture)
				.ReadWrite(1, 2, 0, velocity)
				.ReadWrite(1, 3, 0, inDirectFilterResHistory)
				.ReadWrite(1, 4, 0, position)
				.ReadWrite(1, 5, 0, normal)
				.ReadWrite(1, 6, 0, material)
				.ReadWrite(1, 7, 0, albedo)
				.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_Pipeline);
				command->BindDescriptorSet(context.descriptors[1], 1);
				uint32_t curIndex = context.passIndex[0];
				command->PushConstants(&curIndex, sizeof(uint32_t), SHADER_FREQUENCY_COMPUTE);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Dispatch((w + 15) / 16, (h + 15) / 16, 1);
					})
				.Finish();
		}


	}

}