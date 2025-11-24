#include "hzpch.h"
#include "MaterialAsset.h"
#include "Hazel/Renderer/old/Renderer.h"
#include "Hazel/Asset/AssetManager.h"

namespace GameEngine{
	MaterialAsset::MaterialAsset(bool transparent)
		: m_Transparent(transparent)
	{
		Handle = {};

		if (transparent)
			m_Material = MaterialOld::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent"));
		else
			m_Material = MaterialOld::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static"));

		SetDefaults();
	}

	MaterialAsset::MaterialAsset(Ref<MaterialOld> material)
	{
		Handle = {};
		m_Material = material;
	}

	void MaterialAsset::SetDefaults()
	{
		if (m_Transparent)
		{
			SetAlbedoColor(glm::vec3(0.8f));
			ClearAlbedoMap();
		}
		else
		{
			SetAlbedoColor(glm::vec3(0.8f));
			SetEmission({0,0,0});
			SetUseNormalMap(false);
			SetMetalness(0.0f);
			SetRoughness(0.4f);
			ClearAlbedoMap();
			ClearNormalMap();
			ClearMetalnessMap();
			ClearRoughnessMap();
		}
	}
	void MaterialAsset::SetNormalMap(AssetHandle handle)
	{
		m_Maps.NormalMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->SetNormalTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearNormalMap();
		}
	}
	void MaterialAsset::SetEmissiveMap(AssetHandle handle)
	{
		m_Maps.EmissiveMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->SetEmissionTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearEmssiveMap();
		}
	}
	void MaterialAsset::SetUseNormalMap(bool value)
	{
		m_Material->SetUseNormalTexture(value);
	}
	void MaterialAsset::SetAlbedoMap(AssetHandle handle)
	{
		m_Maps.AlbedoMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->SetAlbedoTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearAlbedoMap();
		}
	}
	void MaterialAsset::SetRoughnessMap(AssetHandle handle)
	{
		m_Maps.RoughnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->SetRoughnessTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearRoughnessMap();
		}
	}
	void MaterialAsset::SetMetalnessMap(AssetHandle handle)
	{
		m_Maps.MetalnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->SetMetalnessTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearMetalnessMap();
		}
	}
	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		m_Material->SetAlbedoColor(color);
	}
	void MaterialAsset::SetMetalness(float value)
	{
		m_Material->SetMetalnessColor(value);
	}
	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->SetRoughnessColor(value);
	}
	void MaterialAsset::SetEmission(const glm::vec3& value)
	{
		m_Material->SetEmissionColor(value);

	}
	void MaterialAsset::ClearAlbedoMap()
	{
		m_Material->SetAlbedoTexture(Renderer::GetWhiteTexture());
	}
	void MaterialAsset::ClearNormalMap()
	{
		m_Material->SetNormalTexture(Renderer::GetWhiteTexture());
	}
	void MaterialAsset::ClearEmssiveMap()
	{
		m_Material->SetEmissionTexture(Renderer::GetWhiteTexture()); // TODO: should be black texture
	}
	void MaterialAsset::ClearMetalnessMap()
	{
		m_Material->SetMetalnessTexture(Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearRoughnessMap()
	{
		m_Material->SetRoughnessTexture(Renderer::GetWhiteTexture());
	}
}

