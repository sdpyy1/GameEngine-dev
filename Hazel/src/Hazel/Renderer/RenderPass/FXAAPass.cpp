#include "hzpch.h"
#include "FXAAPass.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
#include <Hazel/Renderer/RenderSystem/RenderManager.h>
#include <Hazel/Renderer/RenderResource/RenderResourceManager.h>

void GameEngine::FXAAPass::Init()
{
	m_Shader = std::make_shared<Shader>("postprocess/FXAA", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
	RHIRootSignatureInfo info = {};
	info.AddEntryFromReflect(m_Shader).AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
	m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
	RHIComputePipelineInfo pipelineInfo = {};
	pipelineInfo.computeShader = m_Shader;
	pipelineInfo.rootSignature = m_RootSignature;
	m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
}

void GameEngine::FXAAPass::Build(RDGBuilder& builder)
{
	auto& [w, h] = APP_WINDOWSIZE;
	RDGTextureHandle Viewport = builder.GetTexture("ViewPort");
	RDGTextureHandle FXAARes = builder.CreateTexture("FXAA Res")
		.Exetent({ w,h,1 })
		.AllowReadWrite()
		.Format(FORMAT_R32G32B32A32_SFLOAT)
		.Finish();



	builder.CreateComputePass(GetName())
		.RootSignature(m_RootSignature)
		.ReadWrite(1, 0, 0, FXAARes)
		.Read(1, 1, 0, Viewport)
		.Execute([&](RDGPassContext context) {
		auto cmd = context.command;
		cmd->SetComputePipeline(m_Pipeline);
		cmd->BindDescriptorSet(context.descriptors[1], 1);
		cmd->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
		auto& [w, h] = APP_WINDOWSIZE;
		uint32_t groupSizeX = 16;
		uint32_t groupSizeY = 16;
		// 这样计算而不是用ceil的目的是防止ceil前提是你用浮点数，如果整数计算，用下面这个方式更好
		uint32_t numGroupsX = (w + groupSizeX - 1) / groupSizeX;
		uint32_t numGroupsY = (h + groupSizeY - 1) / groupSizeY;
		cmd->Dispatch(numGroupsX, numGroupsY, 1);
			}
		)
		.Finish();


	// 渲染结果写到Viewport
	builder.CreateCopyPass("FXAA_Copy")
		.From(FXAARes)
		.To(Viewport)
		.Finish();
}
