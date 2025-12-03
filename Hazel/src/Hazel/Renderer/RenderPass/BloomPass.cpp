#include "hzpch.h"
#include "BloomPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine
{ 
	void BloomPass::Init()
	{
		SetEnable(true);
		m_Shader = std::make_shared<Shader>("postprocess/Bloom", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
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
		// 已经调整好，调整可能会报错，因为一些屏障状态的设置~
		if (IsEnabled()) {
			auto [w, h] = APP_WINDOWSIZE;
			RDGTextureHandle Viewport = builder.GetTexture("ViewPort");
			RDGTextureHandle Bloom = builder.CreateTexture("Bloom")
				.Exetent({ w,h,1 })
				.AllowReadWrite()
				.MipLevels(0) // auto mip
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();
			RDGTextureHandle UpBloom = builder.CreateTexture("UpBloom")
				.Exetent({ w,h,1 })
				.AllowReadWrite()
				.MipLevels(0) // auto mip
				.Format(FORMAT_R32G32B32A32_SFLOAT)
				.Finish();


			m_BloomComputePushConstants.Params = { m_BloomSettings.Threshold, m_BloomSettings.Threshold - m_BloomSettings.Knee, m_BloomSettings.Knee * 2.0f, 0.25f / m_BloomSettings.Knee };


			// Step1: 提取ViewPort亮的地方
			builder.CreateComputePass("Bloom_Prefilter")
				.RootSignature(m_RootSignature)
				.Read(2, 1, 0, Viewport)
				.ReadWrite(2, 0, 0, Bloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , 0, 1, 0, 1 })
				.Execute([&](RDGPassContext context) {
					auto [w, h] = APP_WINDOWSIZE;
					glm::vec3 workGroups = {(w + m_BloomComputeWorkgroupSize -1) / m_BloomComputeWorkgroupSize,(h + m_BloomComputeWorkgroupSize -1) / m_BloomComputeWorkgroupSize,1};
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

			// Step2: 下采样
			Extent3D extent = { w,h,1 };
			uint32_t mipLevels = extent.MipSize();

			for (uint32_t i = 1; i < mipLevels; i++) {
				m_BloomComputePushConstants.LOD = i - 1;
				RDGComputePassBuilder downBuilder = builder.CreateComputePass("Bloom_DownSample" + std::to_string(i))
					.PassIndex(i)
					.RootSignature(m_RootSignature)
					.Read(2, 1, 0, Bloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , i - 1, 1, 0, 1 })
					.ReadWrite(2, 0, 0, Bloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , i, 1, 0, 1 })
					.Execute([&](RDGPassContext context) {
						m_BloomComputePushConstants.Mode = 1;
						m_BloomComputePushConstants.LOD = 0;   // 因为我已经传递了正确的一层View，所以不需要指定，LOD=0就相当于传递的一层mip

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
					});


				if (i == mipLevels - 1) // 最后一级手动屏障
					downBuilder.OutputRead(Bloom, { TEXTURE_ASPECT_COLOR, (uint32_t)i, 1, 0, 1 });


				downBuilder.Finish();
			}


			// Step3: 上采样
			for (int i = mipLevels - 2; i >= 0; i--) {
				RDGComputePassBuilder downBuilder = builder.CreateComputePass("Bloom_UpSample" + std::to_string(i))
					.PassIndex(i)
					.RootSignature(m_RootSignature)
					.Read(2, 2, 0, Bloom)
					.Read(2, 1, 0, UpBloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , (uint32_t)(i + 1), 1, 0, 1 })
					.ReadWrite(2, 0, 0, UpBloom, VIEW_TYPE_2D, { TEXTURE_ASPECT_COLOR , (uint32_t)i, 1, 0, 1 })
					.Execute([&](RDGPassContext context) {
					RHICommandListRef command = context.command;
					m_BloomComputePushConstants.LOD = context.passIndex[0]+1;
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
						});
				if (i == 0) // 最后一级手动屏障
						downBuilder.OutputRead(UpBloom, { TEXTURE_ASPECT_COLOR, (uint32_t)i, 1, 0, 1 });


				downBuilder.Finish();
			}
		}



	}





}