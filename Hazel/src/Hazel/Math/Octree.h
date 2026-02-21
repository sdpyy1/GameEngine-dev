#pragma once
#include "collision.h"

namespace GameEngine {

#define MAX_LEAF_OBJECT_COUNT 4

	
	template<class T>
	class OctreeNode {
	public:
		OctreeNode(BoundingBox& Box) {
			AABB = Box;
		}
		std::vector<T> Objects;
		BoundingBox AABB;
		std::array<OctreeNode<T>*, 8> children{nullptr};


		bool IsStrictContain(T& Object) {
			return AABB.IsContains(Object.box);
		}
	};

	template<class T>
	class Octree
	{
	public:
		Octree(std::vector<T>& Objects, BoundingBox& Box) {
			Root = std::make_shared<OctreeNode<T>>(Box);
			CreateChildNode(Root);
			GenOctree(Root, Objects);
		}

		void CreateChildNode(std::shared_ptr<OctreeNode<T>> Root)
		{
			// TODO: 八个子空间生成，规划每个子空间的范围
			// Root->children[0] = new OctreeNode({ xxxx});
		}

		void GenOctree(std::shared_ptr<OctreeNode<T>> Root,std::vector<T> &Objects) {
			// 1. 每个物体划分到当前Root的8个子空间或Root空间中
			for (T& obj : Objects) {
				bool isPushed = false;
				for (OctreeNode<T>* CurSpace : Root->children) {
					if(CurSpace->AABB.IsContains(obj.aabb))
					{
						CurSpace->Objects.push_back(obj);
						isPushed = true;
					}
				}
				if (!isPushed)	// 没有一个子空间能够完全包住物体，放在父空间中
				{
					Root->Objects.push_back(obj);
				}
				isPushed = false;
			}

			// 2. 判断子空间是否需要继续划分
			for (OctreeNode<T>* CurSpace : Root->children)
			{
				if (CurSpace->Objects.size() > MAX_LEAF_OBJECT_COUNT)
				{
					CreateChildNode(CurSpace);
					GenOctree(CurSpace,CurSpace->Objects);
					CurSpace->Objects.clear();
				}
			}
			
		}
	
	private:
		std::shared_ptr<OctreeNode<T>> Root = nullptr;
	};
}

