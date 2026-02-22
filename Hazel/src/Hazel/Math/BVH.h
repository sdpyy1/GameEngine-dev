#pragma once
#include "collision.h"
/*
	BVH:（TODO：目前是伪代码，SAH未完成）
	构建方式： 计算所有Mesh的AABB在哪个轴上最宽，在这个轴上对Mesh进行排序,根据某种划分算法把Mesh数组分成两部分，分别递归构建子树，直到叶子结点
	添加Mesh：首先插入一定发生在叶子结点，依据SAH，判断依哪一个节点的插入会有“最小表面积”的变化，具体计算公式看Blog
*/
namespace GameEngine 
{
	template<typename T>
	class BVHNode {
	public:
		BVHNode* left = nullptr;
		BVHNode* right = nullptr;
		BVHNode* parent = nullptr;
		std::vector<T> objects;
		BoundingBox AABB;
	};


	template<typename T>
	class BVH
	{
	public:
		BVHNode<T>* BuildBVHInner(std::vector<T>&objects, BVHNode<T>* Root) {
			// 1. 计算Root的AABB
			for (auto& object : objects) {
				Root->AABB.Merge(object.aabb);
			}

			// 创建Root
			if(!Root) Root = new BVHNode<T>();
			if (objects.size() <= 1) {
                Root->objects = objects;
                return Root;
			}
			// 2. 利用SAH算法，对Root进行划分
			int lefeMaxIndex = SAHSplit(Root, objects,int left, int right);

			// 3. 递归构建左右子树
			BVHNode<T>* left = BuildBVHInner(objects.begin() + left, objects.begin() + lefeMaxIndex, Root);
            BVHNode<T>* right = BuildBVHInner(objects.begin() + left + lefeMaxIndex, objects.begin() + right, Root);
			Root->left = left;
            Root->right = right;
            left->parent = right->parent = Root;
            return Root;
		}


		// SAH算法，返回划分后left的最大Index
		int SAHSplit(BVHNode<T>* node, std::vector<T>& objects, int left, int right) {

		}
		BVHNode<T>* Root = nullptr;
	};


}