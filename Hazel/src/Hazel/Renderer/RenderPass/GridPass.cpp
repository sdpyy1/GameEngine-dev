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
		std::string vertPath ="Assets/Shader/spv/gridVert.spv";
		std::string fragPath = "Assets/Shader/spv/gridFrag.spv";
		m_VertShader = V2::Shader(vertPath, SHADER_FREQUENCY_VERTEX).GetRHIShader();
		m_FragShader = V2::Shader(fragPath, SHADER_FREQUENCY_FRAGMENT).GetRHIShader();


		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntryFromReflect(m_VertShader).AddEntryFromReflect(m_FragShader).AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo());
		m_RootSignature = RHI->CreateRootSignature(rootSignatureInfo);
		RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.vertexShader = m_VertShader;
        pipelineInfo.fragmentShader = m_FragShader;
		pipelineInfo.blendState.renderTargets[0] = { BLEND_OP_ADD, BLEND_FACTOR_SRC_ALPHA, BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		                                              BLEND_OP_ADD, BLEND_FACTOR_SRC_ALPHA, BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		                                              COLOR_MASK_RGBA, true};		
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R8G8B8A8_UNORM;
		m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}

	void GridPass::Build(RDGBuilder& builder)
	{
		auto [w, h] = APP_WINDOWSIZE;
		RDGTextureHandle viewPort = builder.GetTexture("ViewPort");
		RDGTextureHandle outDepth = builder.GetTexture("Depth");
		RDGBufferHandle cameraData = builder.CreateBuffer("CameraData")
			.Import(RENDER_RESOURCEMANAGER->GetCameraDataBuffer().GetRHIBuffer(), RESOURCE_STATE_UNORDERED_ACCESS)
			.Finish();


		RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
			.LabelColor({0.2,0.2,0.6})
			.Read(0, 0, 0, cameraData)
			.Read(0,1,0,outDepth)
			.RootSignature(m_RootSignature)
			.Color(0, viewPort, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE)
			.Execute([&](RDGPassContext context) {
				auto [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;

				command->SetGraphicsPipeline(m_Pipeline);
				command->SetViewport({ 0, 0 }, { w, h });
				command->SetScissor({ 0, 0 }, { w, h });
				command->SetDepthBias(0.0f, 0.0f, 0.0f);
				command->BindDescriptorSet(context.descriptors[0], 0);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);

				command->Draw(6,1,0,0);

			})
			.Finish();




	}

}