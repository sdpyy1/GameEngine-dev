#pragma once
#include "Hazel/Utils/Serializable.h"
#include <Hazel/Math/collision.h>
namespace GameEngine {

	typedef struct BoneInfo
	{
		std::string name;
		int index;
		glm::mat4 offset;

	private:
		BeginSerailize
			SerailizeEntry(name)
			SerailizeEntry(index)
			SerailizeEntry(offset)
		EndSerailize

	} BoneInfo;


	class Mesh {
	public:
		Mesh() = default;
		Mesh(const Mesh& mesh, const std::vector<uint32_t>& subMeshIndex = {});
		void Merge(const Mesh& other, const std::vector<uint32_t> & = {});

		~Mesh() {};
		inline bool HasPosition() { return position.size(); }
		inline bool HasNormal() { return normal.size(); }
		inline bool HasTangent() { return tangent.size(); }
		inline bool HasTexCoord() { return texCoord.size(); }
		inline bool HasColor() { return color.size(); }
		inline bool HasBoneIndex() { return boneIndex.size(); }
		inline bool HasBoneWeight() { return boneWeight.size(); }
		std::string name;
		AxisAlignedBox aabb;
		BoundingSphere sphere;
		BoundingBox box;
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec4> tangent;
		std::vector<glm::vec2> texCoord;
		std::vector<glm::vec3> color;
		std::vector<glm::ivec4> boneIndex;
		std::vector<glm::vec4> boneWeight;
		std::vector<uint32_t> index;
		std::vector<BoneInfo> bone;
		inline uint32_t TriangleNum() { return index.size() / 3; }


	private:
		BeginSerailize
			SerailizeEntry(name)
			SerailizeEntry(aabb)
			SerailizeEntry(sphere)
			SerailizeEntry(box)
			SerailizeEntry(position)
			SerailizeEntry(normal)
			SerailizeEntry(tangent)
			SerailizeEntry(texCoord)
			SerailizeEntry(color)
			SerailizeEntry(boneIndex)
			SerailizeEntry(boneWeight)
			SerailizeEntry(index)
			SerailizeEntry(bone)
		EndSerailize
	};


	using MeshRef = std::shared_ptr<Mesh>;

}
