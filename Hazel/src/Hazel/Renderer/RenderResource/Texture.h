#pragma once-
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Utils/Serializable.h"
namespace GameEngine {
	enum TextureType {
		TEXTURE_TYPE_2D,
		TEXTURE_TYPE_2D_ARRAY,
		TEXTURE_TYPE_CUBE,
		TEXTURE_TYPE_3D
	};

	struct TextureSpec {
		std::string path;
		TextureType type = TEXTURE_TYPE_2D;
		bool srgb = true;   // 需要手动指定传入的图片是不是SRGB
		uint32_t mipLevels = 1;
		uint32_t arrayLayers = 1;
		bool generateMipmap = true;  
		bool yFlip = false;
		bool bindless = true;

		// output
		Extent3D extent = { 1,1,1 };
		RHIFormat format = FORMAT_UKNOWN;  // 从文件中解析
		uint32_t textureID = 0;
		RHITextureRef texture;
		RHITextureViewRef textureView;
		uint32_t bindlessId;

		BeginSerailize
            SerailizeEntry(path)
            SerailizeEntry(type)
            SerailizeEntry(srgb)
            SerailizeEntry(mipLevels)
            SerailizeEntry(arrayLayers)
            SerailizeEntry(generateMipmap)
            SerailizeEntry(yFlip)
            SerailizeEntry(bindless)
		EndSerailize
	};
	class Texture : public std::enable_shared_from_this<Texture> {
	public:
		Texture() = default;
		Texture(TextureSpec& spec);   // 从这里创建的Textue，出去的布局是RESOURCE_STATE_SHADER_RESOURCE
		void LoadFromFile();
		void CreateRHITexture();
		RHIDescriptorSetRef GetImGuiID();
		uint32_t GetWidth() { return m_Spec.extent.width; }
		uint32_t GetHeight() { return m_Spec.extent.height; }
		uint32_t GetbindlessID() { return m_Spec.bindlessId; }
		static RHITextureViewRef CreateView(RHITextureRef texture);
		static RHIDescriptorSetRef GetImGuiID(RHITextureRef texture);
		RHITextureRef GetRHITexture() { return m_Spec.texture; }
		RHITextureViewRef GetRHITextureView() { return m_Spec.textureView; }
	private:
		TextureSpec m_Spec;
		RHIDescriptorSetRef m_ImGuiIDCache;

		static std::map<std::string, std::shared_ptr<Texture>> textureCache;
		BeginSerailize
			SerailizeEntry(m_Spec)
			if (m_Spec.textureID == 0) {
				if (textureCache.find(m_Spec.path) != textureCache.end()) {
					m_Spec = textureCache[m_Spec.path]->m_Spec;
				}
				else {
					LoadFromFile();
                    textureCache[m_Spec.path] = shared_from_this();
				}
			}
			EndSerailize
	};
	typedef std::shared_ptr<Texture> TextureRef;
}
