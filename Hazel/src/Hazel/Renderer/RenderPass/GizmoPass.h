#pragma once
#include "RenderPass.h"
#include "Hazel/Asset/Model.h"
namespace GameEngine {
	class GizmoPass : public RenderPassNew
	{
	public:
		GizmoPass() = default;
		~GizmoPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "GizmoPass"; }
		virtual PassType GetType() override final { return GIZMO_PASS; }
	private:
		RHIShaderRef m_GizmoInitShader;
		RHIShaderRef m_BillboardFragShader;
		RHIShaderRef m_BillboardVertShader;
		RHIShaderRef m_BillboardGeomShader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_GizmoInitPipeline;
		RHIGraphicsPipelineRef m_BillboardPipeline;

		ModelRef cubeWire;
		RHIShaderRef m_BoxVertShader;
		RHIShaderRef m_BoxFragShader;
		RHIGraphicsPipelineRef m_BoxPipeline;

		ModelRef sphereWire;
		RHIShaderRef m_SphereVertShader;
		RHIShaderRef m_SphereFragShader;
		RHIGraphicsPipelineRef m_SpherePipeline;

		RHIShaderRef m_LineVertShader;
		RHIShaderRef m_LineFragShader;
		RHIShaderRef m_LineGeomShader;
		RHIGraphicsPipelineRef m_LinePipeline;


		RHIIndexedIndirectCommand command[4];



	};
}
