#pragma once
#include "string"
#include "Hazel/Renderer/RDG/RDGBuilder.h"
namespace GameEngine {
	// °´Ë³Ğò
	enum PassType
	{
		IBL_PASS,
		GPUCULLING_PASS,
		CLUSTER_LIGHTING_PASS,
		DIR_SHADOW_PASS,
		POINT_SHADOW_PASS,
		PREDEPTH_PASS,
		GBUFFER_PASS,
		DDGI_PASS,
		LIGHT_PASS,
		SKY_PASS,
		BLOOM_PASS,
		EXPOSURE_PASS,
		PATHTRACING_PASS,
		SVGF_PASS,
		SSSR_PASS,
		TAA_PASS,
		FXAA_PASS,
		POST_PROCESS_PASS,
		GIZMO_PASS,
		GRID_PASS,
		IMGUI_PASS,
		PRESENT_PASS,
		PASS_TYPE_MAX_CNT,	//
	};

	enum MeshPassType
	{
		MESH_PASS_DIRSHADOW_PASS,
		MESH_PASS_POINTSHADOW_PASS,
		MESH_PASS_PREDEPTH_PASS,
		MESH_PASS_GBUFFER_PASS,
		MESH_PASS_TYPE_MAX_CNT,	//
	};
	class RenderPass {
	public:
		RenderPass() = default;
		~RenderPass() {};

		virtual void Init() {};
		virtual void Build(RDGBuilder& builder) {};

		virtual std::string GetName() { return "Unknown"; }

		virtual PassType GetType() = 0;

		bool IsEnabled() { return enable; }
		void SetEnable(bool enable) { this->enable = enable; }
	private:
		bool enable = true;
	};
}