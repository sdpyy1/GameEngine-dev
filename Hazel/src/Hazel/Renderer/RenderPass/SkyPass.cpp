#include "hzpch.h"
#include "SkyPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
namespace GameEngine {


	void SkyPass::Init()
	{
		{
			TransmittanceLutShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "TransmittanceLut.comp.spv", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo info= {};
			info.AddEntryFromReflect(TransmittanceLutShader);
			TransmittanceLutRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.rootSignature = TransmittanceLutRootSignature;
			pipelineInfo.computeShader = TransmittanceLutShader;
			TransmittanceLutPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{
			MultiScatteringLutShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "MultiScatteringLut.comp.spv", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
            RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(MultiScatteringLutShader)
				.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo());
            MultiScatteringLutRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
            RHIComputePipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = MultiScatteringLutRootSignature;
            pipelineInfo.computeShader = MultiScatteringLutShader;
            MultiScatteringLutPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{ 
			SkyViewLutShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "SkyViewLut.comp.spv", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
            RHIRootSignatureInfo info = {};
            info.AddEntryFromReflect(SkyViewLutShader)
				.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo())
				.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());

            SkyViewLutRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
            RHIComputePipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = SkyViewLutRootSignature;
            pipelineInfo.computeShader = SkyViewLutShader;
            SkyViewLutPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{
			m_VertShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "SkyVert.spv", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
			m_FragShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "SkyFrag.spv", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(m_VertShader)
				.AddEntryFromReflect(m_FragShader)
				.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo());

			m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIGraphicsPipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = m_RootSignature;
            pipelineInfo.vertexShader = m_VertShader;
            pipelineInfo.fragmentShader = m_FragShader;
			pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
			pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
			pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, false };
			pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
            pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
            m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
		}
	}

	void SkyPass::Build(RDGBuilder& builder)
	{

		// Transmittance Lut
		RDGTextureHandle TransmittanceLutTexture = builder.CreateTexture("TransmittanceLutTexture")
			.Exetent({ TrasmittanceLutWidth ,TrasmittanceLutHeight, 1})
			.AllowReadWrite()
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.Finish();


		builder.CreateComputePass("TransmittanceLutPass")
			.RootSignature(TransmittanceLutRootSignature)
			.ReadWrite(0, 0, 0, TransmittanceLutTexture)
			.Execute([&](RDGPassContext context) {
				RHICommandListRef command = context.command;
				command->SetComputePipeline(TransmittanceLutPipeline);
				command->BindDescriptorSet(context.descriptors[0], 0);
				command->Dispatch(TrasmittanceLutWidth / 8, TrasmittanceLutHeight / 8, 1);
			})
			.Finish();


		// MultiScattering Lut
        RDGTextureHandle MultiScatteringLutTexture = builder.CreateTexture("MultiScatteringLutTexture")
            .Exetent({ MultiScatteringLutResolution ,MultiScatteringLutResolution, 1})
            .AllowReadWrite()
            .Format(FORMAT_R8G8B8A8_UNORM)  // 不能用FORMAT_R32G32B32A32_SFLOAT 会报错
            .Finish();

		builder.CreateComputePass("MultiScatteringLutPass")
			.RootSignature(MultiScatteringLutRootSignature)
			.ReadWrite(0, 0, 0, MultiScatteringLutTexture)
			.Read(0, 1, 0, TransmittanceLutTexture)
			.Execute([&](RDGPassContext context) {
				RHICommandListRef command = context.command;
				command->SetComputePipeline(MultiScatteringLutPipeline);
				command->BindDescriptorSet(context.descriptors[0], 0);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
				command->Dispatch(MultiScatteringLutResolution / 8, MultiScatteringLutResolution / 8, 1);
			})
			.Finish();


		// SkyView Lut
        RDGTextureHandle SkyViewLutTexture = builder.CreateTexture("SkyViewLutTexture")
            .Exetent({ SkyViewLutWidth ,SkyViewLutHeight, 1})
            .AllowReadWrite()
            .Format(FORMAT_R32G32B32A32_SFLOAT)
            .Finish();

        builder.CreateComputePass("SkyViewLutPass")
            .RootSignature(SkyViewLutRootSignature)
            .ReadWrite(2, 0, 0, SkyViewLutTexture)
            .Read(2, 1, 0, TransmittanceLutTexture)
            .Read(2, 2, 0, MultiScatteringLutTexture)
            .Execute([&](RDGPassContext context) {
                RHICommandListRef command = context.command;
                command->SetComputePipeline(SkyViewLutPipeline);
                command->BindDescriptorSet(context.descriptors[2], 2);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
                command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
                command->Dispatch(SkyViewLutWidth / 8, SkyViewLutHeight / 8, 1);
            })
			.Finish();


		// Sky
		RDGTextureHandle ViewPort = builder.GetTexture("ViewPort");
		RDGTextureHandle skyBox = builder.GetTexture("CubeMap");
		RDGTextureHandle depth = builder.GetTexture("Depth");
		builder.CreateRenderPass("SkyPass")
			.RootSignature(m_RootSignature)
			.Read(2, 0, 0, skyBox, VIEW_TYPE_CUBE)
            .Read(2, 1, 0, SkyViewLutTexture)
            .Read(2, 2, 0, TransmittanceLutTexture)
            .Read(2, 3, 0, MultiScatteringLutTexture)
			.Color(0, ViewPort,ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE)
			.DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE)
			.Execute([&](RDGPassContext context) {
				auto [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetGraphicsPipeline(m_Pipeline);
				command->SetViewport({ 0, 0 }, { w,h });
				command->SetScissor({ 0, 0 }, { w,h });
				command->SetDepthBias(0.0f, 0.0f, 0.0f);
				command->BindDescriptorSet(context.descriptors[2], 2);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Draw(3);
			})
			.Finish();

	}
}

