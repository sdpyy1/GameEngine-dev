#pragma once
#include "RenderPass.h"
#include "Hazel/Renderer/RenderResource/RenderBuffer.h"
namespace GameEngine {
	class ExposurePass : public RenderPass
	{
	public:
		ExposurePass() = default;
		~ExposurePass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "ExposurePass"; }
		virtual PassType GetType() override final { return EXPOSURE_PASS; }
	private:
        bool enableStatistics = false;

        float minLuminance = 1.0f / 128.0f;
        float maxLuminance = 32.0f;
        float adjustSpeed = 0.05f;

        struct ExposureSetting
        {
            float minLog2Luminance = 0.0f;
            float inverseLuminanceRange = 0.0f;
            float luminanceRange = 0.0f;
            float numPixels = 0.0f;
            float timeCoeff = 0.0f;
            float _padding[3];
        };
        struct ExposureData
        {
            ExposureSetting setting = {};
            float luminance = 0.0f;
            float adaptedLuminance = 0.0f;
            float _padding[2];
            uint32_t histogramBuffer[256] = { 0 };
            uint32_t readBackHistogramBuffer[256] = { 0 };
        };
        RenderBuffer<ExposureData> exposureDataBuffer;
		RHIShaderRef m_HistogramShader;
		RHIShaderRef m_ExposureShader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_HistogramPipeline;
		RHIComputePipelineRef m_ExposurePipeline;
	};
}

