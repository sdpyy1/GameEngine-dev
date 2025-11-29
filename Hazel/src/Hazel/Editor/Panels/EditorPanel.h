#pragma once
#include <Hazel/Core/Events/Event.h>
#include "Hazel/Renderer/RenderResource/Texture.h"
namespace GameEngine {
	struct IconData {
		TextureRef icon;
		RHIDescriptorSetRef textureID;
		void LoadIconData(const std::string& path,bool isYFlip = true) {
			V2::TextureSpec spec;
			spec.path = path;
			spec.yFlip = isYFlip;
			spec.srgb = false;  // 目前看图标都不是SRGB空间的
			icon = std::make_shared<V2::Texture>(spec);
			textureID = icon->GetImGuiID();
		}
	};
	class EditorPanel
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnImGuiRender() = 0;
		virtual void OnEvent(Event& e) {}
		bool isOpen = true;
	};

}
