#include "hzpch.h"
#include "Model.h"
#include "Hazel/Renderer/RenderResource/Material.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
namespace GameEngine {
	Model::Model(std::string path, ModelProcessSetting processSetting) : path(path), processSetting(processSetting) {
        LoadFromFile(path);
    }

	void Model::OnLoadAsset()
	{

	}

	void Model::OnSaveAsset()
	{

	}
    static const uint32_t s_MeshImportFlags =
        aiProcess_CalcTangentSpace          // Create binormals/tangents just in case
        | aiProcess_Triangulate             // Make sure we're triangles
        | aiProcess_SortByPType             // Split meshes by primitive type
        | aiProcess_GenNormals              // Make sure we have legit normals
        | aiProcess_GenUVCoords             // Convert UVs if required 
        | aiProcess_OptimizeMeshes          // Batch draws where possible
        | aiProcess_JoinIdenticalVertices
        | aiProcess_LimitBoneWeights        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
        | aiProcess_ValidateDataStructure   // Validation
        ;
    void Model::LoadFromFile(std::string path)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, s_MeshImportFlags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            LOG_ERROR("Assimp load error : {}", import.GetErrorString());
            return;
        }

        std::vector<aiMesh*> processMeshes;
        ProcessNode(scene->mRootNode, scene, processMeshes);
        submeshes.resize(processMeshes.size());
        if (processSetting.loadMaterials) materials.resize(processMeshes.size());

        // Submesh
        for (int i = 0; i < processMeshes.size(); i++)
        {
            aiMesh* mesh = processMeshes[i];
            LOG_INFO_TAG("Model", LOG_LINE);
            LOG_TRACE("[{}/{}] Start processing mesh [{}].", i + 1, scene->mNumMeshes, mesh->mName.C_Str());
            ProcessMesh(mesh, scene, i);
            LOG_INFO_TAG("Model", LOG_LINE);
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
    }


	void Model::ProcessNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& processMeshes)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			processMeshes.push_back(mesh);
		}
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, processMeshes);
		}
	}

	void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, int index)
	{
        std::shared_ptr<Mesh> submesh = std::make_shared<Mesh>();

        // 顶点位置
        submesh->position = std::vector<glm::vec3>(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            submesh->position[i].x = mesh->mVertices[i].x;
            submesh->position[i].y = mesh->mVertices[i].y;
            submesh->position[i].z = mesh->mVertices[i].z;
        }
        // 顶点法线
        if (mesh->mNormals)
        {
            submesh->normal = std::vector<glm::vec3>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->normal[i].x = mesh->mNormals[i].x;
                submesh->normal[i].y = mesh->mNormals[i].y;
                submesh->normal[i].z = mesh->mNormals[i].z;
            }

        }
        // 顶点颜色
        if (mesh->mColors[0])
        {
            submesh->color = std::vector<glm::vec3>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->color[i].x = mesh->mColors[0][i].r;
                submesh->color[i].y = mesh->mColors[0][i].g;
                submesh->color[i].z = mesh->mColors[0][i].b;
            }
        }

        // 顶点纹理坐标
        if (mesh->mTextureCoords[0])
        {
            submesh->texCoord = std::vector<glm::vec2>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->texCoord[i].x = mesh->mTextureCoords[0][i].x;
                submesh->texCoord[i].y = mesh->mTextureCoords[0][i].y;
            }
        }
        

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

        if (mesh->mTangents)
        {
            submesh->tangent = std::vector<glm::vec4>(mesh->mNumVertices);
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                submesh->tangent[i].x = mesh->mTangents[i].x;
                submesh->tangent[i].y = mesh->mTangents[i].y;
                submesh->tangent[i].z = mesh->mTangents[i].z;
                submesh->tangent[i].w = 1.0f;
            }
        }
        
        // 处理材质
        if (processSetting.loadMaterials && mesh->mMaterialIndex >= 0)
        {
            aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];  // 这就获得了当前SubMesh相关的材质信息
			auto aiMaterialName = aiMaterial->GetName();
			LOG_TRACE("Load Material [{}]", aiMaterialName.data);

			MaterialRef ma = std::make_shared<Material>();

			aiString aiTexPath;
			glm::vec4 albedoColor(1.0f);
			glm::vec4 emission(0,0,0,1);
			aiColor3D aiColor(1.0f), aiEmission(0.0f);
			if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
				albedoColor = { aiColor.r, aiColor.g, aiColor.b ,1.0};

			if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS)
				emission = { aiEmission.r, aiEmission.g ,aiEmission.b, 1.0 };

			ma->SetDiffuse(albedoColor);
			ma->SetEmission(emission);

			float roughness, metalness;
			if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
				roughness = 0.4f; // Default value

			if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
				metalness = 0.0f;

			// TODO: 这是在干什么
			// Physically realistic materials are either metal (1.0) or not (0.0)
			// Some models seem to come in with 0.5 which seems wrong - materials are either metal or they are not.
			// (maybe these are specular workflow, and what we're seeing is specular = 0.5 in AI_MATKEY_REFLECTIVITY (?))
			if (metalness < 0.9f)
				metalness = 0.0f;
			else
				metalness = 1.0f;

			ma->SetRoughness(roughness);
			ma->SetMetallic(metalness);

			LOG_TRACE("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
            LOG_TRACE("    ROUGHNESS = {0}", roughness);
            LOG_TRACE("    METALNESS = {0}", metalness);


			// 颜色贴图
			bool hasAlbedoMap = aiMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &aiTexPath) == AI_SUCCESS;
			if (!hasAlbedoMap)
			{
				// no PBR base color. Try old-school diffuse  (note: should probably combine with specular in this case)
				hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
			}
			if (hasAlbedoMap)
			{
				TextureRef albedo = LoadMaterialTexture(aiTexPath.C_Str());
				ma->SetDiffuse(albedo);
				ma->SetDiffuse(glm::vec4(1.0f));  // 有贴图就就不需要了
            }



			// 自发光贴图
			bool hasEmissiveMap = aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &aiTexPath) == AI_SUCCESS;
			if (hasEmissiveMap) {
                TextureRef emission = LoadMaterialTexture(aiTexPath.C_Str());
				ma->SetEmission(emission);
			}

			// 法线贴图
			bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
			if (hasNormalMap)
			{
                TextureRef textureHandle = LoadMaterialTexture(aiTexPath.C_Str());
				ma->SetNormal(textureHandle);
				ma->SetUseNormalTexture(true);
			}

			// 粗糙度贴图
			bool hasRoughnessMap = aiMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &aiTexPath) == AI_SUCCESS;
			bool invertRoughness = false;
			if (!hasRoughnessMap)
			{
				// no PBR roughness. Try old-school shininess.  (note: this also picks up the gloss texture from PBR specular/gloss workflow).
				// Either way, Roughness = (1 - shininess)
				hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
				invertRoughness = true;
			}

			if (hasRoughnessMap)
			{
                TextureRef roughnessTextureHandle = LoadMaterialTexture(aiTexPath.C_Str());
				ma->SetRoughness(roughnessTextureHandle);
				ma->SetRoughness(1.0f);
            }

			// Metalness map
			bool hasMetalnessMap = aiMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &aiTexPath) == AI_SUCCESS;
			if (hasMetalnessMap)
			{
				
                TextureRef metalnessTextureHandle = LoadMaterialTexture(aiTexPath.C_Str());

				ma->SetMetallic(metalnessTextureHandle);
				ma->SetMetallic(1.0f);
            }
            materials[index] = ma;
		}
        else {
            materials[index] = std::make_shared<Material>(true); // 默认材质
        }
        // 处理骨骼
        if (mesh->HasBones())   ExtractBoneWeights(submesh.get(), mesh, scene);

        // 处理包围盒
        submesh->aabb = AxisAlignedBox(submesh->position[0], glm::zero<glm::vec3>());
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)   submesh->aabb.Merge(submesh->position[i]);
        submesh->sphere = BoundingSphere(submesh->aabb);
        submesh->box = BoundingBox(submesh->aabb);

        submesh->name = std::string(mesh->mName.C_Str());

        // 优化缓存
        // MeshOptimizor::OptimizeMesh(submesh);

        // 添加到mesh asset
        submeshes[index].mesh = submesh;


        LOG_TRACE("  - Vertex Count: {}", submeshes[index].mesh->position.size());
        LOG_TRACE("  - Index Count: {}", submeshes[index].mesh->index.size());

        // 上传到GPU
        VertexBufferRef vertexBuffer = std::make_shared<VertexBuffer>();
        vertexBuffer->SetPosition(submesh->position);
        vertexBuffer->SetNormal(submesh->normal);
        vertexBuffer->SetTangent(submesh->tangent);
        vertexBuffer->SetTexCoord(submesh->texCoord);
        vertexBuffer->SetColor(submesh->color);
        vertexBuffer->SetBoneIndex(submesh->boneIndex);
        vertexBuffer->SetBoneWeight(submesh->boneWeight);
        submeshes[index].vertexBuffer = vertexBuffer;
        const MeshInfo& vi = vertexBuffer->vertexInfo;
        LOG_TRACE("  - Vertex Buffer Info:");
        LOG_TRACE("    positionID:    {}", vi.positionID);
        LOG_TRACE("    normalID:      {}", vi.normalID);
        LOG_TRACE("    tangentID:     {}", vi.tangentID);
        LOG_TRACE("    texCoordID:    {}", vi.texCoordID);
        LOG_TRACE("    colorID:       {}", vi.colorID);
        LOG_TRACE("    boneIndexID:   {}", vi.boneIndexID);
        LOG_TRACE("    boneWeightID:  {}", vi.boneWeightID);
        IndexBufferRef indexBuffer = std::make_shared<IndexBuffer>();
        indexBuffer->SetIndex(submeshes[index].mesh->index);
        submeshes[index].indexBuffer = indexBuffer;
        LOG_TRACE("    IndexBufferID: {}", indexBuffer->indexID);
	}
    void Model::ExtractBoneWeights(Mesh* submesh, aiMesh* mesh, const aiScene* scene)
    {
        LOG_TRACE("Find Bone Info. Extracting bone weights...");
        findBone = true;
        submesh->boneIndex = std::vector<glm::ivec4>(mesh->mNumVertices);
        submesh->boneWeight = std::vector<glm::vec4>(mesh->mNumVertices);

        // 将骨骼相关信息初始化
        for (int i = 0; i < mesh->mNumVertices; i++)
        {
            submesh->boneIndex[i] = glm::ivec4(-1);
            submesh->boneWeight[i] = glm::vec4(.0f);
        }

        // 遍历mesh的骨骼
        for (uint32_t index = 0; index < mesh->mNumBones; ++index)
        {
            int boneIndex = -1;
            std::string boneName = mesh->mBones[index]->mName.C_Str();

            // 判断当前骨骼是不是在当前submesh上
            bool find = false;
            for (int i = 0; i < submesh->bone.size(); i++)
            {
                if (submesh->bone[i].name.compare(boneName) == 0)  // 当前骨骼是在当前submesh上
                {
                    boneIndex = submesh->bone[i].index;
                    find = true;
                    break;
                }
            }

            // 不是在当前Submesh上
            if (!find)
            {
                BoneInfo newBoneInfo;
                newBoneInfo.index = (int)submesh->bone.size();
                newBoneInfo.name = boneName;
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        newBoneInfo.offset[i][j] = mesh->mBones[index]->mOffsetMatrix[i][j];
                    }
                }
                // newBoneInfo.offset.transposeInPlace();  // 要做一个转置？
                newBoneInfo.name = std::string(boneName);
                submesh->bone.push_back(newBoneInfo);

                boneIndex = newBoneInfo.index;
            }

            auto weights = mesh->mBones[index]->mWeights;  // 当前骨骼对所有顶点的权重
            int numWeights = mesh->mBones[index]->mNumWeights;  // 当前骨骼影响了多少顶点

            // 处理和该骨骼相关的顶点
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                // 骨骼index对顶点vertexId的权重是weight
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;


                // 看vertexId对应顶点绑定的骨骼是否已满，选一个没用的位置设置骨骼和骨骼权重
                for (int i = 0; i < 4; ++i)
                {
                   if (submesh->boneIndex[vertexId].x < 0)
                   {
                       submesh->boneIndex[vertexId].x = boneIndex;
                       submesh->boneWeight[vertexId].x = weight;
                       break;
                    }
                   if (submesh->boneIndex[vertexId].y < 0)
                   {
                       submesh->boneIndex[vertexId].y = boneIndex;
                       submesh->boneWeight[vertexId].y = weight;
                       break;
                   }
                   if (submesh->boneIndex[vertexId].z < 0)
                   {
                       submesh->boneIndex[vertexId].z = boneIndex;
                       submesh->boneWeight[vertexId].z = weight;
                       break;
                   }
                   if (submesh->boneIndex[vertexId].w < 0)
                   {
                       submesh->boneIndex[vertexId].w = boneIndex;
                       submesh->boneWeight[vertexId].w = weight;
                       break;
                   }

                }
            }
        }
    }


    std::shared_ptr<Texture> Model::LoadMaterialTexture(std::string texturePath)
    {
        if (textureMap.find(texturePath) != textureMap.end())
		{
			return textureMap[texturePath];
		}
		TextureSpec textureSpec;
		textureSpec.yFlip = true;
		std::filesystem::path fs_path(path);
		fs_path = fs_path.parent_path();
		std::filesystem::path new_texture_path = fs_path / texturePath;
		textureSpec.path = new_texture_path.string();
		std::shared_ptr<Texture> texture = std::make_shared<Texture>(textureSpec);
		LOG_TRACE("Load Texture: {0}  Bindless ID:{1}", textureSpec.path, texture->GetbindlessID());
		textureMap[texturePath] = texture;
		return texture;
    }
}
