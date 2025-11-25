#include "hzpch.h"
#include "GbufferPass.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderSystem/RenderSystem.h>
namespace GameEngine
{
	void GBufferPassProcessor::OnCollectBatch(const DrawBatch& batch)
	{
		if (batch.material->RenderPassMask() & V2::PASS_MASK_DEFERRED_PASS) AddBatch(batch);
	}

	RHIGraphicsPipelineRef GBufferPassProcessor::OnCreatePipeline(const DrawPipelineState& pipelineState)
	{
		RHIGraphicsPipelineRef pipeline;

		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.vertexShader = pipelineState.vertexShader;
		pipelineInfo.geometryShader = pipelineState.geometryShader;
		pipelineInfo.fragmentShader = pipelineState.fragmentShader;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { pipelineState.fillMode, pipelineState.cullMode, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { pipelineState.depthCompare, pipelineState.depthTest, pipelineState.depthWrite };

		pipelineInfo.rootSignature = pass->rootSignature;
		for (uint32_t i = 0; i < 4; i++) pipelineInfo.blendState.renderTargets[i].enable = false;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R8G8B8A8_UNORM;
		pipelineInfo.colorAttachmentFormats[1] = FORMAT_R8G8B8A8_SNORM;
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;
		if (pipeline) return pipeline;

		pipelineInfo.vertexShader = pipelineState.clusterRender ?
			pass->clusterVertexShader->GetRHIShader() :
			pass->vertexShader->GetRHIShader();                          // 用默认着色器
		pipelineInfo.geometryShader = nullptr;
		pipelineInfo.fragmentShader = pass->fragmentShader->GetRHIShader();
		pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;
		if (pipeline) return pipeline;

		if (pipelineState.clusterRender) return pass->clusterPipeline;                       // 用默认管线
		else                            return pass->pipeline;
	}

	void GBufferPass::Init()
	{
		meshPassProcessor = std::make_shared<GBufferPassProcessor>(this);
		MeshPass::Init();

		vertexShader = std::make_shared<V2::Shader>("Assets/Shader/spv/newGbufferVert.spv", SHADER_FREQUENCY_VERTEX);
		fragmentShader = std::make_shared<V2::Shader>("Assets/Shader/spv/newGbufferFrag.spv", SHADER_FREQUENCY_FRAGMENT);



		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());  // Set=0 全局资源
		rootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.vertexShader = vertexShader->GetRHIShader();
		pipelineInfo.fragmentShader = fragmentShader->GetRHIShader();
		pipelineInfo.rootSignature = rootSignature;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_BACK, DEPTH_CLIP, 0.0f, 0.0f };
		for (uint32_t i = 0; i < 4; i++) pipelineInfo.blendState.renderTargets[i].enable = false;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R8G8B8A8_UNORM;
		pipelineInfo.colorAttachmentFormats[1] = FORMAT_R8G8B8A8_SNORM;
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;    // 普通mesh的默认绘制管线
	}

	void GBufferPass::Build(RDGBuilder& builder)
	{

		auto [w, h] = APP_WINDOWSIZE;

		RDGTextureHandle diffuse = builder.CreateTexture("G-Buffer Diffuse/Roughness")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R8G8B8A8_UNORM)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle normal = builder.CreateTexture("G-Buffer Normal/Metallic")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R8G8B8A8_SNORM)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();

		RDGTextureHandle depth = builder.CreateTexture("Depth")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_D32_SFLOAT)
			.AllowDepthStencil()
			.Finish();
		if (IsEnabled()) {
			builder.CreateRenderPass(GetName())
				.Color(0, diffuse, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f })
				.Color(1, normal, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f })
				.DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
				.Execute([&](RDGPassContext context) {

				auto [w, h] = APP_WINDOWSIZE;


				RHICommandListRef command = context.command;
				command->SetGraphicsPipeline(pipeline);
				command->SetViewport({ 0, 0 }, { w,h });
				command->SetScissor({ 0, 0 }, { w,h });
				command->SetDepthBias(0.0f, 0.0f, 0.0f);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);









					}
				)
				.Finish();

		}
	}




}