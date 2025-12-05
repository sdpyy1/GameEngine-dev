#pragma once
#include <Hazel/Core/UUID.h>
#include "Hazel/Utils/Serializable.h"

namespace GameEngine {
	enum AssetType
	{
		ASSET_TYPE_UNKNOWN = 0,
		ASSET_TYPE_MODEL,
		ASSET_TYPE_TEXTURE,
		ASSET_TYPE_SHADER,
		ASSET_TYPE_MATERIAL,
		ASSET_TYPE_ANIMATION,
		ASSET_TYPE_SCENE,

		ASSET_TYPE_MAX_ENUM,    //
	};

	class Asset {
	public:
		Asset() = default;
		virtual ~Asset() = default;
		virtual std::string GetAssetTypeName() { return "Unknown"; }
		virtual AssetType GetAssetType() { return ASSET_TYPE_UNKNOWN; }
		virtual void OnLoadAsset() = 0;
		virtual void OnSaveAsset() = 0;
		inline const UUID& GetUUID() { return uid; }
	protected:
		UUID uid = {};

	private:
		BeginSerailize
			SerailizeEntry(uid)
		EndSerailize
	};
}
