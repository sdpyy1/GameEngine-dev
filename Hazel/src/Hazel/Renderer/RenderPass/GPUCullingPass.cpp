#include "hzpch.h"
#include "GPUCullingPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
#include "Hazel/Renderer/RenderPass/MeshPass.h"
namespace GameEngine {
	void GPUCullingPass::Init()
	{
		m_Shader = std::make_shared<Shader>("culling/meshCulling", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();

		RHIRootSignatureInfo rootSignatureInfo;
		rootSignatureInfo.AddEntryFromReflect(m_Shader)
			.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIComputePipelineInfo pipelineInfo;
        pipelineInfo.computeShader = m_Shader;
        pipelineInfo.rootSignature = m_RootSignature;
        m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);

	}
	void GPUCullingPass::Build(RDGBuilder& builder)
	{
		auto& passbuilder = builder.CreateComputePass("GPUCullingPass")
			.RootSignature(m_RootSignature);
		uint32_t passIndex = 0;
		for (auto& meshPass : APP_RENDERSYSTEM->GetMeshPasses()) {
			if (meshPass->GetMeshPassProcessors()->GetDrawCommandCount() > 0) {

				//if(meshPass->GetType() == DIR_SHADOW_PASS)




				auto& commandBuffer = meshPass->GetMeshPassProcessors()->GetMeshIndirectDrawDataBuffer();
				RDGBufferHandle MeshIndirectDrawDataBuffer = builder.CreateBuffer("MeshIndirectDrawDataBuffer" + std::to_string(passIndex))
					.Import(commandBuffer, RESOURCE_STATE_UNDEFINED)
					.Finish();
				passbuilder.ReadWrite(1, 0, passIndex, MeshIndirectDrawDataBuffer)
					.OutputIndirectDraw(MeshIndirectDrawDataBuffer);
				passIndex++;
			}
		}

		passbuilder.RootSignature(m_RootSignature)
			.PassIndex(passIndex)
			.Execute([&](RDGPassContext context) {
				if (context.passIndex[0] == 0) {
					return;
				}
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_Pipeline);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->BindDescriptorSet(context.descriptors[1], 1);
				command->Dispatch(MAX_PER_FRAME_INSTANCE_SIZE / 32, context.passIndex[0], 1);
			});

	}
}