#pragma once
#include "collision.h"
/*
	八叉树(目前这个代码跑不了，只是伪代码)：
	构建: 把所有物体分配给Root，如果Root的物体数量大于MAX_LEAF_OBJECT_COUNT，就创建它的8个孩子，并且把所有物体按照位置分配给孩子（如果没法精确分配给某个孩子，就还是存在Root上），再遍历8个孩子，如果某个孩子物体数量大于MAX_LEAF_OBJECT_COUNT，继续递归构建
	添加：判断物体是否包含在Root，包含的话，就判断8个孩子是否完美包含它，如果包含，就递归给孩子去添加，如果都不包含。就存储在当前Root
	更新物体位置：判断是否还在同一个结点，如果不在就删除后重写插入，或者有一些论文去查询邻近结点

	松散八叉树：


	八叉树的用途：（加速遍历Mesh的判断，如果对某个结点的判断失败了，那这个结点以下的Mesh的判断可以完全跳过）
	视锥裁剪：用视锥体和Root进行相交判断，如果AABB完全在视锥外，直接剔除Root（剪枝），如果视锥完全包含当前AABB，直接添加当前Root下所有Mesh，如果只是相交，递归检查8个孩子，递归到孩子结点，如果还相交，就添加到可见物体数组
*/
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
					CurSpace->Objects.clear(); // TODO:如果有物体存在Root上，这行代码就把他删了。。。。
				}
			}
		}
		

		void AddObject(T& Object) {
			AddObjectInner(Object, Root);
		}
		bool AddObjectInner(T& Object, std::shared_ptr<OctreeNode<T>> Root) {
			if (!Root) return false;
			if (!Root->AABB.IsContains(Object.aabb)) {
				return false;
			}
			bool isHandled = false;
			for (OctreeNode<T>* CurSpace : Root->children) {
				if (CurSpace && CurSpace->AABB.IsContains(obj.aabb))
				{
					// 说明子结点插入失败,交给当前结点处理
					isHandled = AddObjectInner(object, CurSpace);
				}
			}
			if (!isHandled) {
				Root->Objects.push_back(object);
			}
			return true;
		}


	private:
		std::shared_ptr<OctreeNode<T>> Root = nullptr;
	};
}

