#pragma once-
#include "Hazel/Renderer/RHI/RHI.h"
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
		bool generateMipmap = true;   // 妈的找了半天BUG，发现不生成Mipmap效果才差的
		bool yFlip = false;
		bool bindless = true;

		// output
		Extent3D extent = { 1,1,1 };
		RHIFormat format = FORMAT_UKNOWN;  // 从文件中解析
		uint32_t textureID = 0;
		RHITextureRef texture;
		RHITextureViewRef textureView;
		uint32_t bindlessId;
	};
	class Texture {
	public:
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
	};
	typedef std::shared_ptr<Texture> TextureRef;
}
