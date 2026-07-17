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
    template<class Archive> void serialize(Archive& ar, glm::vec2& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y)); }
    template<class Archive> void serialize(Archive& ar, glm::vec4& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z),cereal::make_nvp("w", e.w)); }
    template<class Archive> void serialize(Archive& ar, glm::ivec4& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z),cereal::make_nvp("w", e.w)); }
	template<class Archive> void serialize(Archive& ar, glm::uvec3& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z)); }
	template<class Archive> void serialize(Archive& ar, glm::ivec3& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z)); }
	template<class Archive> void serialize(Archive& ar, glm::quat& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z), cereal::make_nvp("w", e.w)); }
	template<class Archive> void serialize(Archive& ar, std::filesystem::path& e) { ar(cereal::make_nvp("path", e.string())); }
    template<class Archive>
    void serialize(Archive& ar, glm::mat4& m)
    {
        ar(
            cereal::make_nvp("c0", m[0]),
            cereal::make_nvp("c1", m[1]),
            cereal::make_nvp("c2", m[2]),
            cereal::make_nvp("c3", m[3])
        );
    }

#define BeginSerailize               	\
friend class cereal::access;            	\
template<class Archive>                 	\
void serialize(Archive& ar)             	\
{
#define SerializeBaseClass(className) \
try { \
    ar(cereal::make_nvp(#className, cereal::base_class<className>(this))); \
} catch (const std::exception& e) { \
    LOG_WARN("{}: failed to serialize base class '{}', reason: {}", __FUNCTION__, #className, e.what()); \
}


#define SerailizeEntry(entry) \
try { \
    ar(cereal::make_nvp(#entry, entry)); \
} catch (const std::exception& e) { \
    LOG_WARN("{}: failed to serialize entry '{}', reason: {}", __FUNCTION__, #entry, e.what()); \
}


#define SerailizeComponent(COMPONENT_TYPE) \
if (HasComponent<COMPONENT_TYPE>()) { \
    ar(cereal::make_nvp("Has" #COMPONENT_TYPE, true)); \
    ar(cereal::make_nvp(#COMPONENT_TYPE, GetComponentConst<COMPONENT_TYPE>())); \
} else { \
    ar(cereal::make_nvp("Has" #COMPONENT_TYPE, false)); \
}

#define DeserializeComponent(COMPONENT_TYPE) \
do { \
    bool hasComponent_##COMPONENT_TYPE = false; \
    try { \
        ar(cereal::make_nvp("Has" #COMPONENT_TYPE, hasComponent_##COMPONENT_TYPE)); \
    } catch (const cereal::Exception&) { \
        hasComponent_##COMPONENT_TYPE = false; \
    } \
    \
    if (hasComponent_##COMPONENT_TYPE) { \
        AddComponent<COMPONENT_TYPE>(); \
        try { \
            ar(cereal::make_nvp(#COMPONENT_TYPE, GetComponent<COMPONENT_TYPE>())); \
        } catch (const cereal::Exception&) { \
            LOG_INFO("Old file missing data for {} component, using default values", #COMPONENT_TYPE); \
        } \
    } \
} while(0)

#define BeginIfSave    if constexpr (Archive::is_saving::value) {
#define EndIfSave }

#define BeginIfLoad    if constexpr (Archive::is_loading::value) {
#define EndIfLoad }

#define SerailizeAssetEntry(entry)          \
ar(cereal::make_nvp(#entry, entry));		\
if(entry) entry->OnLoadAsset();


#define EndSerailize }
	}


#define SerailizeAssetParent         ar(cereal::base_class<Asset>(this));