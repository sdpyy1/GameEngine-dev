#include "hzpch.h"
#include "LightPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine {
	void LightPass::Init()
	{
		m_VertShader = std::make_shared<Shader>(APP_SHADER_PATH + "LightingVert.spv", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
		m_FragShader = std::make_shared<Shader>(APP_SHADER_PATH + "LightingFrag.spv", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntryFromReflect(m_VertShader)
			.AddEntryFromReflect(m_FragShader)
			.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.rootSignature = m_RootSignature;
		pipelineInfo.vertexShader = m_VertShader;
		pipelineInfo.fragmentShader = m_FragShader;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, false, false };
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}
    void LightPass::Build(RDGBuilder& builder)
    {

		auto [w,h] = APP_WINDOWSIZE;

		RDGTextureHandle ViewPort = builder.CreateTexture("ViewPort")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();

		RDGTextureHandle dirShadowMap = builder.GetTexture("CSMTextureArray");
		RDGTextureHandle position = builder.GetTexture("GBufferPosition");
		RDGTextureHandle normal = builder.GetTexture("GBufferNormal");
		RDGTextureHandle material = builder.GetTexture("GBufferMaterial");
		RDGTextureHandle albedo = builder.GetTexture("GBufferAlbedo");

        RDGTextureHandle envRadiance = builder.GetTexture("PrefilterMap");
        RDGTextureHandle envBRDF = builder.GetTexture("BRDFLut");
        RDGTextureHandle envIrradiance = builder.GetTexture("IrradianceMap");


		builder.CreateRenderPass(GetName())
			.RootSignature(m_RootSignature)
			.Color(0, ViewPort, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE)
			.Read(2, GBUFFER_POSITION_BINDING, 0, position)
			.Read(2, GBUFFER_NORMAL_BINDING, 0, normal)
			.Read(2, GBUFFER_MATERIAL_BINDING, 0, material)
			.Read(2, GBUFFER_ALBEDO_BINDING, 0, albedo)
			.Read(1, 0, 0, dirShadowMap, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_DEPTH ,0,1,0,4})
            .Read(1, 1, 0, envRadiance,VIEW_TYPE_CUBE,{TEXTURE_ASPECT_COLOR,0,builder.GetRHITexture("PrefilterMap")->GetInfo().mipLevels,0,6})
            .Read(1, 2, 0, envIrradiance, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR,0,1,0,6 })
            .Read(1, 3, 0, envBRDF)
			.Execute([&](RDGPassContext context)
				{
					auto [w, h] = APP_WINDOWSIZE;
					RHICommandListRef command = context.command;
					command->SetGraphicsPipeline(m_Pipeline);
					command->SetViewport({ 0, 0 }, { w,h });
					command->SetScissor({ 0, 0 }, { w,h });
					command->SetDepthBias(0.0f, 0.0f, 0.0f);
					command->BindDescriptorSet(Application::GetRenderSystem()->GetRenderResourceManager()->GetGlobalResourcePerFrameDescriptorSet(), 0);
					command->BindDescriptorSet(context.descriptors[1], 1);
					command->BindDescriptorSet(context.descriptors[2], 2);
					command->Draw(3);
				})
			.Finish();



    }
}