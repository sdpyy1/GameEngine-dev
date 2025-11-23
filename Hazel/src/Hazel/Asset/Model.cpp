#include "hzpch.h"
#include "Model.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

namespace GameEngine {
	Model::Model(std::string path, ModelProcessSetting processSetting) : path(path), processSetting(processSetting) {}

	void Model::OnLoadAsset()
	{

	}

	void Model::OnSaveAsset()
	{
		LoadFromFile(path);

	}

	void Model::LoadFromFile(std::string path)
	{
		if (processSetting.generateVirtualMesh) processSetting.smoothNormal = true;  //对于生成虚拟几何体需要顶点去重，强制平滑法线

		uint32_t processSteps = aiProcess_Triangulate | aiProcess_FixInfacingNormals;
		if (processSetting.flipUV) processSteps |= aiProcess_FlipUVs;
		if (processSetting.smoothNormal) processSteps |= aiProcess_DropNormals | aiProcess_GenSmoothNormals;
		if (!processSetting.smoothNormal) processSteps |= aiProcess_JoinIdenticalVertices | aiProcess_GenNormals;  //不需要平滑法线就可以合并重复顶点了，

		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path, processSteps);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG_ERROR("Assimp load error : {}", import.GetErrorString());
			return;
		}

		std::vector<aiMesh*> processMeshes;
		ProcessNode(scene->mRootNode, scene, processMeshes);
		submeshes.resize(processMeshes.size());
		if (processSetting.loadMaterials) materials.resize(processMeshes.size());

		//并行加载各个子mesh
		for (int i = 0; i < processMeshes.size(); i++)
		{
			aiMesh* mesh = processMeshes[i];
			LOG_INFO("[{}/{}] Start processing mesh [{}].", i, scene->mNumMeshes, mesh->mName.C_Str());

			ProcessMesh(mesh, scene, i);
		}
		textureMap.clear();


		// 统计信息
		totalIndex = 0;
		totalVertex = 0;
		for (auto& submesh : submeshes)
		{
			totalIndex += submesh.mesh->index.size();
			totalVertex += submesh.mesh->position.size();
		}
		if (processSetting.generateCluster)
		{
			
		}
		if (processSetting.generateVirtualMesh)
		{
			
		}


	}


	void Model::ProcessNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& processMeshes)
	{
		// 处理节点所有的网格（如果有的话）
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			processMeshes.push_back(mesh);
		}
		// 接下来对它的子节点重复这一过程
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, processMeshes);
		}
	}

	void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, int index)
	{
        std::shared_ptr<Mesh> submesh = std::make_shared<Mesh>();

        // 处理顶点位置
        submesh->position = std::vector<glm::vec3>(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            submesh->position[i](0) = mesh->mVertices[i].x;
            submesh->position[i](1) = mesh->mVertices[i].y;
            submesh->position[i](2) = mesh->mVertices[i].z;
        }

        // 处理顶点法线
        if (mesh->mNormals)
        {
            submesh->normal = std::vector<glm::vec3>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->normal[i](0) = mesh->mNormals[i].x;
                submesh->normal[i](1) = mesh->mNormals[i].y;
                submesh->normal[i](2) = mesh->mNormals[i].z;
            }
        }

        // 处理顶点颜色
        if (mesh->mColors[0])
        {
            submesh->color = std::vector<glm::vec3>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->color[i](0) = mesh->mColors[0][i].r;
                submesh->color[i](1) = mesh->mColors[0][i].g;
                submesh->color[i](2) = mesh->mColors[0][i].b;
            }
        }

        // 处理顶点纹理坐标
        if (mesh->mTextureCoords[0])
        {
            submesh->texCoord = std::vector<glm::vec2>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->texCoord[i](0) = mesh->mTextureCoords[0][i].x;
                submesh->texCoord[i](1) = mesh->mTextureCoords[0][i].y;
            }
        }
        // if (mesh->mTextureCoords[1])
        // {
        //     submesh->texCoord = std::vector<Vec2>(mesh->mNumVertices);
        //     for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        //     {  
        //         submesh->texCoord[i](0) = mesh->mTextureCoords[1][i].x;
        //         submesh->texCoord[i](1) = mesh->mTextureCoords[1][i].y;    
        //     }
        // }

        // 处理索引,已经三角面化了就全当3处理了
        submesh->index = std::vector<uint32_t>(mesh->mNumFaces * 3);

        int tempCnt = 0;
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
                submesh->index[j + tempCnt] = face.mIndices[j];
            }
            tempCnt += face.mNumIndices;
        }

        // 处理顶点切线
        if (mesh->mTangents)
        {
            submesh->tangent = std::vector<glm::vec4>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->tangent[i](0) = mesh->mTangents[i].x;
                submesh->tangent[i](1) = mesh->mTangents[i].y;
                submesh->tangent[i](2) = mesh->mTangents[i].z;
                submesh->tangent[i](3) = 1.0f;  //最后一位为符号(手性)
            }
        }
        else if (processSetting.tangentSpace)
        {
            if (submesh->normal.size() == 0 ||
                submesh->position.size() == 0 ||
                submesh->texCoord.size() == 0)      // 必须要有这些数据才能生成
            {
                LOG_ERROR("Try to generate tangent space but missing necesscary datas!");
            }
            else
            {
                submesh->tangent = std::vector<Vec4>(mesh->mNumVertices);

                TangentSpace tangentCalculator = TangentSpace();
                tangentCalculator.Generate(submesh.get());  // 需要先把上面的信息准备完成
            }
        }

        // 处理材质
        if (processSetting.loadMaterials && mesh->mMaterialIndex >= 0)
        {
            aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];

            if (materials[index] == nullptr) // 首次创建；后续通过序列化创建时会绑定第一次创建的材质
            {
                materials[index] = std::make_shared<Material>();
                std::shared_ptr<V2::Texture> diffuse = LoadMaterialTexture(aiMaterial, aiTextureType_DIFFUSE);
                std::shared_ptr<V2::Texture> normal = LoadMaterialTexture(aiMaterial, aiTextureType_NORMALS);
                std::shared_ptr<V2::Texture> specular = LoadMaterialTexture(aiMaterial, aiTextureType_SPECULAR);
                //std::shared_ptr<Texture> unknownTexture = LoadMaterialTexture(aiMaterial, aiTextureType_UNKNOWN);

                materials[index]->SetDiffuse(diffuse);
                materials[index]->SetNormal(normal);
                materials[index]->SetSpecular(specular);
            }
        }

        // 处理骨骼
        if (mesh->HasBones())   ExtractBoneWeights(submesh.get(), mesh, scene);

        // 处理包围盒
        submesh->aabb = AxisAlignedBox(submesh->position[0], Vec3::Zero());
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)   submesh->aabb.Merge(submesh->position[i]);
        submesh->sphere = BoundingSphere(submesh->aabb);
        submesh->box = BoundingBox(submesh->aabb);

        // 处理mesh名称
        submesh->name = std::string(mesh->mName.C_Str());

        // 优化缓存
        // MeshOptimizor::OptimizeMesh(submesh);

        // 添加到mesh asset
        submeshes[index].mesh = submesh;

        // 处理分簇
        if (processSetting.generateCluster)
        {
            
        }

        // 处理虚拟几何体
        if (processSetting.generateVirtualMesh)
        {
           
        }
	}
    void Model::ExtractBoneWeights(Mesh* submesh, aiMesh* mesh, const aiScene* scene)
    {
        submesh->boneIndex = std::vector<IVec4>(mesh->mNumVertices);
        submesh->boneWeight = std::vector<Vec4>(mesh->mNumVertices);

        // 将骨骼相关信息初始化
        for (int i = 0; i < mesh->mNumVertices; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                submesh->boneIndex[i](j) = -1;
                submesh->boneWeight[i](j) = 0.0f;
            }
        }

        // 遍历骨骼
        for (uint32_t index = 0; index < mesh->mNumBones; ++index)
        {
            int boneIndex = -1;
            std::string boneName = mesh->mBones[index]->mName.C_Str();

            bool find = false;
            for (int i = 0; i < submesh->bone.size(); i++)
            {
                if (submesh->bone[i].name.compare(boneName) == 0)
                {
                    boneIndex = submesh->bone[i].index;
                    find = true;
                    break;
                }
            }
            if (!find)
            {
                BoneInfo newBoneInfo;
                newBoneInfo.index = (int)submesh->bone.size();
                newBoneInfo.name = boneName;
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        newBoneInfo.offset(i, j) = mesh->mBones[index]->mOffsetMatrix[i][j];
                    }
                }
                newBoneInfo.offset.transposeInPlace();  // 要做一个转置？
                newBoneInfo.name = std::string(boneName);
                submesh->bone.push_back(newBoneInfo);

                boneIndex = newBoneInfo.index;
            }

            auto weights = mesh->mBones[index]->mWeights;
            int numWeights = mesh->mBones[index]->mNumWeights;

            // 处理和该骨骼相关的顶点
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;

                for (int i = 0; i < 4; ++i)
                {
                    if (submesh->boneIndex[vertexId](i) < 0)
                    {
                        submesh->boneIndex[vertexId](i) = boneIndex;
                        submesh->boneWeight[vertexId](i) = weight;
                        break;
                    }
                }
            }
        }
    }


    std::shared_ptr<V2::Texture> Model::LoadMaterialTexture(aiMaterial* mat, aiTextureType type)
    {
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)   //可以有很多个，只用了一个
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::string texturePath = EngineContext::File()->RemoveFilename(path).append("/").append(str.C_Str());

            auto iter = textureMap.find(texturePath);   // 先从缓存中找
            if (iter != textureMap.end())    return iter->second;
            else
            {
                std::shared_ptr<V2::Texture> texture = std::make_shared<V2::Texture>(texturePath);
                // EngineContext::Asset()->SaveAsset(texture);
                textureMap[texturePath] = texture;
                return texture;
            }
        }
        return nullptr;
    }
}
