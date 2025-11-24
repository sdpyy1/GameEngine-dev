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
        bool smoothNormal = false;                  // 生成平滑法线
        bool flipUV = false;                        // 翻转UV
        bool loadMaterials = true;                 // 读取文件中的材质并生成材质资源
        bool tangentSpace = false;                  // 生成切线
        bool generateBVH = false;                   // 生成BVH
        bool generateCluster = false;               // 生成Cluster
        bool generateVirtualMesh = false;           // 生成虚拟几何体
        bool cacheCluster = false;                  // 对于虚拟几何体和Cluster做缓存，只需要生成一次

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
        std::shared_ptr<V2::Mesh> mesh;                                 // CPU端的mesh和cluster信息
        //std::vector<MeshClusterRef> clusters;                       // 仅生成cluster时的信息
        //std::shared_ptr<VirtualMesh> virtualMesh;                   // 生成cluster + cluster group时的信息

       // VertexBufferRef vertexBuffer;                               // GPU端的顶点和索引缓冲，既可能存储单个submesh的全部顶点和索引，也可能存储其全部cluster合并后的数据
        //IndexBufferRef indexBuffer;

        //IndexRange meshClusterID = { 0, 0 };            // 提交的一组cluster的ID范围
        //IndexRange meshClusterGroupID = { 0, 0 };       // 提交的一组cluster group的ID范围

        //RHIBottomLevelAccelerationStructureRef blas;
    };


	class Model : public V2::Asset {
    public:
		Model(std::string path, ModelProcessSetting processSetting);
        void LoadFromFile(std::string path);
        virtual std::string GetAssetTypeName() override { return "Model Asset"; }
        virtual V2::AssetType GetAssetType() override { return V2::ASSET_TYPE_MODEL; }
        virtual void OnLoadAsset() override;
        virtual void OnSaveAsset() override;

    private:
        std::string path;
        ModelProcessSetting processSetting;
        uint64_t totalIndex = 0;    // 统计信息
        uint64_t totalVertex = 0;
        uint32_t totalClusterCnt = 0;
        uint32_t totalClusterMaxMip = 0;
        std::vector<SubmeshData> submeshes;
        std::vector<MaterialRef> materials;
        std::unordered_map<std::string, V2::TextureRef> textureMap; // Cache
        void ProcessNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& processMeshes);
        void ProcessMesh(aiMesh* mesh, const aiScene* scene, int index);
        void ExtractBoneWeights(V2::Mesh* submesh, aiMesh* mesh, const aiScene* scene);
        std::shared_ptr<V2::Texture> LoadMaterialTexture(aiMaterial* mat, aiTextureType type);
    };
}

