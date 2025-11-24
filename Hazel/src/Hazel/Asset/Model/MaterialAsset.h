#pragma once
#include "Hazel/Asset/Asset.h"
#include <Hazel/Asset/Model/Material.h>

namespace GameEngine {
	class MeshSource;
	class MaterialAsset : public Asset
	{
	public:
		explicit MaterialAsset(bool transparent = false);
		explicit MaterialAsset(Ref<MaterialOld> material);
		void SetDefaults();
		void SetNormalMap(AssetHandle handle);
		void SetEmissiveMap(AssetHandle handle);
		void SetUseNormalMap(bool value);
		void SetAlbedoMap(AssetHandle handle);
		void SetRoughnessMap(AssetHandle handle);
		void SetMetalnessMap(AssetHandle handle);
		void SetAlbedoColor(const glm::vec3& color);
		void SetEmission(const glm::vec3& value);
		void SetMetalness(float value);
		void SetRoughness(float value);
		void ClearAlbedoMap();
		void ClearNormalMap();
		void ClearEmssiveMap();
		void ClearMetalnessMap();
		void ClearRoughnessMap();
		Ref<MaterialOld> GetMaterial() { return m_Material; }
	private:
		Ref<MaterialOld> m_Material;

		struct MapAssets
		{
			AssetHandle AlbedoMap = 0;
			AssetHandle NormalMap = 0;
			AssetHandle MetalnessMap = 0;
			AssetHandle RoughnessMap = 0;
			AssetHandle EmissiveMap = 0;
		} m_Maps;

		bool m_Transparent = false;
	};

	class MaterialTable : public RefCounted
	{
	
	};

}
