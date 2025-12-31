#include "hzpch.h"
#include "PointShadowPass.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderSystem/RenderManager.h>
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderSystem/LightCollector.h"
namespace GameEngine {
	
	void PointShadowPassProcessor::AddMeshBatch(const MeshBatch& batch)
	{
		if (batch.material->CastShadow()) AddBatch(batch);
	}

	RHIGraphicsPipelineRef PointShadowPassProcessor::OnCreatePipeline(const DrawPipelineState& pipelineState)
	{
		return pass->m_Pipeline;
	}

	void PointShadowPass::Init()
	{
		for(int i = 0;i< MAX_POINT_SHADOW_COUNT;i++){
			meshPassProcessors.emplace_back(std::make_shared<PointShadowPassProcessor>(this));
		}
		MeshPass::Init();

		m_VertShader = std::make_shared<Shader>("mesh/shadow/PointShadow", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
        m_GeomShader = std::make_shared<Shader>("mesh/shadow/PointShadow", SHADER_FREQUENCY_GEOMETRY)->GetRHIShader();
        m_FragShader = std::make_shared<Shader>("mesh/shadow/PointShadow", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
		
		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntryFromReflect(m_VertShader)
			.AddEntryFromReflect(m_GeomShader)
			.AddEntryFromReflect(m_FragShader)
			.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
			.AddPushConstant({ 4,SHADER_FREQUENCY_GRAPHICS });
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.vertexShader = m_VertShader;
        pipelineInfo.geometryShader = m_GeomShader;
        pipelineInfo.fragmentShader = m_FragShader;
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
        pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_FRONT, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.blendState.renderTargets[0].enable = false;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		m_Pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;


	}

	void PointShadowPass::Build(RDGBuilder& builder)
	{
		LightInfo& lightInfo = LightCollector::GetLightInfo();
		for (int i = 0; i < lightInfo.pointLightCount; i++) {
			if (i >= MAX_POINT_SHADOW_COUNT) {
				return;
			}
			auto& CurPointLight = lightInfo.pointLights[i];
			RDGTextureHandle color = builder.CreateTexture("Point Shadow Color[" + std::to_string(i) + "]")
				.Exetent({ PointShadowResolution, PointShadowResolution, 1 })
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.ArrayLayers(6)  // 存储六个面
				.MipLevels(1)
				.AllowRenderTarget()
				.CubeMap()
				.Finish();


			RDGTextureHandle depth = builder.CreateTexture("pointShadowDepth[" + std::to_string(i) + "]")
				.ArrayLayers(6)
				.AllowDepthStencil()
				.CubeMap()
				.Exetent({ PointShadowResolution,PointShadowResolution,1 })
				.Format(FORMAT_D32_SFLOAT)
				.Finish();

			RDGRenderPassHandle pass = builder.CreateRenderPass(GetName() + std::to_string(i))
				.PassIndex(i)
				.Color(0, color, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE,{ 0.0f, 0.0f, 0.0f, 0.0f }, { TEXTURE_ASPECT_COLOR, 0, 1, 0, 6 })  // 注意layer=6
				.DepthStencil(depth, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, 1.0f, 0, { TEXTURE_ASPECT_DEPTH, 0, 1, 0, 6 }) // 注意layer=6
				.Execute([&](RDGPassContext context) {

				uint32_t pointLightID = context.passIndex[0];

				RHICommandListRef command = context.command;
				command->SetGraphicsPipeline(m_Pipeline);
				command->SetViewport({ 0, 0 }, { PointShadowResolution, PointShadowResolution });
				command->SetScissor({ 0, 0 }, { PointShadowResolution, PointShadowResolution });
				command->SetDepthBias(0.005,0.0,0.0f);  // TODO: 这个东西怎么用的？
				command->PushConstants(&pointLightID, sizeof(uint32_t), SHADER_FREQUENCY_GRAPHICS);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(),0);
				meshPassProcessors[context.passIndex[0]]->Draw(command);
					})
				.OutputRead(depth)
				.OutputRead(color)  // 手动屏障
				.Finish();
		}
	}

}