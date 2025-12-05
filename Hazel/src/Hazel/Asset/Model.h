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

    typedef struct ModelSpec
    {
        bool flipUV = false;
        bool loadMaterials = true; 
        bool genBLAS = false;

    private:
        BeginSerailize
            SerailizeEntry(flipUV)
            SerailizeEntry(loadMaterials)
            SerailizeEntry(genBLAS)
        EndSerailize

    }ModelSpec;

    struct SubmeshData
    {
        std::shared_ptr<Mesh> mesh;                                
        VertexBufferRef vertexBuffer;
        IndexBufferRef indexBuffer;
        RHIBottomLevelAccelerationStructureRef blas;
    };


	class Model : public Asset {
    public:
		Model(std::string path, ModelSpec m_ModelSpec);
        void LoadFromFile(std::string path);
        virtual std::string GetAssetTypeName() override { return "Asset_Model"; }
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
        ModelSpec m_ModelSpec;
        uint64_t totalIndex = 0; 
        uint64_t totalVertex = 0;
        uint32_t totalClusterCnt = 0;
        uint32_t totalClusterMaxMip = 0;
        std::vector<SubmeshData> submeshes;
        std::vector<MaterialRef> materials;
        std::unordered_map<std::string, TextureRef> textureMap;

        bool findBone = false;
    private:
        BeginSerailize
            SerailizeBaseClass(Asset)
            SerailizeEntry(path)
            SerailizeEntry(m_ModelSpec)
        EndSerailize
    };

    using ModelRef = std::shared_ptr<Model>;
}

