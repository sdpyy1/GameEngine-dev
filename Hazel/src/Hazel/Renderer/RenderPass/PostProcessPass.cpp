#include "hzpch.h"
#include "PostProcessPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
// ÆØ¹â¡¢ ToneMapping ¡¢ ColorGrading
namespace GameEngine {


	void PostProcessPass::Init()
	{
		{
			m_VertShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "FinalColorVert.spv", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
			m_FragShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "FinalColorFrag.spv", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
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
			pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;  // Depth?
			m_Pipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo); 
		}
	}

	void PostProcessPass::Build(RDGBuilder& builder)
	{

	}
}
