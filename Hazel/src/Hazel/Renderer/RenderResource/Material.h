#pragma once
#include "Hazel/Asset/Asset.h"
#include <Hazel/Renderer/RenderResource/Texture.h>
#include "Shader.h"
#include "RenderStruct.h"
namespace GameEngine {

	namespace V2 {

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
			uint32_t materialID;

			glm::vec4 diffuse = glm::vec4(1);
			glm::vec4 emission = glm::vec4(0);

			float roughness = 0.5f;
			float metallic = 0.0f;
			float alphaClip = 0.0f;
			uint32_t useNormalTexture = 0;
			TextureRef textureDiffuse;
			TextureRef textureNormal;
			TextureRef textureArm;
			TextureRef textureSpecular;
			TextureRef textureEmission;


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

		};



	}

	using MaterialRef = std::shared_ptr<V2::Material>;

}

