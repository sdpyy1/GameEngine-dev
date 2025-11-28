#include "hzpch.h"
#include "BloomPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
namespace GameEngine
{ 
	void BloomPass::Init()
	{
		SetEnable(true);
		m_Shader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "Bloom.comp.spv", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntryFromReflect(m_Shader);
		info.AddPushConstant({ sizeof(bloomComputePushConstants),SHADER_FREQUENCY_COMPUTE })
			.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo())
			.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
        m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIComputePipelineInfo pipelineInfo = {};
        pipelineInfo.computeShader = m_Shader;
        pipelineInfo.rootSignature = m_RootSignature;
        m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
	}

	void BloomPass::Build(RDGBuilder& builder)
	{
		if (IsEnabled()) {
			auto [w, h] = APP_WINDOWSIZE;
			RDGTextureHandle Bloom = builder.CreateTexture("Bloom")
				.Exetent({ w,h,1 })
				.AllowReadWrite()
				.MipLevels(0) // auto mip
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();
			RDGTextureHandle BloomDonwSample = builder.CreateTexture("BloomDonwSample")
				.Exetent({ w,h,1 })
				.AllowReadWrite()
				.MipLevels(0) // auto mip
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();
			RDGTextureHandle BloomUpSample = builder.CreateTexture("BloomUpSample")
				.Exetent({ w,h,1 })
				.AllowReadWrite()
				.MipLevels(0) // auto mip
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();
			RDGTextureHandle Viewport = builder.GetTexture("ViewPort");


			m_BloomComputePushConstants.Params = { m_BloomSettings.Threshold, m_BloomSettings.Threshold - m_BloomSettings.Knee, m_BloomSettings.Knee * 2.0f, 0.25f / m_BloomSettings.Knee };


			// Step1: 提取ViewPort亮的地方
			builder.CreateComputePass("Bloom_Prefilter")
				.RootSignature(m_RootSignature)
				.Read(2, 1, 0, Viewport)
				.ReadWrite(2, 0, 0, Bloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , 0, 1, 0, 1 })
				.Execute([&](RDGPassContext context) {
					auto [w, h] = APP_WINDOWSIZE;
					glm::vec3 workGroups = {
						(w + m_BloomComputeWorkgroupSize +1) / m_BloomComputeWorkgroupSize,
						(h + m_BloomComputeWorkgroupSize +1) / m_BloomComputeWorkgroupSize,
						1
					};
					m_BloomComputePushConstants.Mode = 0;
					m_BloomComputePushConstants.LOD = 0;
					RHICommandListRef command = context.command;
					command->SetComputePipeline(m_Pipeline);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
					command->BindDescriptorSet(context.descriptors[2], 2);
					command->PushConstants(&m_BloomComputePushConstants, sizeof(bloomComputePushConstants), SHADER_FREQUENCY_COMPUTE);
					command->Dispatch(workGroups.x, workGroups.y, workGroups.z);
				})
				.Finish();


			builder.CreateCopyPass("Bloom_Copy")
				.From(Bloom)
				.To(BloomDonwSample)
				.OutputReadWrite(BloomDonwSample)
				.OutputRead(Bloom)
				.Finish();

			// Step2: 下采样
			Extent3D extent = { w,h,1 };
			uint32_t mipLevels = extent.MipSize();


			for (uint32_t i = 1; i < mipLevels; i++) {
				m_BloomComputePushConstants.LOD = i - 1;
				builder.CreateComputePass("Bloom_DownSample" + std::to_string(i))
					.PassIndex(i)
					.RootSignature(m_RootSignature)
					.ReadWrite(2, 0, 0, Bloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , i, 1, 0, 1 })   // 同时绑定同一个纹理的Read和ReadWrite，可以跑通，但是会导致现在的布局转换出问题报错
					.Read(2, 1, 0, Bloom)
					.Execute([&](RDGPassContext context) {
					m_BloomComputePushConstants.Mode = 1;
					m_BloomComputePushConstants.LOD = context.passIndex[0] - 1;

					RHICommandListRef command = context.command;
					auto [w, h] = APP_WINDOWSIZE;
					Extent3D extent = { w,h,1 };
					auto [mipWidth, mipHeight, height] = extent.GetMipExtent(context.passIndex[0]);
					glm::vec3 workGroups = { (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize) ,(uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize), 1 };
					command->SetComputePipeline(m_Pipeline);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
					command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
					command->BindDescriptorSet(context.descriptors[2], 2);
					command->PushConstants(&m_BloomComputePushConstants, sizeof(bloomComputePushConstants), SHADER_FREQUENCY_COMPUTE);
					command->Dispatch(workGroups.x, workGroups.y, workGroups.z);
						})
					.Finish();
			}
			/*builder.CreateCopyPass("Bloom_Copy1")
				.From(BloomDonwSample)
				.To(Bloom)
				.OutputReadWrite(Bloom)
				.OutputRead(BloomDonwSample)
				.Finish();*/

			// Step3: 上采样
			for (int i = mipLevels - 2; i >= 0; i--) {
				builder.CreateComputePass("Bloom_UpSample" + std::to_string(i))
					.PassIndex(i)
					.RootSignature(m_RootSignature)
					.Read(2, 1, 0, Bloom)
					.ReadWrite(2, 0, 0, Bloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , (uint32_t)i, 1, 0, 1 })   // 某一层Mip
					.Execute([&](RDGPassContext context) {
						RHICommandListRef command = context.command;
						m_BloomComputePushConstants.LOD = context.passIndex[0];
						m_BloomComputePushConstants.Mode = 3;
						auto [w, h] = APP_WINDOWSIZE;

						Extent3D extent = { w,h,1 };
						auto [mipWidth, mipHeight, height] = extent.GetMipExtent(context.passIndex[0]);
						glm::vec3 workGroups;
						workGroups.x = (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize);
						workGroups.y = (uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize);
						workGroups.z = 1;
						command->SetComputePipeline(m_Pipeline);
						command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
						command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
						command->BindDescriptorSet(context.descriptors[2], 2);
						command->PushConstants(&m_BloomComputePushConstants, sizeof(bloomComputePushConstants), SHADER_FREQUENCY_COMPUTE);
						command->Dispatch(workGroups.x, workGroups.y, workGroups.z);
					})
					.Finish();
			}
		}



	}





}