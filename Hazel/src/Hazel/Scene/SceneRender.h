#pragma once
#include "Scene.h"
#include "Hazel/Asset/Model/Mesh.h"
#include <Hazel/Renderer/old/ComputePass.h>
#include "Pass/EnvPass.h"
namespace GameEngine {

	class SceneRender : public RefCounted
	{
	public: // open
		SceneRender();
		Ref<Image2D> GetTextureWhichNeedDebug() { 
			return m_BloomImage->GetImage();
			// return m_TransmittanceLutImage->GetImage();
		};
		Ref<Image2D> GetFinalImage() { return m_GridFrameBuffer->GetImage(0); }
		void PreRender(SceneInfo sceneData);
		void EndRender();
		void SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform);
		void SubmitDynamicMesh(Ref<MeshSource> meshSource, uint32_t submeshIndex, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms);
	private: // Initial
		void Init();
		void InitBuffers();
		void InitDirShadowPass();
		void InitGeoPass();
		void InitGridPass();
		void InitPreDepthPass();
		void InitSpotShadowPass();
		void InitHZBPass();
		void InitLightPass();
		void InitEnvPass();
		void InitSceneCompositePass();
		void InitSkyPass();
		void InitBloomPass();
		void InitAtmospherePass();

	private: // Update Pre Frame
		void UploadCameraData();
		void UploadMeshAndBoneTransForm();
		void UploadCSMShadowData();
		void HandleResizeRuntime();
		void UploadSpotShadowData();
		void HandleHZBResize();
		void UploadRenderSettingData();
		void uploadSceneData();

	private: // Pass
		void TransmiitanceLutPass();
		void MultiScatteringLutPass();
		void SkyViewLutPass();

		void preCompute();
		void BloomPass();
		void Draw();
		void ShadowPass();
		void GeoPass();
		void GridPass();
		void PreDepthPass();
		void SpotShadowPass();
		void HZBComputePass();
		void LightPass();
		void SceneCompositePass();
		void SkyPass();

		void ClearPass(Ref<RenderPass> renderPass, bool explicitClear = false);
	private: // Utils
		void SetViewprotSize(float width, float height);
	private: // struct
		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};
		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "a_MRow0" },
			{ ShaderDataType::Float4, "a_MRow1" },
			{ ShaderDataType::Float4, "a_MRow2" },
		};
		VertexBufferLayout boneInfluenceLayout = {
			{ ShaderDataType::Int4,   "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_BoneWeights" },
		};
		using BoneTransforms = std::array<glm::mat4, 100>; // Note: 100 == MAX_BONES from the shaders

		struct MeshKey
		{
			AssetHandle MeshHandle;
			AssetHandle MaterialHandle;
			uint32_t SubmeshIndex;
			bool IsSelected;

			MeshKey(AssetHandle meshHandle, AssetHandle materialHandle, uint32_t submeshIndex, bool isSelected)
				: MeshHandle(meshHandle), MaterialHandle(materialHandle), SubmeshIndex(submeshIndex), IsSelected(isSelected)
			{
			}

			bool operator<(const MeshKey& other) const
			{
				if (MeshHandle < other.MeshHandle)
					return true;

				if (MeshHandle > other.MeshHandle)
					return false;

				if (SubmeshIndex < other.SubmeshIndex)
					return true;

				if (SubmeshIndex > other.SubmeshIndex)
					return false;

				if (MaterialHandle < other.MaterialHandle)
					return true;

				if (MaterialHandle > other.MaterialHandle)
					return false;

				return IsSelected < other.IsSelected;
			}
		};
		struct CascadeData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			float SplitDepth;
		};
		struct StaticDrawCommand
		{
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> MaterialAsset;
			uint32_t InstanceCount = 0;
		};
		struct DynamicDrawCommand
		{
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> MaterialAsset;
			uint32_t InstanceCount = 0;
			uint32_t InstanceOffset = 0;
			bool IsRigged = false;
		};
		struct TransformVertexData
		{
			glm::vec4 MRow[3]; // 4行4列存储Model变换矩阵，最后一行不存，因为固定是0001
		};
		struct TransformBuffer
		{
			Ref<VertexBuffer> Buffer;
			TransformVertexData* Data = nullptr;
		};
		struct TransformMapData
		{
			std::vector<TransformVertexData> Transforms;
			uint32_t TransformOffset = 0;
		};
		struct BoneTransformsMapData
		{
			std::vector<BoneTransforms> BoneTransformsData;
			uint32_t BoneTransformsBaseIndex = 0;
		};
		struct UBViewProjToLight
		{
			glm::mat4 ViewProjection[4];
		};
		struct SpotLightMatrixs
		{
			glm::mat4 ShadowMatrices[1000]{};
		} SpotShadowDataUB;
		struct CameraData {
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
			float Width;
			float Height;
			float Near;
			float Far;
			glm::vec3 Position;
			float padding;
			glm::mat4 InverseViewProj;
		};
		struct UBSpotLights
		{
			uint32_t Count{ 0 };
			glm::vec3 Padding{};
			SpotLight SpotLights[1000]{};
		} SpotLightUB;
		struct HierarchicalDepthTexture
		{
			Ref<Texture2D> Texture;
			std::vector<Ref<ImageView>> ImageViews; // per-mip
		} m_HierarchicalDepthTexture;

		struct SceneDataForShader {
			DirectionalLight DirectionalLight;
			float EnvironmentMapIntensity;
			uint32_t isDynamic;
		};

	private: // Utils which need struct
		void CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<glm::mat4>& boneTransforms);
		void CalculateCascades(CascadeData* cascades, const EditorCamera& sceneCamera, const glm::vec3& lightDirection) const;
	private: // member
		// 配置信息
		bool NeedResize = false;
		float m_ViewportWidth = 1216.0f, m_ViewportHeight = 849.0f;
		uint32_t shadowMapResolution = 4096;
		uint32_t NumShadowCascades = 4;
		uint32_t TrasmittanceLutWidth = 256;
		uint32_t TrasmittanceLutHeight = 64;
		uint32_t MultiScatteringLutResolution = 32;
		uint32_t SkyViewLutWidth = 256;
		uint32_t SkyViewLutHeight = 128;
		Ref<RenderCommandBuffer> m_CommandBuffer;
		glm::vec4 CascadeSplits;
		// 收集来自场景的数据
		SceneInfo m_SceneDataFromScene;

		// 这些Map用于注册要绘制的Mesh，同时自动处理实例化
		std::map<MeshKey, DynamicDrawCommand> m_DynamicDrawList;
		std::map<MeshKey, StaticDrawCommand> m_StaticMeshDrawList;
		std::map<MeshKey, TransformMapData> m_MeshTransformMap;
		std::map<MeshKey, BoneTransformsMapData> m_MeshBoneTransformsMap;

		// 用一个很大的顶点缓冲区来存储所有的变换矩阵（这是为了适配实例化渲染。切换一个实例，才会切换一次数据）
		const size_t TransformBufferCount = 10 * 1024;
		std::vector<TransformBuffer> m_SubmeshTransformBuffers;

		// 骨骼变换矩阵 SSBO
		BoneTransforms* m_BoneTransformsData = nullptr;
		Ref<StorageBufferSet> m_SBSBoneTransforms;

		// 摄像机数据 UBO
		std::vector<CameraData> m_CameraData;
		Ref<UniformBufferSet> m_UBSCameraData;

		// 平行光级联阴影变换矩阵
		std::vector<UBViewProjToLight> m_ViewProjToLigthData;
		Ref<UniformBufferSet> m_UBSViewProjToLight;

		// TODO: 聚光灯级联阴影变换矩阵
		std::vector<SpotLightMatrixs> m_SpotLightMatrixData;
		Ref<UniformBufferSet> m_UBSSpotLightMatrixData;

		// 渲染设置 UBO
		std::vector<RenderSettingData> m_RenderSettingData;
		Ref<UniformBufferSet> m_UBSRenderSetting;

		// Shader需要的场景数据
		std::vector<SceneDataForShader> m_SceneDataForShader;
        Ref<UniformBufferSet> m_UBSSceneDataForShader;

	private: // Pass
		// shadowPass
		std::vector<Ref<RenderPass>> m_DirectionalShadowMapPass; // Per-cascade
		std::vector<Ref<RenderPass>> m_DirectionalShadowMapAnimPass; // Per-cascade
		Ref<Pipeline> m_ShadowPassPipelines[4];
		Ref<Pipeline> m_ShadowPassPipelinesAnim[4];

		Ref<Framebuffer> m_SpotFrameBuffer;
		Ref<Framebuffer> m_SpotFrameAnimBuffer;
		Ref<Pipeline> m_SpotShadowPassPipeline;
		Ref<Pipeline> m_SpotShadowPassAnimPipeline;
		Ref<RenderPass> m_SpotShadowPass;
		Ref<RenderPass> m_SpotShadowAnimPass;

		//PreDepthPass
		Ref<Pipeline> m_PreDepthPipeline;
		Ref<Pipeline> m_PreDepthPipelineAnim;
		Ref<RenderPass> m_PreDepthPass, m_PreDepthAnimPass;
		Ref<Framebuffer> m_PreDepthClearFramebuffer, m_PreDepthLoadFramebuffer;

		// GeoPass
		Ref<RenderPass> m_GeoPass;
		Ref<Pipeline> m_GeoPipeline;
		Ref<Framebuffer> m_GeoFrameBuffer;

		// GeoAnimPass
		Ref<RenderPass> m_GeoAnimPass;
		Ref<Pipeline> m_GeoAnimPipeline;
		Ref<Framebuffer> m_GeoAnimFrameBuffer;

		// GridPass
		Ref<RenderPass> m_GridPass;
		Ref<Pipeline> m_GridPipeline;
		Ref<Framebuffer> m_GridFrameBuffer;

		// HierarchicalDepthPass
		Ref<ComputePass> m_HierarchicalDepthPass;

		// EnvPass
		EnvPass m_EnvPass;
		EnvTextures m_EnvTextures;


		// LightPass
		Ref<Framebuffer> m_LightPassFramebuffer;
		Ref<Pipeline> m_LightPassPipeline;
		Ref<RenderPass> m_LightPass;

		// finalPass
		Ref<Framebuffer> m_SceneCompositeFrameBuffer;
		Ref<Pipeline> m_SceneCompositePipeline;
		Ref<RenderPass> m_SceneCompositePass;

		// Sky
		Ref<Framebuffer> m_SkyFrameBuffer;
		Ref<Pipeline> m_SkyPipeline;
		Ref<RenderPass> m_SkyPass;

		Ref<Texture2D> m_TransmittanceLutImage;
		Ref<Texture2D> m_MultiScatteringLutImage;
		Ref<Texture2D> m_SkyViewLutImage;

		Ref<ComputePass> m_TransmittanceLutPass;
		Ref<ComputePass> m_MultiScatteringLutPass;
		Ref<ComputePass> m_SkyViewLutPass;


		// Bloom
		Ref<ComputePass> m_BloomPass;
		Ref<Shader> m_BloomShader;
		Ref<MaterialOld> m_BloomPreFilterMaterial;
		std::vector<Ref<MaterialOld>> m_BloomPreDownSamplerMaterials;
		std::vector<Ref<MaterialOld>> m_BloomPreUpSamplerMaterials;
		Ref<Texture2D> m_BloomImage;
		std::vector<Ref<ImageView>> m_BloomImageViews;
	};
}
