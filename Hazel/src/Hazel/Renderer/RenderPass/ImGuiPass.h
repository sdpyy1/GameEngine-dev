#pragma once
#include "RenderPass.h"
#include "Hazel/Renderer/RenderResource/Texture.h"
#include <Hazel/Core/Definations.h>

namespace GameEngine {
	class PanelManager;
	class ImGuiPass :public RenderPassNew
	{
	public:
		ImGuiPass() = default;
		~ImGuiPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "ImGuiPass"; }
		virtual PassType GetType() override final { return IMGUI_PASS; }
	private:
		std::shared_ptr<PanelManager> m_PanelManager;
		TextureRef viewportTexture;
		RHIDescriptorSetRef viewportID[FRAMES_IN_FLIGHT];

	};

}