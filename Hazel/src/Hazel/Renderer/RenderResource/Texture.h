#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
namespace GameEngine {
	namespace V2 {

		enum TextureType {
			TEXTURE_TYPE_2D,
			TEXTURE_TYPE_2D_ARRAY,
			TEXTURE_TYPE_CUBE,
			TEXTURE_TYPE_3D
		};

		struct TextureSpec {
			std::string path;
            TextureType type = TEXTURE_TYPE_2D;
			Extent3D extent = {1,1,0};
            RHIFormat format = FORMAT_R8G8B8A8_SRGB;   // ×Ô¶¯Ù¤Âí
			bool srgb = true;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            bool generateMipmap = false;
			uint32_t textureID = 0;
			RHITextureRef texture;
			RHITextureViewRef textureView;
			uint32_t bindlessId;
			bool yFlip = false;
		};
		class Texture {
		public:
			Texture(TextureSpec& spec);
			void LoadFromFile();
			RHIDescriptorSetRef GetImGuiID();
			uint32_t GetWidth() { return m_Spec.extent.width; }
            uint32_t GetHeight() { return m_Spec.extent.height; }
			uint32_t GetbindlessID() { return m_Spec.bindlessId; }
			static RHITextureViewRef CreateView(RHITextureRef texture);
			static RHIDescriptorSetRef GetImGuiID(RHITextureRef texture);
			RHITextureRef GetRHITexture() { return m_Spec.texture; }
		private:
			TextureSpec m_Spec;
		};
		typedef std::shared_ptr<Texture> TextureRef;
		
	}

}

