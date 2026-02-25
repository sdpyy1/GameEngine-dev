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
		meshPassProcessors.emplace_back(std::make_shared<PreDepthPassProcessor>(this));
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


		{
			m_HZBShader = std::make_shared<Shader>("mesh/HZB", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
			info.AddEntryFromReflect(m_HZBShader);
			m_HZBSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.computeShader = m_HZBShader;
			pipelineInfo.rootSignature = m_HZBSignature;
			m_HZBPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}
	}

	void PreDepthPass::Build(RDGBuilder& builder)
	{
		auto [w, h] = APP_WINDOWSIZE;
		RDGTextureHandle depth = builder.CreateTexture("Depth")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_D32_SFLOAT)
			.AllowDepthStencil()
			.Finish();

		RDGTextureHandle HZB = builder.CreateTexture("HZB")
			.Import(RENDER_RESOURCEMANAGER->GetHZB(), RESOURCE_STATE_UNDEFINED)
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
			meshPassProcessors[0]->Draw(command);
				})
			.Finish();

		builder.CreateCopyPass("Copy Depth")
			.From(depth)
			.To(HZB)
			.OutputReadWrite(HZB)
			.Finish();

		Extent3D extent = { w,h,1 };
		uint32_t mipLevels = extent.MipSize();


		// 生成HZB  
		// TODO:UE4有更好的方法，一次Dispatch生成4个Mip层
		for (uint32_t i = 1; i < mipLevels; i++){
			builder.CreateComputePass(GetName() + "_HZB")
				.ReadWrite(1, 0, 0, HZB, VIEW_TYPE_2D, { TEXTURE_ASPECT_DEPTH ,i,1,0,1 })
				.ReadWrite(1, 1, 0, HZB, VIEW_TYPE_2D, { TEXTURE_ASPECT_DEPTH ,i-1,1,0,1 })
				.RootSignature(m_HZBSignature)
				.PassIndex(i)
				.Execute([&](RDGPassContext context) {
				auto [w, h] = APP_WINDOWSIZE;
				Extent3D extent = { w,h,1 };
				extent = extent.GetMipExtent(context.passIndex[0]);
				w = extent.width;
                h = extent.height;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_HZBPipeline);
				command->BindDescriptorSet(Application::GetRenderSystem()->GetRenderResourceManager()->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->BindDescriptorSet(context.descriptors[1], 1);
				uint32_t groupSizeX = 8;
				uint32_t groupSizeY = 8;
				uint32_t numGroupsX = (w + groupSizeX - 1) / groupSizeX;
				uint32_t numGroupsY = (h + groupSizeY - 1) / groupSizeY;
				command->Dispatch(numGroupsX, numGroupsY, 1);
					})
				.Finish();
		}



	}

}