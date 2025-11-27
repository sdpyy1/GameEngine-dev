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
		m_Shader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "Bloom.comp.spv", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntryFromReflect(m_Shader);
		info.AddPushConstant({ sizeof(bloomSetting),SHADER_FREQUENCY_COMPUTE });
        m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIComputePipelineInfo pipelineInfo = {};
        pipelineInfo.computeShader = m_Shader;
        pipelineInfo.rootSignature = m_RootSignature;
        m_Pipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
	}

	void BloomPass::Build(RDGBuilder& builder)
	{

	}





}