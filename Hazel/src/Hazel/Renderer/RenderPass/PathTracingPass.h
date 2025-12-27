#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class PathTracingPass : public RenderPass
	{
	public:
		PathTracingPass() = default;
		~PathTracingPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "PathTracingPass"; }
		virtual PassType GetType() override final { return PATHTRACING_PASS; }
	private:
		RHIShaderRef m_RayGenShader;
		RHIShaderRef m_ClosestHitShader;
		RHIShaderRef m_MissShader;
		struct setting
		{
			int numSamples = 1;			// 每帧采样数
			int totalNumSamples = 0;	// 累计采样数
			int numBounce = 50;			// 光线反射深度
			int sampleSkyBox = 1;		// 是否采样来自天空盒的光照
			int indirectOnly = 0;		// 仅间接光照
			int historyActive = 1;		// 是否时域累积
		} m_Settings;
		RHITextureRef m_HistoryTexture;
		RHITextureRef m_DirectTexture;
		RHITextureRef m_InDirectTexture;
		RHIRootSignatureRef m_RootSignature;
		RHIRayTracingPipelineRef m_Pipeline;
		bool isFirstTick = true;
	};
}


