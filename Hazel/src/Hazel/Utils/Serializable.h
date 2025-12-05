#pragma once
#include <cereal/access.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/queue.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <Hazel/Renderer/RHI/RHIBase.h>
namespace cereal {
	// 指定各种类型的序列化规则
	template<class Archive> void serialize(Archive& ar, GameEngine::Extent2D& e) { ar(cereal::make_nvp("width", e.width), cereal::make_nvp("height", e.height)); }
	template<class Archive> void serialize(Archive& ar, GameEngine::Extent3D& e) { ar(cereal::make_nvp("width", e.width), cereal::make_nvp("height", e.height), cereal::make_nvp("depth", e.depth)); }
	template<class Archive> void serialize(Archive& ar, glm::vec3& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z)); }
	template<class Archive> void serialize(Archive& ar, glm::quat& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z), cereal::make_nvp("w", e.w)); }

#define BeginSerailize               	\
friend class cereal::access;            	\
template<class Archive>                 	\
void serialize(Archive& ar)             	\
{
#define SerailizeBaseClass(className)   	\
ar(cereal::make_nvp(#className, cereal::base_class<className>(this)));

#define SerailizeEntry(entry)           	\
ar(cereal::make_nvp(#entry, entry));

#define SerializeComponent(ComponentType)                   \
    if (HasComponent<ComponentType>()) {            \
        ar(cereal::make_nvp(#ComponentType,                \
            GetComponent<ComponentType>()));        \
    }

#define SerailizeAssetEntry(entry)          \
ar(cereal::make_nvp(#entry, entry));		\
if(entry) entry->OnLoadAsset();


#define EndSerailize }
	}