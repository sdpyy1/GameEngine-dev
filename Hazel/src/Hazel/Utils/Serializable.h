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
namespace GameEngine {

	template<class Archive> void serialize(Archive& ar, Extent2D& e) { ar(cereal::make_nvp("width", e.width), cereal::make_nvp("height", e.height)); }
	template<class Archive> void serialize(Archive& ar, Extent3D& e) { ar(cereal::make_nvp("width", e.width), cereal::make_nvp("height", e.height), cereal::make_nvp("depth", e.depth)); }
	template<class Archive> void serialize(Archive& ar, glm::vec3& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z)); }









#define BeginSerailize()                	\
friend class cereal::access;            	\
template<class Archive>                 	\
void serialize(Archive& ar)             	\
{                               

#define SerailizeBaseClass(className)   	\
ar(cereal::make_nvp(#className, cereal::base_class<className>(this)));

#define SerailizeEntry(entry)           	\
ar(cereal::make_nvp(#entry, entry));

#define SerailizeAssetEntry(entry)          \
ar(cereal::make_nvp(#entry, entry));		\
if(entry) entry->OnLoadAsset();

#define SerailizeAssetArrayEntry(entry)     \
ar(cereal::make_nvp(#entry, entry));		\
for(auto& asset : entry) { if(asset) asset->OnLoadAsset(); }

#define SerailizeFilePath(entry, path)  	\
if(entry) { path = entry->GetFilePath(); }	\
ar(cereal::make_nvp(#path, path));

#define IfSerailizeInput()					\
{											\
	cereal::JSONInputArchive* jsonPtr = dynamic_cast<cereal::JSONInputArchive*>(&ar);			\
	cereal::BinaryInputArchive* binaryPtr = dynamic_cast<cereal::BinaryInputArchive*>(&ar);		\
    if(jsonPtr || binaryPtr) { 

#define IfSerailizeOutput()					\
{											\
	cereal::JSONOutputArchive* jsonPtr = dynamic_cast<cereal::JSONOutputArchive*>(&ar);			\
	cereal::BinaryOutputArchive* binaryPtr = dynamic_cast<cereal::BinaryOutputArchive*>(&ar);	\
    if(jsonPtr || binaryPtr) { 

#define EndIfSerailize 						\
	}										\
}

#define EndSerailize }

















}