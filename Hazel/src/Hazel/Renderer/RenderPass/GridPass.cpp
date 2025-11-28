#include "hzpch.h"
#include "GridPass.h"
#include <Hazel/Core/Application.h>
#include "Hazel/Renderer/RenderResource/Shader.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"

namespace GameEngine {
	void GridPass::Init()
	{
		auto RHI = APP_DYNAMICRHI;
		std::string vertPath =APP_SHADER_PATH + "gridVert.spv";
		std::string fragPath = APP_SHADER_PATH + "gridFrag.spv";
		m_VertShader = V2::Shader(vertPath, SHADER_FREQUENCY_VERTEX).GetRHIShader();
		m_FragShader = V2::Shader(fragPath, SHADER_FREQUENCY_FRAGMENT).GetRHIShader();


		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntryFromReflect(m_VertShader)
			.AddEntryFromReflect(m_FragShader)
			.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo())
			.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = RHI->CreateRootSignature(rootSignatureInfo);
		RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.vertexShader = m_VertShader;
        pipelineInfo.fragmentShader = m_FragShader;		
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}

	void GridPass::Build(RDGBuilder& builder)
	{
		auto [w, h] = APP_WINDOWSIZE;
		RDGTextureHandle viewPort = builder.GetTexture("ViewPort");
		RDGTextureHandle outDepth = builder.GetTexture("Depth");


		RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
			.Read(2,1,0,outDepth)
			.RootSignature(m_RootSignature)
			.Color(0, viewPort, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE)
			.Execute([&](RDGPassContext context) {
				auto [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;

				command->SetGraphicsPipeline(m_Pipeline);
				command->SetViewport({ 0, 0 }, { w, h });
				command->SetScissor({ 0, 0 }, { w, h });
				command->SetDepthBias(0.0f, 0.0f, 0.0f);
				command->BindDescriptorSet(context.descriptors[2], 2);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Draw(6,1,0,0);
			})
			.Finish();
	}

}