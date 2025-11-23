#pragma once
#include <Hazel/Core/UUID.h>
#include "Hazel/Asset/AssetTypes.h"
#include "Hazel/Utils/Serializable.h"

namespace GameEngine {

	namespace V2 {
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
			inline const UUID& GetUID() { return uid; }
		protected:
			UUID uid = {};


		private:
			BeginSerailize()
			SerailizeEntry(uid)
			EndSerailize
		};
	}









































	using AssetHandle = UUID;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle = 0;
		uint16_t Flags = (uint16_t)AssetFlag::None; 

		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		virtual void OnDependencyUpdated(AssetHandle handle) {}

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

	private:
		friend class AssimpMeshImporter;
		friend class AssetManager;
		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
	};
}
