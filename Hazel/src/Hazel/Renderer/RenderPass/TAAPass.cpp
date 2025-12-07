#include "hzpch.h"
#include "TAAPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine {
	void TAAPass::Init()
	{
		m_Shader = std::make_shared<Shader>("postprocess/TAA", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntryFromReflect(m_Shader).AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIComputePipelineInfo pipelineInfo = {};
		pipelineInfo.computeShader = m_Shader;
		pipelineInfo.rootSignature = m_RootSignature;
		m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);

		// HistoryImage
		auto& [w, h] = APP_WINDOWSIZE;
		TextureSpec spec = {};
		spec.srgb = false;
		spec.generateMipmap = false;
		spec.bindless = false;
		spec.format = FORMAT_R32G32B32A32_SFLOAT;
		spec.extent = { w, h, 1 };
		historyTexture = std::make_shared<Texture>(spec)->GetRHITexture();
	}

	void TAAPass::Build(RDGBuilder& builder)
	{
		auto& [w, h] = APP_WINDOWSIZE;
		RDGTextureHandle Viewport = builder.GetTexture("ViewPort");
		RDGTextureHandle velocity = builder.GetTexture("GBufferVelocity");
		RDGTextureHandle depth = builder.GetTexture("Depth");
		RDGTextureHandle history = isFirstTick ? builder.CreateTexture("TAA History").Import(historyTexture, RESOURCE_STATE_UNDEFINED).Finish()
			: builder.CreateTexture("TAA History").Import(historyTexture, RESOURCE_STATE_TRANSFER_DST).Finish();
		if (isFirstTick) {
            isFirstTick = false;
		}
		RDGTextureHandle TaaRes = builder.CreateTexture("TAA Res")
			.Exetent({ w,h,1 })
			.AllowReadWrite()
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.Finish();

		builder.CreateComputePass("TAA")
			.RootSignature(m_RootSignature)
			.ReadWrite(1, 0, 0, TaaRes)
			.Read(1, 1, 0, velocity)
			.Read(1, 2, 0, history)
			.Read(1, 3, 0, Viewport)
			.Read(1, 4, 0, depth)
			.Execute([&](RDGPassContext context) {
			auto cmd = context.command;
			cmd->SetComputePipeline(m_Pipeline);
			cmd->BindDescriptorSet(context.descriptors[1], 1);
			cmd->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
			auto& [w, h] = APP_WINDOWSIZE;
			uint32_t groupSizeX = 16;
			uint32_t groupSizeY = 16;
			uint32_t numGroupsX = (w + groupSizeX - 1) / groupSizeX;
			uint32_t numGroupsY = (h + groupSizeY - 1) / groupSizeY;
			cmd->Dispatch(numGroupsX, numGroupsY, 1);
				})
			.Finish();

		// 采样结果cpoy到历史纹理
		builder.CreateCopyPass("TAA_HistoryCopy")
			.From(TaaRes)
			.To(history)
			.Finish();

		// 渲染结果写到Viewport
		builder.CreateCopyPass("TAA_Copy")
			.From(TaaRes)
			.To(Viewport)
			.Finish();
	}
}