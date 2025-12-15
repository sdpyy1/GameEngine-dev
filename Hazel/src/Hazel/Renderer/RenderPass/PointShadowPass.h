#pragma once
#include "MeshPass.h"

namespace GameEngine {
	class PointShadowPass : public MeshPass
	{
	public:
		PointShadowPass() = default;
		~PointShadowPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "PointShadowPass"; }
		virtual PassType GetType() override final { return POINT_SHADOW_PASS; }
	private:
		RHIShaderRef m_VertShader;
		RHIShaderRef m_GeomShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;


		uint32_t PointShadowResolution = 512;



		friend class PointShadowPassProcessor;
	};


	class PointShadowPassProcessor : public MeshPassProcessor
	{
	public:
		PointShadowPassProcessor(PointShadowPass* pass) { this->pass = pass; }

		virtual void AddMeshBatch(const MeshBatch& batch) override final;
		virtual RHIGraphicsPipelineRef OnCreatePipeline(const DrawPipelineState& pipelineState) override final;

	private:
		PointShadowPass* pass;
	};
}


