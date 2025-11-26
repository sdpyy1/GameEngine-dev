#pragma once
#include "string"
#include "Hazel/Renderer/RDG/RDGBuilder.h"
namespace GameEngine {


	// °´Ë³Ğò
	enum PassType
	{
		IBL_PASS,
		GBUFFER_PASS,
		GRID_PASS,
		IMGUI_PASS,
		PRESENT_PASS,
		PASS_TYPE_MAX_CNT,	//
	};

	enum MeshPassType
	{
		
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