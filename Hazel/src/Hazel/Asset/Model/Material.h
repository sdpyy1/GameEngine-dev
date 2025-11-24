#pragma once
#include <Hazel/Renderer/old/Shader.h>
#include <Hazel/Renderer/old/Renderer.h>
#include <Hazel/Renderer/old/Texture.h>
namespace GameEngine {
	struct MaterialPush {  // PushRange需要是16字节的整数倍
		glm::vec3 AlbedoColor;
		float Metalness;
		glm::vec3 Emission;
		float Roughness;
		uint32_t UseNormalMap;
		uint32_t padding[3];
	};
	class MaterialOld : public Asset
	{
	public:
		static Ref<MaterialOld> Create(const Ref<Shader>& shader, const std::string& name = "");

		virtual void SetAlbedoTexture(Ref<Texture2D> texture) = 0;
		virtual void SetNormalTexture(Ref<Texture2D> texture) = 0;
		virtual void SetMetalnessTexture(Ref<Texture2D> texture) = 0;
		virtual void SetRoughnessTexture(Ref<Texture2D> texture) = 0;
		virtual void SetEmissionTexture(Ref<Texture2D> texture) = 0;
		virtual void SetAlbedoColor(glm::vec3 color) = 0;
		virtual void SetMetalnessColor(float color) = 0;
		virtual void SetRoughnessColor(float color) = 0;
		virtual void SetEmissionColor(glm::vec3 color) = 0;
		virtual void SetUseNormalTexture(bool value) { bUseNormalTexture = value; }
		Ref<Texture2D> GetAlbedoTexture() { return AlbedoTexture; }
        Ref<Texture2D> GetNormalTexture() { return NormalTexture; }
        Ref<Texture2D> GetMetalnessTexture() { return MetalnessTexture; }
        Ref<Texture2D> GetRoughnessTexture() { return RoughnessTexture; }
        Ref<Texture2D> GetEmissionTexture() { return EmissionTexture; }
		glm::vec3 GetAlbedoColor() { return m_AlbedoColor; }
        glm::vec3 GetEmissionColor() { return m_EmissionColor; }
        float GetRoughnessColor() { return m_RoughnessColor; }
        float GetMetalnessColor() { return m_MetalnessColor; }
        bool GetUseNormalTexture() { return bUseNormalTexture; }


		virtual void SetInput(std::string name, Ref<UniformBufferSet> UboSet) = 0;
		virtual void SetInput(std::string name, Ref<Texture2D> texture, bool isInit = false) = 0;
		virtual void SetInput(std::string name, Ref<StorageBufferSet> SBSet) = 0;
		virtual void SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit = false) = 0;
		virtual void SetInput(std::string name, Ref<ImageView> cubeMap) = 0;
		virtual void SetInput(std::string name, Ref<Image2D> image, bool isInit = false) = 0;

		MaterialPush BuildPush();

	protected:
		glm::vec3 m_AlbedoColor = glm::vec3{ 1,1,1 };
		glm::vec3 m_EmissionColor = glm::vec3{ 0,0,0 };
		float m_MetalnessColor = 0.0f;
		float m_RoughnessColor = 1.0f;
		bool bUseNormalTexture = false;
		Ref<Texture2D> AlbedoTexture = nullptr;
		Ref<Texture2D> NormalTexture = nullptr;
		Ref<Texture2D> MetalnessTexture = nullptr;
		Ref<Texture2D> RoughnessTexture = nullptr;
		Ref<Texture2D> EmissionTexture = nullptr;
	};
}
