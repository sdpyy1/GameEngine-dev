#include "hzpch.h"
#include "Texture.h"
#include "Hazel/Utils/FileSystem.h"
#include "Hazel/Core/Application.h"
#include <stb_image.h>
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"

namespace GameEngine::V2
{
	Texture::Texture(TextureSpec& spec) : m_Spec(spec)
	{
		LoadFromFile();
	}		
	BindlessSlot TextureTypeToBindlessSlot(TextureType type)
	{
		BindlessSlot slot;
		switch (type) {
		case TEXTURE_TYPE_2D:           slot = BINDLESS_SLOT_TEXTURE_2D;        break;
		case TEXTURE_TYPE_2D_ARRAY:     slot = BINDLESS_SLOT_TEXTURE_2D_ARRAY;  break;
		case TEXTURE_TYPE_CUBE:         slot = BINDLESS_SLOT_TEXTURE_CUBE;      break;
		case TEXTURE_TYPE_3D:           slot = BINDLESS_SLOT_TEXTURE_3D;        break;
		default:                        LOG_ERROR("Unsupported texture type!");
		}

		return slot;
	}
	TextureViewType TextureTypeToViewType(TextureType type)
	{
		TextureViewType viewType;
		switch (type) {
		case TEXTURE_TYPE_2D:           viewType = VIEW_TYPE_2D;        break;
		case TEXTURE_TYPE_2D_ARRAY:     viewType = VIEW_TYPE_2D_ARRAY;  break;
		case TEXTURE_TYPE_CUBE:         viewType = VIEW_TYPE_CUBE;      break;
		case TEXTURE_TYPE_3D:           viewType = VIEW_TYPE_3D;        break;
		default:                        LOG_ERROR("Unsupported texture type!");
		}

		return viewType;
	}
	void Texture::LoadFromFile()
	{
		if (m_Spec.type == TEXTURE_TYPE_3D) {
			LOG_ERROR("Texture type 3D not supported yet");
		}
		if (m_Spec.type == TEXTURE_TYPE_CUBE && m_Spec.arrayLayers != 6){
            LOG_ERROR("TEXTURE_TYPE_CUBE need arrayLayers = 6");
		}

		// 文件读取
		std::vector<uint8_t> data;
		FileSystem::LoadBinary(m_Spec.path, data);
		int width, height, channels;
		if (m_Spec.yFlip) {
			stbi_set_flip_vertically_on_load(true);
		}
		stbi_info_from_memory(data.data(), data.size(), &width, &height, &channels);
		stbi_uc* pixels = stbi_load_from_memory(data.data(), data.size(), &width, &height, &channels, 4);
		stbi_set_flip_vertically_on_load(false);

		uint32_t bufferSize = width * height * sizeof(uint32_t);
		m_Spec.extent = { (uint32_t)width,(uint32_t)height,1 };
		if (m_Spec.generateMipmap) {
			m_Spec.mipLevels = (uint32_t)(std::floor(std::log2(std::max(width, height)))) + 1;
		}
		ResourceType resourceType = (m_Spec.type == TEXTURE_TYPE_CUBE) ? (RESOURCE_TYPE_TEXTURE_CUBE | RESOURCE_TYPE_TEXTURE) : RESOURCE_TYPE_TEXTURE;
		// RHI
		{
			ResourceType resourceType = (m_Spec.type == TEXTURE_TYPE_CUBE) ? (RESOURCE_TYPE_TEXTURE_CUBE | RESOURCE_TYPE_TEXTURE) : RESOURCE_TYPE_TEXTURE;
			if (IsRWFormat(m_Spec.format))      resourceType |= RESOURCE_TYPE_RW_TEXTURE;
			TextureAspectFlags aspects = IsDepthStencilFormat(m_Spec.format) ? TEXTURE_ASPECT_DEPTH_STENCIL :
				IsDepthFormat(m_Spec.format) ? TEXTURE_ASPECT_DEPTH :
				IsStencilFormat(m_Spec.format) ? TEXTURE_ASPECT_STENCIL : TEXTURE_ASPECT_COLOR;

			RHITextureInfo rhiTextureInfo;
            rhiTextureInfo.type = resourceType;
            rhiTextureInfo.extent = m_Spec.extent;
            rhiTextureInfo.format = m_Spec.format;
            rhiTextureInfo.mipLevels = m_Spec.mipLevels;
			rhiTextureInfo.arrayLayers = m_Spec.arrayLayers;
			rhiTextureInfo.creationFlag = TEXTURE_CREATION_NONE;
			m_Spec.texture = APP_DYNAMICRHI->CreateTexture(rhiTextureInfo);

			RHITextureViewInfo rhiTextureViewInfo;
			rhiTextureViewInfo.texture = m_Spec.texture;
			rhiTextureViewInfo.format = m_Spec.format; 
			rhiTextureViewInfo.viewType = TextureTypeToViewType(m_Spec.type);
			rhiTextureViewInfo.subresource = { aspects, 0, m_Spec.mipLevels, 0, m_Spec.arrayLayers };
			m_Spec.textureView = APP_DYNAMICRHI->CreateTextureView(rhiTextureViewInfo);
		}

		// 转移数据
		RHIBufferInfo bufferInfo = {
			bufferInfo.size = bufferSize,
			bufferInfo.memoryUsage = MEMORY_USAGE_CPU_ONLY,
			bufferInfo.type = RESOURCE_TYPE_BUFFER,
			bufferInfo.creationFlag = BUFFER_CREATION_PERSISTENT_MAP
		};
		RHIBufferRef stagingBuffer = APP_DYNAMICRHI->CreateBuffer(bufferInfo);
		memcpy(stagingBuffer->Map(), pixels, bufferSize);
		APP_DYNAMICRHI->GetImmediateCommandList(true)->TextureBarrier(
			{ m_Spec.texture,
			RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_TRANSFER_DST,
			{TEXTURE_ASPECT_COLOR, 0, m_Spec.mipLevels, 0, 1} });
		APP_DYNAMICRHI->GetImmediateCommandList()->CopyBufferToTexture(stagingBuffer, 0, m_Spec.texture, { TEXTURE_ASPECT_COLOR, 0, 0, 1 });
		stbi_image_free(pixels);


		// mipmap
		if (m_Spec.generateMipmap)
        { 
			APP_DYNAMICRHI->GetImmediateCommandList()->TextureBarrier({ m_Spec.texture,
	RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_TRANSFER_SRC,
			{TEXTURE_ASPECT_COLOR, 0, m_Spec.mipLevels, 0, m_Spec.arrayLayers} });
			APP_DYNAMICRHI->GetImmediateCommandList()->GenerateMips(m_Spec.texture);
			APP_DYNAMICRHI->GetImmediateCommandList()->TextureBarrier({ m_Spec.texture,
				RESOURCE_STATE_TRANSFER_SRC, RESOURCE_STATE_SHADER_RESOURCE,
						{TEXTURE_ASPECT_COLOR, 0, m_Spec.mipLevels, 0, m_Spec.arrayLayers} });
		}
		else { // 也需要转到Shader读取布局
			APP_DYNAMICRHI->GetImmediateCommandList(true)->TextureBarrier(
				{ m_Spec.texture,
				RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_SHADER_RESOURCE,
				{TEXTURE_ASPECT_COLOR, 0, m_Spec.mipLevels, 0, 1} });
		}

		APP_DYNAMICRHI->GetImmediateCommandList()->Flush();

		// bindless
		BindlessResourceInfo bindlessResourceInfo;
        bindlessResourceInfo.textureView = m_Spec.textureView;
        bindlessResourceInfo.resourceType = RESOURCE_TYPE_TEXTURE;
		m_Spec.bindlessId = RENDER_RESOURCEMANAGER->AllocateBindlessID(bindlessResourceInfo, TextureTypeToBindlessSlot(m_Spec.type));		
	}

	RHIDescriptorSetRef Texture::GetImGuiID()
	{
		return APP_DYNAMICRHI->GetImGuiTextId(m_Spec.textureView);
	}

	RHIDescriptorSetRef Texture::GetImGuiID(RHITextureRef texture)
	{
		return APP_DYNAMICRHI->GetImGuiTextId(CreateView(texture));
	}

	RHITextureViewRef Texture::CreateView(RHITextureRef texture)   // TODO:目前只有Viewport使用
	{
		RHITextureViewInfo rhiTextureViewInfo;
		rhiTextureViewInfo.texture = texture;
		rhiTextureViewInfo.format = FORMAT_R8G8B8A8_UNORM;
		rhiTextureViewInfo.viewType = VIEW_TYPE_2D;
		rhiTextureViewInfo.subresource = { TEXTURE_ASPECT_COLOR, 0, 1, 0, 1 };
		return APP_DYNAMICRHI->CreateTextureView(rhiTextureViewInfo);
	}

}
