#include "hzpch.h"
#include "Hazel/Asset/Model/Material.h"
#include "Hazel/Platform/Vulkan/VulkanMaterial.h"

#include "Hazel/Renderer/old/RendererAPI.h"

namespace GameEngine {
	Ref<MaterialOld> MaterialOld::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	MaterialPush MaterialOld::BuildPush()
	{
		MaterialPush push;
		push.AlbedoColor = m_AlbedoColor;
		push.Emission = m_EmissionColor;
        push.Metalness = m_MetalnessColor;
        push.Roughness = m_RoughnessColor;
		push.UseNormalMap = bUseNormalTexture ? 1u : 0u;
		return push;
	}

}
