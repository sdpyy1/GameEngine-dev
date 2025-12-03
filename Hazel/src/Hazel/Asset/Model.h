#pragma once
#include "Hazel/Asset/Asset.h"
#include "Hazel/Renderer/RenderResource/Texture.h"
#include "Mesh.h"
#include "Hazel/Renderer/RenderResource/RenderBuffer.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "Hazel/Renderer/RenderResource/Material.h"
namespace GameEngine { 

    typedef struct ModelProcessSetting
    {
        bool flipUV = false;
        bool loadMaterials = true; 

    private:
        BeginSerailize()
            SerailizeEntry(smoothNormal)
            SerailizeEntry(flipUV)
            SerailizeEntry(loadMaterials)
            SerailizeEntry(tangentSpace)
            SerailizeEntry(generateBVH)
            SerailizeEntry(generateCluster)
            SerailizeEntry(generateVirtualMesh)
            SerailizeEntry(cacheCluster)
        EndSerailize

    }ModelProcessSetting;

    struct SubmeshData
    {
        std::shared_ptr<Mesh> mesh;                                 // CPU端的mesh和cluster信息
        VertexBufferRef vertexBuffer;                               // GPU端的顶点和索引缓冲，既可能存储单个submesh的全部顶点和索引，也可能存储其全部cluster合并后的数据
        IndexBufferRef indexBuffer;



        //std::vector<MeshClusterRef> clusters;                       // 仅生成cluster时的信息
        //std::shared_ptr<VirtualMesh> virtualMesh;                   // 生成cluster + cluster group时的信息



        //IndexRange meshClusterID = { 0, 0 };            // 提交的一组cluster的ID范围
        //IndexRange meshClusterGroupID = { 0, 0 };       // 提交的一组cluster group的ID范围

        //RHIBottomLevelAccelerationStructureRef blas;
    };


	class Model : public Asset {
    public:
		Model(std::string path, ModelProcessSetting processSetting);
        void LoadFromFile(std::string path);
        virtual std::string GetAssetTypeName() override { return "Model Asset"; }
        virtual AssetType GetAssetType() override { return ASSET_TYPE_MODEL; }
        virtual void OnLoadAsset() override;
        virtual void OnSaveAsset() override;
        bool hasBone() {return findBone;}
        std::vector<SubmeshData>& GetSubmeshes() { return submeshes; }
        SubmeshData& GetSubmeshData(uint32_t index) { return submeshes[index]; }
        MeshRef GetSubMesh(uint32_t index) { return submeshes[index].mesh; }
        std::vector<MaterialRef>& GetMaterials() { return materials; }
        std::string GetPath() { return path; }
        VertexBufferRef GetVertexBuffer(int index){return submeshes[index].vertexBuffer;}
        IndexBufferRef GetIndexBuffer(int index){return submeshes[index].indexBuffer;}
    private:
        void ProcessNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& processMeshes);
        void ProcessMesh(aiMesh* mesh, const aiScene* scene, int index);
        void ExtractBoneWeights(Mesh* submesh, aiMesh* mesh, const aiScene* scene);
        std::shared_ptr<Texture> Model::LoadMaterialTexture(std::string texturePath);
    private:
        std::string path;
        ModelProcessSetting processSetting;
        uint64_t totalIndex = 0;    // 统计信息
        uint64_t totalVertex = 0;
        uint32_t totalClusterCnt = 0;
        uint32_t totalClusterMaxMip = 0;
        std::vector<SubmeshData> submeshes;
        std::vector<MaterialRef> materials;
        std::unordered_map<std::string, TextureRef> textureMap; // Cache

        bool findBone = false;
    };

    using ModelRef = std::shared_ptr<Model>;
}

