#pragma once
#include "VulkanShader.h"
#include "Hazel/Asset/Model/Material.h"
#include "DescriptorManager.h"
namespace GameEngine {
	// 关于材质的资源描述符全部放在Set=1
	class VulkanMaterial : public MaterialOld
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader, const std::string& name = "");
		void CreateDescriptorSet();
		void UpdateDescriptorSet(bool bInit);
		virtual void SetAlbedoTexture(Ref<Texture2D> texture) override;
		virtual void SetNormalTexture(Ref<Texture2D> texture) override;
		virtual void SetMetalnessTexture(Ref<Texture2D> texture) override;
		virtual void SetRoughnessTexture(Ref<Texture2D> texture) override;
		virtual void SetAlbedoColor(glm::vec3 color) override;
		virtual void SetMetalnessColor(float color) override;
		virtual void SetRoughnessColor(float color) override;
		virtual void SetEmissionColor(glm::vec3 color) override;
		virtual void SetEmissionTexture(Ref<Texture2D> texture)override;


		virtual void SetInput(std::string name, Ref<UniformBufferSet> UboSet) override;
		virtual void SetInput(std::string name, Ref<Texture2D> texture, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<StorageBufferSet> SBSet) override;
		virtual void SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<Image2D> image, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<ImageView> image) override;

		VkDescriptorSet RT_GetDescriptorSet() { return m_DescriptorManager.GetDescriptorSet();}

		const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }
		virtual ~VulkanMaterial() override;

	private:
		Ref<VulkanShader> m_Shader;
		std::string m_Name;
		VkDevice  m_Device;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		Buffer m_UniformBuffer;
		DescriptorManager m_DescriptorManager;
	};
}
