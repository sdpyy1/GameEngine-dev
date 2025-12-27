#include "hzpch.h"
#include "PathTracingPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include <Hazel/Renderer/RenderResource/Shader.h>
#include "Hazel/Core/Input.h"
namespace GameEngine { 

	void PathTracingPass::Init()
	{
		m_RayGenShader = std::make_shared<Shader>("raytracing/PathTracingForSVGF", SHADER_FREQUENCY_RAY_GEN)->GetRHIShader();
        m_MissShader = std::make_shared<Shader>("raytracing/PathTracingForSVGF", SHADER_FREQUENCY_RAY_MISS)->GetRHIShader();
        m_ClosestHitShader = std::make_shared<Shader>("raytracing/PathTracingForSVGF", SHADER_FREQUENCY_CLOSEST_HIT)->GetRHIShader();

		RHIShaderBindingTableInfo sbtInfo = {};
		sbtInfo.AddRayGenGroup(m_RayGenShader);
        sbtInfo.AddMissGroup(m_MissShader);
        sbtInfo.AddHitGroup(m_ClosestHitShader, nullptr, nullptr);
		RHIShaderBindingTableRef sbt = APP_DYNAMICRHI->CreateShaderBindingTable(sbtInfo);

		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo())
			.AddEntry({ 1, 0, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
			.AddEntry({ 1, 1, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
			.AddEntry({ 1, 2, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_TEXTURE_CUBE })
			.AddEntry({ 1, 3, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_BUFFER })
			.AddEntry({ 1, 4, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
			.AddEntry({ 1, 5, 1, SHADER_FREQUENCY_RAY_TRACING, RESOURCE_TYPE_RW_TEXTURE })
			.AddPushConstant({ 128, SHADER_FREQUENCY_RAY_TRACING });

		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);

		RHIRayTracingPipelineInfo pipelineInfo = {};
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.shaderBindingTable = sbt;
        m_Pipeline = APP_DYNAMICRHI->CreateRayTracingPipeline(pipelineInfo);

		auto& [w, h] = APP_WINDOWSIZE;
		RHITextureInfo textureInfo = {};
        textureInfo.extent = { w, h, 1 };
		textureInfo.format = FORMAT_R32G32B32A32_SFLOAT;
        textureInfo.mipLevels = 1;
        textureInfo.arrayLayers = 1;
		textureInfo.type = RESOURCE_TYPE_RW_TEXTURE | RESOURCE_TYPE_TEXTURE;
		m_HistoryTexture = APP_DYNAMICRHI->CreateTexture(textureInfo);
		m_DirectTexture = APP_DYNAMICRHI->CreateTexture(textureInfo);
		m_InDirectTexture = APP_DYNAMICRHI->CreateTexture(textureInfo);

	}

	void PathTracingPass::Build(RDGBuilder& builder)
	{
		PathTracingSetting globalSetting = RENDER_RESOURCEMANAGER->GetGlobalSettingInfo().postprocess.pathTracingSetting;
		if (globalSetting.enable == 0) {
			return;
		}
		m_Settings.numBounce = globalSetting.numBounce;
        m_Settings.numSamples = globalSetting.numSamples;
        m_Settings.sampleSkyBox = globalSetting.sampleSkyBox;
        m_Settings.indirectOnly = globalSetting.indirectOnly;
        m_Settings.historyActive = globalSetting.historyActive;
		auto& [w, h] = APP_WINDOWSIZE;
		RDGBufferHandle exposureData = builder.GetBuffer("ExposureData");
		RDGTextureHandle rayTexture = builder.CreateTexture("PathTracing")
			.Exetent({ w, h ,1 })
			.Format(FORMAT_R32G32B32A32_SFLOAT)
			.AllowRenderTarget()
			.AllowReadWrite()
			.Finish();
		RDGTextureHandle historyTexture = builder.CreateTexture("PathTracingHistory")
			.Import(m_HistoryTexture, isFirstTick?RESOURCE_STATE_UNDEFINED: RESOURCE_STATE_UNORDERED_ACCESS)
			.Finish();
		RDGTextureHandle directResTexture = builder.CreateTexture("PathTracingdirectRes")
			.Import(m_DirectTexture, isFirstTick?RESOURCE_STATE_UNDEFINED: RESOURCE_STATE_UNDEFINED)
			.Finish();
		RDGTextureHandle indirectResTexture = builder.CreateTexture("PathTracingIndirectRes")
			.Import(m_InDirectTexture, isFirstTick?RESOURCE_STATE_UNDEFINED: RESOURCE_STATE_UNDEFINED)
			.Finish();
		RDGTextureHandle skyBox = builder.GetTexture("CubeMap");
		if(isFirstTick) isFirstTick = false;
		builder.CreateRayTracingPass(GetName())
			.RootSignature(m_RootSignature)
			.ReadWrite(1, 0, 0, rayTexture)
			.ReadWrite(1, 1, 0, historyTexture)
			.Read(1, 2, 0, skyBox, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR ,0,1,0,6 })
			.ReadWrite(1, 3, 0, exposureData)
			.ReadWrite(1, 4, 0, directResTexture)
			.ReadWrite(1, 5, 0, indirectResTexture)
			.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;
				if (APP_SCENE_CAMERA->GetIsMove() || Input::IsKeyDown(KeyCode::R)) {
					m_Settings.totalNumSamples = 0;  // 相机移动后/手动更新后重新累计
				}
				m_Settings.totalNumSamples += m_Settings.numSamples;
				RHICommandListRef command = context.command;
				command->SetRayTracingPipeline(m_Pipeline);
				command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
				command->BindDescriptorSet(context.descriptors[1], 1);
				command->PushConstants(&m_Settings, sizeof(setting), SHADER_FREQUENCY_RAY_TRACING);
				command->TraceRays(w, h, 1); 
			})
			.Finish();


	}






}