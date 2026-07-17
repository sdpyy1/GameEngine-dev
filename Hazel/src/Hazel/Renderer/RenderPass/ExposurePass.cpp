#include "hzpch.h"
#include "ExposurePass.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Renderer/RenderResource/Shader.h"
/*
	计算场景亮度的柱状图Histogram，并根据柱状图求平均亮度，计算出一个曝光值


	Luminance = Radiance × 人眼亮度感知函数
*/
namespace GameEngine {
	void ExposurePass::Init()
	{
		m_HistogramShader = std::make_shared<Shader>("postprocess/luminance_histogram", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
		m_ExposureShader = std::make_shared<Shader>("postprocess/luminance_exposure", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
        RHIRootSignatureInfo rootSignatureInfo;
		rootSignatureInfo.AddEntry({ 0, 0, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE })
			.AddEntry({ 0, 1, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_BUFFER });

		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);
		RHIComputePipelineInfo pipelineInfo = {};
		pipelineInfo.rootSignature = m_RootSignature;
		pipelineInfo.computeShader = m_HistogramShader;
		m_HistogramPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);


		pipelineInfo.computeShader = m_ExposureShader;
		m_ExposurePipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);


	}
	void ExposurePass::Build(RDGBuilder& builder)
	{
		auto& [w, h] = APP_WINDOWSIZE;
		ExposureSetting setting = {};
		setting.minLog2Luminance = log2(minLuminance);
		setting.luminanceRange = log2(maxLuminance) - log2(minLuminance);
		setting.inverseLuminanceRange = 1.0f / setting.luminanceRange;
		setting.numPixels = w * h;
		setting.timeCoeff = adjustSpeed;
		exposureDataBuffer.SetData(&setting, sizeof(ExposureSetting), 0);


		RDGTextureHandle Viewport = builder.GetTexture("ViewPort");
		RDGBufferHandle exposureData = builder.CreateBuffer("ExposureData")
			.Import(exposureDataBuffer.GetRHIBuffer(), RESOURCE_STATE_UNDEFINED)
			.Finish();
		// 统计直方图
		builder.CreateComputePass(GetName() + "_Luminance Histogram")
			.RootSignature(m_RootSignature)
			.ReadWrite(0, 0, 0, Viewport)
			.ReadWrite(0, 1, 0, exposureData)
			.Execute([&](RDGPassContext context) {
				auto& [w, h] = APP_WINDOWSIZE;

				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_HistogramPipeline);
				command->BindDescriptorSet(context.descriptors[0], 0);
				command->Dispatch(w / 16,h / 16,1);
			})
			.Finish();



		builder.CreateComputePass(GetName()+"_LuminanceExposure")
			.RootSignature(m_RootSignature)
			.ReadWrite(0, 0, 0, Viewport)
			.ReadWrite(0, 1, 0, exposureData)
			.Execute([&](RDGPassContext context) {
				RHICommandListRef command = context.command;
				command->SetComputePipeline(m_ExposurePipeline);
				command->BindDescriptorSet(context.descriptors[0], 0);
				command->Dispatch(1,1,1);
			})
			.Finish();

	}
}