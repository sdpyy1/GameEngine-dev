#pragma once
#include <Hazel/Events/Event.h>
#include "Hazel/Renderer/RenderResource/Texture.h"
namespace GameEngine {
	struct IconData {
		TextureRef icon;
		RHIDescriptorSetRef textureID;
		void LoadIconData(const std::string& path,bool isYFlip = true) {
			V2::TextureSpec spec;
			spec.path = path;
			spec.yFlip = isYFlip;
			spec.format = FORMAT_R32G32B32A32_SFLOAT;
			icon = std::make_shared<V2::Texture>(spec);
			textureID = icon->GetImGuiID();
		}
	};
	class EditorPanel : public RefCounted
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnImGuiRender() = 0;
		virtual void OnEvent(Event& e) {}
		bool isOpen = true;
	};

}
