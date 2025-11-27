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
            info.AddEntryFromReflect(MultiScatteringLutShader);
            MultiScatteringLutRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
            RHIComputePipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = MultiScatteringLutRootSignature;
            pipelineInfo.computeShader = MultiScatteringLutShader;
            MultiScatteringLutPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{ 
			SkyViewLutShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "SkyViewLut.comp.spv", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
            RHIRootSignatureInfo info = {};
            info.AddEntryFromReflect(SkyViewLutShader);  // TODO: 需要看一下是不是common也挂上了
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
			info.AddEntryFromReflect(m_VertShader).AddEntryFromReflect(m_FragShader);
			m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
			RHIGraphicsPipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = m_RootSignature;
            pipelineInfo.vertexShader = m_VertShader;
            pipelineInfo.fragmentShader = m_FragShader;
			pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
			pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
			pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
			pipelineInfo.colorAttachmentFormats[0] = FORMAT_R8G8B8A8_UNORM;
            pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
            m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);  // Depth Test
		}
	}

	void SkyPass::Build(RDGBuilder& builder)
	{

	}
}

