#include "hzpch.h"
#include "ClusterLightingPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine { 

	void ClusterLightingPass::Init()
	{
		m_Shader = std::make_shared<Shader>("culling/clusterLighting", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();

        RHIRootSignatureInfo info = {};
        info.AddEntryFromReflect(m_Shader).AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
        RHIComputePipelineInfo pipelineInfo = {};
        pipelineInfo.computeShader = m_Shader;
        pipelineInfo.rootSignature = m_RootSignature;
        m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
	}

	void ClusterLightingPass::Build(RDGBuilder& builder)
	{
		auto&[w,h] = APP_WINDOWSIZE;
		uint32_t clusterX = (w + LIGHT_CLUSTER_GRID_SIZE - 1) / LIGHT_CLUSTER_GRID_SIZE;
		uint32_t clusterY = (h + LIGHT_CLUSTER_GRID_SIZE - 1) / LIGHT_CLUSTER_GRID_SIZE;
		uint32_t clusterZ = LIGHT_CLUSTER_DEPTH;
		uint32_t clusterCount = clusterX * clusterY * clusterZ;


		// 存储每个簇的光源索引位置和数量
		RDGTextureHandle clusterTexture = builder.CreateTexture("ClusterLightingInfoTexture")
			.Exetent({ clusterX ,clusterY ,1 })
			.ArrayLayers(LIGHT_CLUSTER_DEPTH)
			.Format(FORMAT_R32G32_UINT)  // 只需要存两个int
			.MipLevels(1)
			.AllowReadWrite()
			.Finish();


		// 存储每个簇实际的光源ID
		RDGBufferHandle clusterIdBuffer = builder.CreateBuffer("ClusterLightingIdBuffer")
			.Size(clusterCount * sizeof(LightingClusterIdInfo) * MAX_LIGHTS_PER_CLUSTER)
			.AllowReadWrite()
			.Finish();

		builder.CreateComputePass(GetName())
			.PassIndex(clusterX, clusterY, clusterZ)
			.RootSignature(m_RootSignature)
			.ReadWrite(1,0,0,clusterTexture, VIEW_TYPE_2D_ARRAY, { TEXTURE_ASPECT_COLOR ,0,1,0,LIGHT_CLUSTER_DEPTH })
			.ReadWrite(1,1,0,clusterIdBuffer)
			.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;

				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_Pipeline);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->BindDescriptorSet(context.descriptors[1], 1);
				command->Dispatch((context.passIndex[0]+7)/8, (context.passIndex[1]+7)/8, context.passIndex[2]);
			}).OutputRead(clusterIdBuffer).OutputRead(clusterTexture, { TEXTURE_ASPECT_COLOR ,0,1,0,LIGHT_CLUSTER_DEPTH });

	}

}