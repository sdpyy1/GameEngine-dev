#pragma once
#include "Hazel/Asset/Asset.h"
#include "Hazel/Renderer/RenderResource/Texture.h"
#include "Mesh.h"
#include "Hazel/Renderer/RenderResource/RenderBuffer.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "Hazel/Renderer/RenderResource/Material.h"
#include "Hazel/Utils/Serializable.h"
namespace GameEngine { 

    typedef struct ModelSpec
    {
        bool flipUV = false;
        bool loadMaterials = true; 
        bool genBLAS = false;
        bool uploadGPU = false;  // CPU内部创建模型，必须true
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

        BeginSerailize
            SerailizeEntry(mesh)
         
            BeginIfLoad //加载时需要把顶点数据上传到GPU
            LOG_TRACE("  - Vertex Count: {}", mesh->position.size());
            LOG_TRACE("  - Index Count: {}", mesh->index.size());
            // 上传到GPU
            vertexBuffer = std::make_shared<VertexBuffer>();
            vertexBuffer->SetPosition(mesh->position);
            vertexBuffer->SetNormal(mesh->normal);
            vertexBuffer->SetTangent(mesh->tangent);
            vertexBuffer->SetTexCoord(mesh->texCoord);
            vertexBuffer->SetColor(mesh->color);
            vertexBuffer->SetBoneIndex(mesh->boneIndex);
            vertexBuffer->SetBoneWeight(mesh->boneWeight);
            vertexBuffer->SetBoundingBox(mesh->box);

            const MeshInfo& vi = vertexBuffer->vertexInfo;
            LOG_TRACE("  - Vertex Buffer Info:");
            LOG_TRACE("    positionID:    {}", vi.positionID);
            LOG_TRACE("    normalID:      {}", vi.normalID);
            LOG_TRACE("    tangentID:     {}", vi.tangentID);
            LOG_TRACE("    texCoordID:    {}", vi.texCoordID);
            LOG_TRACE("    colorID:       {}", vi.colorID);
            LOG_TRACE("    boneIndexID:   {}", vi.boneIndexID);
            LOG_TRACE("    boneWeightID:  {}", vi.boneWeightID);
            indexBuffer = std::make_shared<IndexBuffer>();
            indexBuffer->SetIndex(mesh->index);
            LOG_TRACE("    IndexBufferID: {}", indexBuffer->indexID);

            // RayTracing
            if (RENDER_ENABLE_RAY_TRACING) {
                RHIBottomLevelAccelerationStructureInfo blasInfo = {};
                blasInfo.vertexBuffer = vertexBuffer->positionBuffer;
                blasInfo.indexBuffer = indexBuffer->buffer;
                blasInfo.triangleCount = mesh->TriangleNum();
                blasInfo.vertexStride = sizeof(glm::vec3);
                blasInfo.indexOffset = 0;
                blasInfo.vertexOffset = 0;
                blas = APP_DYNAMICRHI->CreateBottomLevelAccelerationStructure(blasInfo);
            }
            EndIfLoad

        EndSerailize
    };


	class Model : public Asset {
    public:
        Model() = default;
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
        MaterialRef GetMaterial(uint32_t index) { return materials[index]; }
        std::string GetPath() { return path; }
        VertexBufferRef GetVertexBuffer(int index){return submeshes[index].vertexBuffer;}
        IndexBufferRef GetIndexBuffer(int index){return submeshes[index].indexBuffer;}
    private:
        void ProcessNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& processMeshes);
        void ProcessMesh(aiMesh* mesh, const aiScene* scene, int index);
        void ExtractBoneWeights(Mesh* submesh, aiMesh* mesh, const aiScene* scene);
        std::shared_ptr<Texture> Model::LoadMaterialTexture(std::string texturePath, bool srgb = true, bool yFlip = true);
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
            SerailizeAssetParent
            SerailizeEntry(path)
            SerailizeEntry(m_ModelSpec)
            SerailizeEntry(totalIndex)
            SerailizeEntry(totalVertex)
            SerailizeEntry(totalClusterCnt)
            SerailizeEntry(totalClusterMaxMip)
            SerailizeEntry(submeshes)
            SerailizeEntry(materials)
        EndSerailize
    };

    using ModelRef = std::shared_ptr<Model>;
}

