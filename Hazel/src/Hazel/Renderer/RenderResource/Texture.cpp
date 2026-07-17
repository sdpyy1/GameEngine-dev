#include "hzpch.h"
#include "Texture.h"
#include "Hazel/Utils/FileSystem.h"
#include "Hazel/Core/Application.h"
#include <stb_image.h>
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RDG/RDGPool.h"
namespace GameEngine
{
	std::map<std::string, std::shared_ptr<Texture>> Texture::textureCache; // only for Serailize

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

	Texture::Texture(TextureSpec& spec) : m_Spec(spec)
	{
		if (spec.path != "") {
			LoadFromFile();
		}
		else {
			if (m_Spec.extent == Extent3D{ 1, 1, 1 }) {
				LOG_WARN("Create RHI Texture Extent is 1,1,1");
			}
			if (m_Spec.format == FORMAT_UKNOWN) {
				LOG_ERROR("Texture format is unknown!");
			}
			CreateRHITexture();
		}
	}

	void Texture::CreateRHITexture()
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

	bool isDepthFormalt(RHIFormat format) {
		return format == RHIFormat::FORMAT_D32_SFLOAT ||
			format == RHIFormat::FORMAT_D32_SFLOAT_S8_UINT ||
			format == RHIFormat::FORMAT_D24_UNORM_S8_UINT;
	}
	void Texture::LoadFromFile()
	{
		if (m_Spec.type == TEXTURE_TYPE_3D) {
			LOG_ERROR("Texture type 3D not supported yet");
		}
		if (m_Spec.type == TEXTURE_TYPE_CUBE && m_Spec.arrayLayers != 6) {
			LOG_ERROR("TEXTURE_TYPE_CUBE need arrayLayers = 6");
		}

		// 文件读取
		Buffer imageBuffer;
		int width, height, channels;
		if (m_Spec.yFlip) {
			stbi_set_flip_vertically_on_load(true);
		}
		if (stbi_is_hdr(m_Spec.path.c_str())) {
			m_Spec.format = RHIFormat::FORMAT_R32G32B32A32_SFLOAT;
			imageBuffer.Data = (byte*)stbi_loadf(m_Spec.path.c_str(), &width, &height, &channels, 4);
			imageBuffer.Size = width * height * 4 * sizeof(float);
		}
		else {
			imageBuffer.Data = stbi_load(m_Spec.path.c_str(), &width, &height, &channels, 4);  // TODO:不支持dds格式
			if (!imageBuffer.Data) {
				LOG_ERROR("Image Load Fail! [{}]", m_Spec.path);
			}
			imageBuffer.Size = width * height * 4;
			if (m_Spec.srgb) {
				m_Spec.format = RHIFormat::FORMAT_R8G8B8A8_SRGB;
			}
			else {
				m_Spec.format = RHIFormat::FORMAT_R8G8B8A8_UNORM;
			}
		}

		stbi_set_flip_vertically_on_load(false);

		m_Spec.extent = { (uint32_t)width,(uint32_t)height,1 };
		if (m_Spec.generateMipmap) {
			m_Spec.mipLevels = m_Spec.extent.MipSize();
		}
		ResourceType resourceType = (m_Spec.type == TEXTURE_TYPE_CUBE) ? (RESOURCE_TYPE_TEXTURE_CUBE | RESOURCE_TYPE_TEXTURE) : RESOURCE_TYPE_TEXTURE;

		CreateRHITexture();

		// 转移数据
		RHIBufferInfo bufferInfo = {
			bufferInfo.size = imageBuffer.Size,
			bufferInfo.memoryUsage = MEMORY_USAGE_CPU_ONLY,
			bufferInfo.type = RESOURCE_TYPE_BUFFER,
			bufferInfo.creationFlag = BUFFER_CREATION_PERSISTENT_MAP
		};
		RHIBufferRef stagingBuffer = APP_DYNAMICRHI->CreateBuffer(bufferInfo);
		memcpy(stagingBuffer->Map(), imageBuffer.Data, imageBuffer.Size);
		APP_DYNAMICRHI->GetImmediateCommandList()->TextureBarrier(
			{ m_Spec.texture,
			RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_TRANSFER_DST,
			{TEXTURE_ASPECT_COLOR, 0, m_Spec.mipLevels, 0, 1} });
		APP_DYNAMICRHI->GetImmediateCommandList()->CopyBufferToTexture(stagingBuffer, 0, m_Spec.texture, { TEXTURE_ASPECT_COLOR, 0, 0, 1 });
		// stbi_image_free(pixels);

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
			APP_DYNAMICRHI->GetImmediateCommandList()->TextureBarrier(
				{ m_Spec.texture,
				RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_SHADER_RESOURCE,
				{TEXTURE_ASPECT_COLOR, 0, m_Spec.mipLevels, 0, 1} });
		}

		APP_DYNAMICRHI->GetImmediateCommandList()->Flush();

		if (m_Spec.bindless) {
			// bindless
			BindlessResourceInfo bindlessResourceInfo;
			bindlessResourceInfo.textureView = m_Spec.textureView;
			bindlessResourceInfo.resourceType = RESOURCE_TYPE_TEXTURE;
			m_Spec.bindlessId = RENDER_RESOURCEMANAGER->AllocateBindlessID(bindlessResourceInfo, TextureTypeToBindlessSlot(m_Spec.type));
		}
	}

	RHIDescriptorSetRef Texture::GetImGuiID()
	{
		if (!m_ImGuiIDCache) {
			m_ImGuiIDCache = APP_DYNAMICRHI->GetImGuiTextId(m_Spec.textureView);
		}
		return m_ImGuiIDCache;
	}

	RHIDescriptorSetRef Texture::GetImGuiID(RHITextureRef texture)
	{
		return APP_DYNAMICRHI->GetImGuiTextId(CreateView(texture));  // 每次都创建一个新的
	}

	RHITextureViewRef Texture::CreateView(RHITextureRef texture)   // TODO:目前只有Viewport使用
	{
		if (!texture) {
			LOG_ERROR("Texture is null");
		}
		RHITextureViewInfo rhiTextureViewInfo;
		rhiTextureViewInfo.texture = texture;
		rhiTextureViewInfo.format = texture->GetInfo().format;
		rhiTextureViewInfo.viewType = texture->GetInfo().arrayLayers == 1?VIEW_TYPE_2D: VIEW_TYPE_2D_ARRAY;
		rhiTextureViewInfo.subresource = { isDepthFormalt(texture->GetInfo().format) ? TEXTURE_ASPECT_DEPTH : TEXTURE_ASPECT_COLOR, 0, 1, texture->GetInfo().arrayLayers/2, 1 };
		// return RDGTextureViewPool::Get()->Allocate(rhiTextureViewInfo).textureView;   // TODO:这种入池没有释放有没有问题
		return APP_DYNAMICRHI->CreateTextureView(rhiTextureViewInfo);
	}
}