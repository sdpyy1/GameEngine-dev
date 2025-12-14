#include "hzpch.h"
#include "PostProcessPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>

// ÆØ¹â¡¢ ToneMapping ¡¢ ColorGrading
namespace GameEngine {
	void PostProcessPass::Init()
	{
		{
			m_VertShader = std::make_shared<Shader>("postprocess/FinalColor", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
			m_FragShader = std::make_shared<Shader>("postprocess/FinalColor", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(m_VertShader)
				.AddEntryFromReflect(m_FragShader)
				.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo())
				.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddPushConstant({ 128, SHADER_FREQUENCY_FRAGMENT });
			m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIGraphicsPipelineInfo pipelineInfo = {};
			pipelineInfo.rootSignature = m_RootSignature;
			pipelineInfo.vertexShader = m_VertShader;
			pipelineInfo.fragmentShader = m_FragShader;
			pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
			pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
			pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
			pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
			m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo); 
		}
	}

	void PostProcessPass::Build(RDGBuilder& builder)
	{
		auto [w, h] = APP_WINDOWSIZE;

		RDGTextureHandle RenderRes = builder.CreateTexture("RenderRes")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle viewport = builder.GetTexture("ViewPort");
		RDGBufferHandle exposureData = builder.GetBuffer("ExposureData");
		RDGTextureHandle bloomRes = builder.GetTexture("UpBloom");

		builder.CreateRenderPass("PostProcess")
			.RootSignature(m_RootSignature)
			.Color(0, RenderRes,ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE)
			.Read(2, 0, 0, viewport)
			.Read(2, 1, 0, bloomRes)
			.ReadWrite(2, 2, 0, exposureData)
			.Execute([&](RDGPassContext context) {
				auto [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetGraphicsPipeline(m_Pipeline);
				command->SetViewport({ 0, 0 }, { w,h });
				command->SetScissor({ 0, 0 }, { w,h });
				command->SetDepthBias(0.0f, 0.0f, 0.0f);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
				command->BindDescriptorSet(context.descriptors[2], 2);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->PushConstants(&m_Setting, sizeof(PostProcessingSetting), SHADER_FREQUENCY_FRAGMENT);
				command->Draw(3);
			})
			.Finish();

	}
}
