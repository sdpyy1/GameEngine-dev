#include "hzpch.h"
#include "DirShadowPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
namespace GameEngine {
	void DirShadowPassProcessor::OnCollectBatch(const DrawBatch& batch)
	{
		if (batch.material->CastShadow()) AddBatch(batch);

	}

	RHIGraphicsPipelineRef DirShadowPassProcessor::OnCreatePipeline(const DrawPipelineState& pipelineState)
	{
		return pass->m_Pipeline;
	}
	void DirShadowPass::Init()
	{
		meshPassProcessor = std::make_shared<DirShadowPassProcessor>(this);
		MeshPass::Init();
		m_VertShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "DirShadowMapVert.spv", SHADER_FREQUENCY_VERTEX);
		m_FragShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "DirShadowMapFrag.spv", SHADER_FREQUENCY_FRAGMENT);
		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
			.AddPushConstant({ 4, SHADER_FREQUENCY_VERTEX });
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.vertexShader = m_VertShader->GetRHIShader();
        pipelineInfo.fragmentShader = m_FragShader->GetRHIShader();
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		m_Pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;

	}

	void DirShadowPass::Build(RDGBuilder& builder)
	{ 
		if (APP_SCENEMANAGER->HasDirLight()) {
			for (int i = 0; i < CSM_LEVEL_COUNT; i++) {
				RDGTextureHandle depth = builder.CreateTexture("CSMTexture" + std::to_string(i))
					.AllowDepthStencil()
					.Exetent({ 4096,4096,1 })
					.Format(FORMAT_D32_SFLOAT)
					.Finish();

				builder.CreateRenderPass("DirShadowPass" + std::to_string(i))
					.DepthStencil(depth, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
					.RootSignature(m_RootSignature)
					.PassIndex(i)
					.Execute([&](RDGPassContext context) {
					RHICommandListRef command = context.command;
					command->SetGraphicsPipeline(m_Pipeline);
					command->SetViewport({ 0, 0 }, { 4096,4096 });
					command->SetScissor({ 0, 0 }, { 4096,4096 });
					command->SetDepthBias(0.0f, 0.0f, 0.0f);
					command->BindDescriptorSet(Application::GetRenderSystem()->GetRenderResourceManager()->GetGlobalResourcePerFrameDescriptorSet(), 0);
					uint32_t csmIndex = context.passIndex[0];
					command->PushConstants(&csmIndex, sizeof(uint32_t), SHADER_FREQUENCY_VERTEX);
					meshPassProcessor->Draw(command);
						})
					.Finish();
			}
		}
		
	}




}
