#include "hzpch.h"
#include "GbufferPass.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderSystem/RenderSystem.h>
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine
{
	void GBufferPassProcessor::OnCollectBatch(const DrawBatch& batch)
	{
		if (batch.material->RenderPassMask() & PASS_MASK_DEFERRED_PASS) AddBatch(batch);
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
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.colorAttachmentFormats[1] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.colorAttachmentFormats[2] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.colorAttachmentFormats[3] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;

		// TODO: 如果材质自带了Shader，可以直接在这里就创建管线并返回，否则就是当前Pass自带的Shader信息
		// pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;
		//if (pipeline) return pipeline;   // 材质自带Shader

		pipelineInfo.vertexShader = pass->vertexShader->GetRHIShader();                          // 用默认着色器
		pipelineInfo.geometryShader = nullptr;
		pipelineInfo.fragmentShader = pass->fragmentShader->GetRHIShader();
		pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;
		if (pipeline) return pipeline;
		LOG_ERROR("[GBufferPass] Failed to create pipeline");
		return nullptr;
	}

	void GBufferPass::Init()
	{
		meshPassProcessor = std::make_shared<GBufferPassProcessor>(this);
		MeshPass::Init();

		vertexShader = std::make_shared<Shader>("mesh/Gbuffer", SHADER_FREQUENCY_VERTEX);
		fragmentShader = std::make_shared<Shader>("mesh/Gbuffer", SHADER_FREQUENCY_FRAGMENT);

		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());  // Set=0 全局资源
		rootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.vertexShader = vertexShader->GetRHIShader();
		pipelineInfo.fragmentShader = fragmentShader->GetRHIShader();
		pipelineInfo.rootSignature = rootSignature;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_FRONT, DEPTH_CLIP, 0.0f, 0.0f };
		for (uint32_t i = 0; i < 4; i++) pipelineInfo.blendState.renderTargets[i].enable = false;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.colorAttachmentFormats[1] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.colorAttachmentFormats[2] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.colorAttachmentFormats[3] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, false };   // 不需要写入深度了
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		pipeline = GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;
	}

	void GBufferPass::Build(RDGBuilder& builder)
	{

		auto [w, h] = APP_WINDOWSIZE;

		RDGTextureHandle position = builder.CreateTexture("GBufferPosition")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle normal = builder.CreateTexture("GBufferNormal")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle material = builder.CreateTexture("GBufferMaterial")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle albedo = builder.CreateTexture("GBufferAlbedo")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle depth = builder.GetTexture("Depth");


		if (IsEnabled()) {
			builder.CreateRenderPass(GetName())
				.Color(0, position, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 1.0f })
				.Color(1, normal, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 1.0f })
				.Color(2, material, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 1.0f })
				.Color(3, albedo, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 1.0f })
				.DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
				.Execute([&](RDGPassContext context) {
					auto [w, h] = APP_WINDOWSIZE;
					RHICommandListRef command = context.command;
					command->SetGraphicsPipeline(pipeline);
					command->SetViewport({ 0, 0 }, { w,h });
					command->SetScissor({ 0, 0 }, { w,h });
					command->SetDepthBias(0.0f, 0.0f, 0.0f);
					command->BindDescriptorSet(Application::GetRenderSystem()->GetRenderResourceManager()->GetGlobalResourcePerFrameDescriptorSet(), 0);
					meshPassProcessor->Draw(command);
				})
				.Finish();

		}
	}




}