#include "hzpch.h"
#include "SVGFPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine {
	void SVGFPass::Init()
	{
		{
			m_AtrousShader = std::make_shared<Shader>("SVGF/SVGF", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(m_AtrousShader);
			info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddPushConstant({ 4,SHADER_FREQUENCY_COMPUTE });
			m_AtrousRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.computeShader = m_AtrousShader;
			pipelineInfo.rootSignature = m_AtrousRootSignature;
			m_AtrousPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}
		{
			m_CombineShader = std::make_shared<Shader>("SVGF/combine", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(m_CombineShader);
			info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddPushConstant({ 4,SHADER_FREQUENCY_COMPUTE });
			m_CombineRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.computeShader = m_CombineShader;
			pipelineInfo.rootSignature = m_CombineRootSignature;
			m_CombinePipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}
		{
			m_VarianceShader = std::make_shared<Shader>("SVGF/variance", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(m_VarianceShader);
			info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddPushConstant({ 4,SHADER_FREQUENCY_COMPUTE });
			m_VarianceRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.computeShader = m_VarianceShader;
			pipelineInfo.rootSignature = m_VarianceRootSignature;
			m_VariancePipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}
		{
			m_MixHistoryShader = std::make_shared<Shader>("SVGF/mixHistory", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
			RHIRootSignatureInfo info = {};
			info.AddEntryFromReflect(m_MixHistoryShader);
			info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
				.AddPushConstant({ 4,SHADER_FREQUENCY_COMPUTE });
			m_MixHistoryRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.computeShader = m_MixHistoryShader;
			pipelineInfo.rootSignature = m_MixHistoryRootSignature;
			m_MixHistoryPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{
			auto& [w, h] = APP_WINDOWSIZE;
			RHITextureInfo textureInfo;
			textureInfo.extent = { w, h, 1 };
			textureInfo.format = RHIFormat::FORMAT_R32G32B32A32_SFLOAT;
			textureInfo.mipLevels = 1;
			textureInfo.arrayLayers = 1;
			textureInfo.type |= RESOURCE_TYPE_RW_TEXTURE;
			m_DirectHistory = APP_DYNAMICRHI->CreateTexture(textureInfo);
			m_IndirectHistory = APP_DYNAMICRHI->CreateTexture(textureInfo);

			m_DirVarianceHistory = APP_DYNAMICRHI->CreateTexture(textureInfo);
			m_InDirVarianceHistory = APP_DYNAMICRHI->CreateTexture(textureInfo);
		}
	}

	void SVGFPass::Build(RDGBuilder& builder)
	{
		auto& [w, h] = APP_WINDOWSIZE;
	// 路径追踪结果
		
		RDGTextureHandle pathTracingDirectRes = builder.GetTexture("PathTracingdirectRes");
		if (pathTracingDirectRes.ID() == UINT32_MAX) { return; }
		RDGTextureHandle pathTracingIndirectRes = builder.GetTexture("PathTracingIndirectRes");
		

		
		/////////////////////////////////////////// 历史信息///////////////////////////////////////////
		
		// 直接光方差历史
		RDGTextureHandle dirVarianceHistory = builder.CreateTexture("PathTracing_SVGF_dirVarianceHistory")
			.Import(m_DirVarianceHistory, RESOURCE_STATE_UNDEFINED).Finish();

		// 间接光方差历史
		RDGTextureHandle inDirVarianceHistory = builder.CreateTexture("PathTracing_SVGF_inDirVariance")
			.Import(m_InDirVarianceHistory, RESOURCE_STATE_UNDEFINED).Finish();

		// 直接光历史
		RDGTextureHandle directHistory = builder.CreateTexture("PathTracing_SVGF_directHistory")
			.Import(m_DirectHistory, RESOURCE_STATE_UNDEFINED).Finish();

		// 间接光历史
		RDGTextureHandle inDirectHistory = builder.CreateTexture("PathTracing_SVGF_inDirectHistory")
			.Import(m_IndirectHistory, RESOURCE_STATE_UNDEFINED).Finish();
		

		/////////////////////////////////////////// 存储当前 ///////////////////////////////////////////
		
		// 直接光混合结果
		RDGTextureHandle DirMixRes = builder.CreateTexture("PathTracing_SVGF_DirMixRes")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		// 间接光混合结果
		RDGTextureHandle InDirMixRes = builder.CreateTexture("PathTracing_SVGF_InDirMixRes")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();


		// 直接光方差混合结果
		RDGTextureHandle DirVariance = builder.CreateTexture("PathTracing_SVGF_dirvariance")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		// 间接光混合结果
		RDGTextureHandle inDirVariance = builder.CreateTexture("PathTracing_SVGF_Indirvariance")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		// 直接光滤波结果
		RDGTextureHandle directFilterRes = builder.CreateTexture("PathTracing_SVGF_direct")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		// 间接光滤波结果
		RDGTextureHandle inDirectFilterRes = builder.CreateTexture("PathTracing_SVGF_inDirect")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		// pingpong
		RDGTextureHandle forPinpongTexture = builder.CreateTexture("PathTracing_SVGF_forPinpongTexture")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		// 最终结果
		RDGTextureHandle SVGFCombineRes = builder.CreateTexture("PathTracing_SVGF_CombineRes")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		


		/////////////////////////////////////////// 其他信息///////////////////////////////////////////
	
		RDGTextureHandle position = builder.GetTexture("GBufferPosition");
		RDGTextureHandle normal = builder.GetTexture("GBufferNormal");
		RDGTextureHandle material = builder.GetTexture("GBufferMaterial");
		RDGTextureHandle albedo = builder.GetTexture("GBufferAlbedo");
		RDGTextureHandle velocity = builder.GetTexture("GBufferVelocity");
		RDGTextureHandle depth = builder.GetTexture("Depth");
		RDGBufferHandle exposureData = builder.GetBuffer("ExposureData");
	




		///////////////////////////////////////////// 累积历史Pass ///////////////////////////////////////////
		builder.CreateComputePass(GetName() + "_MixHistory")
			.RootSignature(m_MixHistoryRootSignature)
			.ReadWrite(1, 0, 0, DirMixRes)  // 存储直接光结果
			.ReadWrite(1, 1, 0, InDirMixRes) // 存储间接光结果
			.ReadWrite(1, 2, 0, directHistory)
			.ReadWrite(1, 3, 0, inDirectHistory)
			.ReadWrite(1, 4, 0, DirVariance)
			.ReadWrite(1, 5, 0, inDirVariance)
			.ReadWrite(1, 6, 0, dirVarianceHistory)
			.ReadWrite(1, 7, 0, inDirVarianceHistory)
			.Read(1, 8, 0, velocity)
			.ReadWrite(1, 9, 0, pathTracingDirectRes)
			.ReadWrite(1, 10, 0, pathTracingIndirectRes)

			.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_MixHistoryPipeline);
				command->BindDescriptorSet(context.descriptors[1], 1);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Dispatch((w + 15) / 16, (h + 15) / 16, 1);
			});

		builder.CreateCopyPass(GetName() + "_CopyDirColor")
			.From(DirMixRes)
			.To(directHistory)
			.Finish();
		builder.CreateCopyPass(GetName() + "_CopyDirVariance")
			.From(DirVariance)
			.To(dirVarianceHistory)
			.Finish();
		builder.CreateCopyPass(GetName() + "_CopyInDirColor")
			.From(InDirMixRes)
			.To(inDirectHistory)
			.Finish();
		builder.CreateCopyPass(GetName() + "_CopyInDirVariance")
			.From(inDirVariance)
			.To(inDirVarianceHistory)
			.Finish();







		//// 方差更新
		//builder.CreateComputePass(GetName() + "_VarianceUpdate")
		//	.RootSignature(m_AtrousRootSignature)
		//	.ReadWrite(1, 0, 0, variance)
		//	.Execute([&](RDGPassContext context) {
		//		auto& [w, h] = APP_WINDOWSIZE;
		//		RHICommandListRef command = context.command;
		//	});





		// 直接光
		for (int i = 0; i < 5; i++) {
			builder.CreateComputePass(GetName() + "_DirectRes" + std::to_string(i))
				.RootSignature(m_AtrousRootSignature)
				.PassIndex(i)
				.ReadWrite(1, 0, 0, i == 0 ? directFilterRes : i % 2 == 1 ? forPinpongTexture : directFilterRes)
				.ReadWrite(1, 1, 0, i == 0 ? DirMixRes : i % 2 == 1 ? directFilterRes : forPinpongTexture)
				.ReadWrite(1, 2, 0, velocity)
				.ReadWrite(1, 3, 0, directHistory)
				.ReadWrite(1, 4, 0, position)
				.ReadWrite(1, 5, 0, normal)
				.ReadWrite(1, 6, 0, albedo)
				.ReadWrite(1, 7, 0, albedo)
				.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_AtrousPipeline);
				command->BindDescriptorSet(context.descriptors[1], 1);
				int curIndex = context.passIndex[0];
				command->PushConstants(&curIndex, sizeof(uint32_t), SHADER_FREQUENCY_COMPUTE);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Dispatch((w + 15) / 16, (h + 15) / 16, 1);
					})
				.Finish();
		}



		// 间接光
		for (int i = 0; i < 5; i++) {
			builder.CreateComputePass(GetName() + "_InDirectRes" + std::to_string(i))
				.RootSignature(m_AtrousRootSignature)
				.PassIndex(i)
				.ReadWrite(1, 0, 0, i == 0 ? inDirectFilterRes : i % 2 == 1 ? forPinpongTexture : inDirectFilterRes)
				.ReadWrite(1, 1, 0, i == 0 ? InDirMixRes : i % 2 == 1 ? inDirectFilterRes : forPinpongTexture)
				.ReadWrite(1, 2, 0, velocity)
				.ReadWrite(1, 3, 0, inDirectHistory)
				.ReadWrite(1, 4, 0, position)
				.ReadWrite(1, 5, 0, normal)
				.ReadWrite(1, 6, 0, albedo)
				.ReadWrite(1, 7, 0, albedo)
				.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_AtrousPipeline);
				command->BindDescriptorSet(context.descriptors[1], 1);
				uint32_t curIndex = context.passIndex[0];
				command->PushConstants(&curIndex, sizeof(uint32_t), SHADER_FREQUENCY_COMPUTE);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Dispatch((w + 15) / 16, (h + 15) / 16, 1);
					})
				.Finish();
		}


		builder.CreateComputePass(GetName() + "_Res")
			.RootSignature(m_CombineRootSignature)
			.ReadWrite(1, 0, 0, SVGFCombineRes)
			.ReadWrite(1, 1, 0, directFilterRes)
			.ReadWrite(1, 2, 0, inDirectFilterRes)
			.ReadWrite(1, 3, 0, albedo)
			.ReadWrite(1, 4, 0, exposureData)

			.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_CombinePipeline);
				command->BindDescriptorSet(context.descriptors[1], 1);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->Dispatch((w + 15) / 16, (h + 15) / 16, 1);
			})
			.Finish();

	}

}