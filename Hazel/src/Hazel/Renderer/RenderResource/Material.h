#pragma once
#include "Hazel/Asset/Asset.h"
#include <Hazel/Renderer/RenderResource/Texture.h>
#include "Shader.h"
#include "RenderStruct.h"
namespace GameEngine {

	namespace V2 {
		enum RenderPassMaskBits
		{
			PASS_MASK_NONE = 0x00000000,
			// PASS_MASK_DEPTH_PASS = 0x00000001,           // 暂时不支持，深度pass都使用默认着色器，后面再支持override吧
			PASS_MASK_FORWARD_PASS = 0x00000001,
			PASS_MASK_DEFERRED_PASS = 0x00000002,
			PASS_MASK_TRANSPARENT_PASS = 0x00000004,
			// PASS_MASK_POST_PROCESS_PASS = 0x00000008,    // 后处理的screen pass也暂时不支持

			PASS_MASK_MAX_ENUM = 0x7FFFFFFF,	//
		};
		typedef uint32_t RenderPassMasks;
		class Material : public V2::Asset
		{
		public:
			Material();
			Material(const Material& other) = default;  // 可以拷贝构造
			~Material();

			virtual std::string GetAssetTypeName() override { return "Material Asset"; }
			virtual AssetType GetAssetType() override { return ASSET_TYPE_MATERIAL; }

			virtual void OnLoadAsset() override;
			virtual void OnSaveAsset() override;

			uint32_t GetMaterialID() { return materialID; }

			void Update();
			void SetDiffuse(glm::vec4 diffuse) { this->diffuse = diffuse;      Update(); }
			void SetEmission(glm::vec4 emission) { this->emission = emission;    Update(); }
			void SetRoughness(float roughness) { this->roughness = roughness;  Update(); }
			void SetMetallic(float metallic) { this->metallic = metallic;    Update(); }
			void SetAlphaClip(float alphaClip) { this->alphaClip = alphaClip;  Update(); }
			void SetInt(int32_t data, uint32_t index) { ints[index] = data;           Update(); }
			void SetFloat(float data, uint32_t index) { floats[index] = data;         Update(); }
			void SetColor(glm::vec4 data, uint32_t index) { colors[index] = data;         Update(); }
			void SetDiffuse(TextureRef texture) { textureDiffuse = texture;     Update(); }
			void SetNormal(TextureRef texture) { textureNormal = texture;      Update(); }
			void SetARM(TextureRef texture) { textureArm = texture;         Update(); }
			void SetSpecular(TextureRef texture) { textureSpecular = texture;    Update(); }
			void SetTexture2D(TextureRef texture, uint32_t index) { texture2D[index] = texture;   Update(); }
			void SetTextureCube(TextureRef texture, uint32_t index) { textureCube[index] = texture; Update(); }
			void SetTexture3D(TextureRef texture, uint32_t index) { texture3D[index] = texture;   Update(); }
			void SetVertexShader(ShaderRef shader) { vertexShader = shader; }
			void SetGeometryShader(ShaderRef shader) { geometryShader = shader; }
			void SetFragmentShader(ShaderRef shader) { fragmentShader = shader; }
			void SetUseNormalTexture(uint32_t use) { useNormalTexture = use;      Update(); }
			inline ShaderRef GetVertexShader() const { return vertexShader; }
			inline ShaderRef GetGeometryShader() const { return geometryShader; }
			inline ShaderRef GetFragmentShader() const { return fragmentShader; }
			// 渲染管线设置
			uint32_t RenderQueue() { return renderQueue; }
			RenderPassMasks RenderPassMask() { return renderPassMask; }
			RasterizerCullMode CullMode() { return cullMode; }
			RasterizerFillMode GetFillMode() { return fillMode; }
			bool DepthTest() { return depthTest; }
			bool DepthWrite() { return depthWrite; }
			CompareFunction DepthCompare() { return depthCompare; }
			bool UseForDepthPass() { return useForDepthPass; }
			bool CastShadow() { return castShadow; }

			void SetRenderQueue(uint32_t queue) { renderQueue = queue; }
			void SetRenderPassMask(RenderPassMasks mask) { renderPassMask = mask; }
			void SetCullMode(RasterizerCullMode cull) { cullMode = cull; }
			void SetFillMode(RasterizerFillMode fill) { fillMode = fill; }
			void SetDepthTest(bool test) { depthTest = test; }
			void SetDepthWrite(bool write) { depthWrite = write; }
			void SetDepthCompare(CompareFunction compare) { depthCompare = compare; }
			void SetUseForDepthPass(bool use) { useForDepthPass = use; }
			void SetCastShadow(bool shadow) { castShadow = shadow; }

			uint32_t materialID;

			glm::vec4 diffuse = glm::vec4(1);
			glm::vec4 emission = glm::vec4(0);

			float roughness = 0.5f;
			float metallic = 0.0f;
			float alphaClip = 0.0f;
			uint32_t useNormalTexture = 1;
			TextureRef textureDiffuse = nullptr;
			TextureRef textureNormal = nullptr;
			TextureRef textureArm = nullptr;
			TextureRef textureSpecular = nullptr;
			TextureRef textureEmission = nullptr;


			std::array<int32_t, 8> ints = { 0 };
			std::array<float, 8> floats = { 0.0f };
			std::array<glm::vec4, 8> colors = { glm::zero<glm::vec4>() };



			std::array<TextureRef, 8> texture2D;
			std::array<TextureRef, 4> textureCube;
			std::array<TextureRef, 4> texture3D;

			ShaderRef vertexShader;                                     // 材质使用的着色器，若为空则可能使用各个pass的默认着色器
			ShaderRef geometryShader;
			ShaderRef fragmentShader;
			MaterialInfo materialInfo;

			



			// 材质系统才是创建不同Pipeline的依据
			uint32_t renderQueue = 1000;                                // 用于指示渲染顺序
			RenderPassMasks renderPassMask = PASS_MASK_DEFERRED_PASS;   // 用于指示和标记特定pass，方便对应的mesh pass收集

			RasterizerCullMode cullMode = CULL_MODE_BACK;               // 剔除模式
			RasterizerFillMode fillMode = FILL_MODE_SOLID;              // 填充模式
			bool depthTest = true;                                      // 深度测试
			bool depthWrite = true;                                     // 深度写入
			CompareFunction depthCompare = COMPARE_FUNCTION_LESS_EQUAL; // 深度测试函数
			// TODO 是否加入混合信息？

			bool useForDepthPass = true;                                // 是否加入深度pass渲染
			bool castShadow = true;                                     // 是否加入阴影pass渲染



		};



	}

	using MaterialRef = std::shared_ptr<V2::Material>;

}

