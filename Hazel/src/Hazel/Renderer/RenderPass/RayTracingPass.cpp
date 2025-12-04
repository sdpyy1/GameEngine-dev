#include "hzpch.h"
#include "RayTracingPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
namespace GameEngine { 

	void RayTracingPass::Init()
	{
		m_RayGenShader = std::make_shared<Shader>("raytracing/RayLearn", SHADER_FREQUENCY_RAY_GEN)->GetRHIShader();
        m_MissShader = std::make_shared<Shader>("raytracing/RayLearn", SHADER_FREQUENCY_RAY_MISS)->GetRHIShader();
        m_ClosestHitShader = std::make_shared<Shader>("raytracing/RayLearn", SHADER_FREQUENCY_CLOSEST_HIT)->GetRHIShader();

		// SBT
		RHIShaderBindingTableInfo sbtInfo = {};
		sbtInfo.AddRayGenGroup(m_RayGenShader);
        sbtInfo.AddMissGroup(m_MissShader);
        sbtInfo.AddHitGroup(m_ClosestHitShader, nullptr, nullptr);
		RHIShaderBindingTableRef sbt = APP_DYNAMICRHI->CreateShaderBindingTable(sbtInfo);

		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
			.AddEntry({ 1, 0, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
			.AddEntry({ 1, 1, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE_CUBE });
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIRayTracingPipelineInfo pipelineInfo = {};
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.shaderBindingTable = sbt;
        m_Pipeline = APP_DYNAMICRHI->CreateRayTracingPipeline(pipelineInfo);
	}

	void RayTracingPass::Build(RDGBuilder& builder)
	{
		auto &[w,h] = APP_WINDOWSIZE;
		RDGTextureHandle rayTexture = builder.CreateTexture("RayColor")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();

		RDGTextureHandle skyBox = builder.GetTexture("CubeMap");


		builder.CreateRayTracingPass(GetName())
			.RootSignature(m_RootSignature)
			.ReadWrite(1, 0, 0, rayTexture)
			.Read(1, 1, 0, skyBox, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR ,0,1,0,6})
			.Execute([&](RDGPassContext context) {
			auto&[w, h] = APP_WINDOWSIZE;
				RHICommandListRef command = context.command;
				command->SetRayTracingPipeline(m_Pipeline);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->BindDescriptorSet(context.descriptors[1], 1);
				// command->PushConstants(&setting, sizeof(RayTracingBaseSetting), SHADER_FREQUENCY_RAY_TRACING);
				command->TraceRays(w,h,1);  // 其实和dispatch道理一样
			})
			.Finish();


	}






}