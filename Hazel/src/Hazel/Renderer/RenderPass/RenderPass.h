#pragma once
#include "string"
#include "Hazel/Renderer/RDG/RDGBuilder.h"
namespace GameEngine {


	// °´Ë³Ğò
	enum PassType
	{
		IBL_PASS,
		DIR_SHADOW_PASS,
		PREDEPTH_PASS,
		GBUFFER_PASS,
		LIGHT_PASS,
		SKY_PASS,
		GRID_PASS,
		BLOOM_PASS,
		POST_PROCESS_PASS,
		IMGUI_PASS,
		PRESENT_PASS,
		PASS_TYPE_MAX_CNT,	//
	};

	enum MeshPassType
	{
		MESH_PASS_DIRSHADOW_PASS,
		MESH_PASS_PREDEPTH_PASS,
		MESH_PASS_GBUFFER_PASS,
		MESH_PASS_TYPE_MAX_CNT,	//
	};
	class RenderPassNew {
	public:
		RenderPassNew() = default;
		~RenderPassNew() {};

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