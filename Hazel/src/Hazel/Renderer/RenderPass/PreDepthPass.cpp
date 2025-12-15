#include "hzpch.h"
#include "PreDepthPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>

namespace GameEngine { 

	void PreDepthPassProcessor::AddMeshBatch(const MeshBatch& batch)
	{
		if (batch.material->RenderPassMask() & PASS_MASK_DEFERRED_PASS) AddBatch(batch);
	}

	RHIGraphicsPipelineRef PreDepthPassProcessor::OnCreatePipeline(const DrawPipelineState& pipelineState)
	{
		return pass->m_Pipeline;
	}

	void PreDepthPass::Init()
	{
		meshPassProcessor = std::make_shared<PreDepthPassProcessor>(this);
		MeshPass::Init();
		m_VertShader = std::make_shared<Shader>("mesh/PreDepth", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
		m_FragShader = std::make_shared<Shader>("mesh/PreDepth", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.rootSignature = m_RootSignature;
		pipelineInfo.vertexShader = m_VertShader;
		pipelineInfo.fragmentShader = m_FragShader;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);  // Depth Test
	}

	void PreDepthPass::Build(RDGBuilder& builder)
	{
		auto [w, h] = APP_WINDOWSIZE;
		RDGTextureHandle depth = builder.CreateTexture("Depth")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_D32_SFLOAT)
			.AllowDepthStencil()
			.Finish();

		builder.CreateRenderPass(GetName())
			.DepthStencil(depth, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
			.RootSignature(m_RootSignature)
			.Execute([&](RDGPassContext context) {
					auto [w, h] = APP_WINDOWSIZE;
					RHICommandListRef command = context.command;
					command->SetGraphicsPipeline(m_Pipeline);
					command->SetViewport({ 0, 0 }, { w,h });
					command->SetScissor({ 0, 0 }, { w,h });
					command->SetDepthBias(0.0f, 0.0f, 0.0f);
					command->BindDescriptorSet(Application::GetRenderSystem()->GetRenderResourceManager()->GetGlobalResourcePerFrameDescriptorSet(), 0);
					meshPassProcessor->Draw(command);
				})
			.Finish();
	}

}