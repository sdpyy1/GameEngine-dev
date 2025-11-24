#include "hzpch.h"
#include "VulkanRenderer.h"
#include <Hazel/Renderer/old/RendererCapabilities.h>
#include "Vulkan.h"
#include "Hazel/Renderer/old/Renderer.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"
#include <imgui.h>
#include <Hazel/Renderer/old/IndexBuffer.h>
#include "VulkanNativeCall.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanRenderCommandBuffer.h"
#include <glm/gtc/type_ptr.hpp>
#include "VulkanRenderPass.h"
#include "VulkanMaterial.h"
#include "VulkanComputePass.h"
#include "VulkanComputePipeline.h"
#include "Hazel/Platform/Windows/WindowsWindow.h"

namespace GameEngine {
	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;

		Ref<Texture2D> BRDFLut;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> DescriptorPools;
		VkDescriptorPool MaterialDescriptorPool;
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

		// Default samplers
		VkSampler SamplerClamp = nullptr;
		VkSampler SamplerPoint = nullptr;

		int32_t SelectedDrawCall = -1;
		int32_t DrawCallCount = 0;
	};
	static VulkanRendererData* s_Data = nullptr;

	namespace Utils {
		static const char* VulkanVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
			case 0x10DE: return "NVIDIA";
			case 0x1002: return "AMD";
			case 0x8086: return "INTEL";
			case 0x13B5: return "ARM";
			}
			return "Unknown";
		}
	}
	VkDescriptorSet VulkanRenderer::RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
	{
		uint32_t bufferIndex = Renderer::RT_GetCurrentFrameIndex();
		allocInfo.descriptorPool = s_Data->DescriptorPools[bufferIndex];
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VkDescriptorSet result;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result));
		s_Data->DescriptorPoolAllocationCount[bufferIndex] += allocInfo.descriptorSetCount;
		return result;
	}
	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();
		const auto& config = Renderer::GetConfig();
		s_Data->DescriptorPools.resize(config.FramesInFlight);
		s_Data->DescriptorPoolAllocationCount.resize(config.FramesInFlight);
		auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);
		LOG_INFO_TAG("Renderer", "GPU info:");
		LOG_INFO_TAG("Renderer", "  Vendor: {0}", caps.Vendor);
		LOG_INFO_TAG("Renderer", "  Device: {0}", caps.Device);
		LOG_INFO_TAG("Renderer", "  Version: {0}", caps.Version);
		RENDER_SUBMIT([]() mutable
			{
				VkDescriptorPoolSize pool_sizes[] =
				{
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
				};
				VkDescriptorPoolCreateInfo pool_info = {};
				pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				pool_info.maxSets = 100000;
				pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
				pool_info.pPoolSizes = pool_sizes;
				VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t i = 0; i < framesInFlight; i++)
				{
					VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->DescriptorPools[i]));
					s_Data->DescriptorPoolAllocationCount[i] = 0;
				}

				VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->MaterialDescriptorPool));
			});
	}
	void VulkanRenderer::Shutdown()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		if (s_Data->SamplerPoint)
		{
			Vulkan::DestroySampler(s_Data->SamplerPoint);
			s_Data->SamplerPoint = nullptr;
		}

		if (s_Data->SamplerClamp)
		{
			Vulkan::DestroySampler(s_Data->SamplerClamp);
			s_Data->SamplerClamp = nullptr;
		}

#if HZ_HAS_SHADER_COMPILER
		VulkanShaderCompiler::ClearUniformBuffers();
#endif
		delete s_Data;
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}
	void VulkanRenderer::BeginFrame()
	{
		RENDER_SUBMIT([]()
			{
				VulkanSwapChain& swapChain = Application::Get().GetWindow()->GetSwapChain();
				swapChain.BeginFrame();
				VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				uint32_t bufferIndex = swapChain.GetCurrentBufferIndex();
				//
				vkResetDescriptorPool(device, s_Data->DescriptorPools[bufferIndex], 0);
				memset(s_Data->DescriptorPoolAllocationCount.data(), 0, s_Data->DescriptorPoolAllocationCount.size() * sizeof(uint32_t));

				s_Data->DrawCallCount = 0;
			});
	}
	void VulkanRenderer::BindVertData(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> testVertexBuffer) {
		RENDER_SUBMIT([commandBuffer, testVertexBuffer] {
			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer vkCommandBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
			VkDeviceSize offsets[] = { 0 };
			VkBuffer vertexBuffers[] = { testVertexBuffer.As<VulkanVertexBuffer>()->GetVulkanBuffer() };
			vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
			});
	}
	void VulkanRenderer::DrawPrueVertex(Ref<RenderCommandBuffer> commandBuffer, uint32_t count)
	{
		RENDER_SUBMIT([commandBuffer, count] {
			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer vkCommandBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
			vkCmdDraw(vkCommandBuffer, count, 1, 0, 0);
			});
	}
	void VulkanRenderer::EndFrame()
	{
		RENDER_SUBMIT([]() {
			Ref<WindowsWindow> m_GLFWWindow = Application::Get().GetWindow();
			m_GLFWWindow->SwapBuffers();
			});
#if 0

		RENDER_SUBMIT([]()
			{
				VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
				commandBuffer = nullptr;
			});
#endif
	}

	void VulkanRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		RENDER_SUBMIT([renderCommandBuffer, renderPass, explicitClear]()
			{
				//LOG_TRACE(" Pass Begin [{}]", renderPass->GetSpecification().DebugName);
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

				VkDebugUtilsLabelEXT debugLabel{};
				debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				memcpy(&debugLabel.color, glm::value_ptr(renderPass->GetSpecification().MarkerColor), sizeof(float) * 4);
				debugLabel.pLabelName = renderPass->GetSpecification().DebugName.c_str();
				fpCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);

				auto fb = renderPass->GetSpecification().Pipeline->GetSpecification().TargetFramebuffer;
				Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
				const auto& fbSpec = framebuffer->GetSpecification();

				uint32_t width = framebuffer->GetWidth();
				uint32_t height = framebuffer->GetHeight();

				VkViewport viewport = {};
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.pNext = nullptr;
				renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				if (framebuffer->GetSpecification().SwapChainTarget)
				{
					VulkanSwapChain& swapChain = Application::Get().GetWindow()->GetSwapChain();
					width = swapChain.GetWidth();
					height = swapChain.GetHeight();
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.pNext = nullptr;
					renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
					renderPassBeginInfo.renderArea.offset.x = 0;
					renderPassBeginInfo.renderArea.offset.y = 0;
					renderPassBeginInfo.renderArea.extent.width = width;
					renderPassBeginInfo.renderArea.extent.height = height;
					renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

					viewport.x = 0.0f;
					viewport.y = (float)height;
					viewport.width = (float)width;
					viewport.height = -(float)height;
				}
				else
				{
					width = framebuffer->GetWidth();
					height = framebuffer->GetHeight();
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.pNext = nullptr;
					renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
					renderPassBeginInfo.renderArea.offset.x = 0;
					renderPassBeginInfo.renderArea.offset.y = 0;
					renderPassBeginInfo.renderArea.extent.width = width;
					renderPassBeginInfo.renderArea.extent.height = height;
					renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)width;
					viewport.height = (float)height;
				}

				// TODO: Does our framebuffer have a depth attachment?
				const auto& clearValues = framebuffer->GetVulkanClearValues();
				renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
				renderPassBeginInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				if (explicitClear)
				{
					const uint32_t colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentCount();
					const uint32_t totalAttachmentCount = colorAttachmentCount + (framebuffer->HasDepthAttachment() ? 1 : 0);
					ASSERT(clearValues.size() == totalAttachmentCount);

					std::vector<VkClearAttachment> attachments(totalAttachmentCount);
					std::vector<VkClearRect> clearRects(totalAttachmentCount);
					for (uint32_t i = 0; i < colorAttachmentCount; i++)
					{
						attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						attachments[i].colorAttachment = i;
						attachments[i].clearValue = clearValues[i];

						clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
						clearRects[i].rect.extent = { width, height };
						clearRects[i].baseArrayLayer = 0;
						clearRects[i].layerCount = 1;
					}

					if (framebuffer->HasDepthAttachment())
					{
						attachments[colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/;
						attachments[colorAttachmentCount].clearValue = clearValues[colorAttachmentCount];
						clearRects[colorAttachmentCount].rect.offset = { (int32_t)0, (int32_t)0 };
						clearRects[colorAttachmentCount].rect.extent = { width, height };
						clearRects[colorAttachmentCount].baseArrayLayer = 0;
						clearRects[colorAttachmentCount].layerCount = 1;
					}

					vkCmdClearAttachments(commandBuffer, totalAttachmentCount, attachments.data(), totalAttachmentCount, clearRects.data());
				}

				// Update dynamic viewport state
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

				// Update dynamic scissor state
				VkRect2D scissor = {};
				scissor.extent.width = width;
				scissor.extent.height = height;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

				// TODO: automatic layout transitions for input resources

				// Bind Vulkan Pipeline
				Ref<VulkanPipeline> vulkanPipeline = renderPass->GetSpecification().Pipeline.As<VulkanPipeline>();
				VkPipeline vPipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vPipeline);

				if (vulkanPipeline->IsDynamicLineWidth())
					vkCmdSetLineWidth(commandBuffer, vulkanPipeline->GetSpecification().LineWidth);

				// Bind input descriptors
				if (!renderPass->GetPipeline()->GetShader().As<VulkanShader>()->GetDescriptorSet().empty()) {
					Ref<VulkanRenderPass> vulkanRenderPass = renderPass.As<VulkanRenderPass>();
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass->GetPipeline().As<VulkanPipeline>()->GetVulkanPipelineLayout(), 0, 1, &renderPass->GetPipeline()->GetShader().As<VulkanShader>()->GetDescriptorSet()[frameIndex], 0, nullptr);

				}
			});
	}
	void VulkanRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		RENDER_SUBMIT([renderCommandBuffer]()
			{
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

				vkCmdEndRenderPass(commandBuffer);
				fpCmdEndDebugUtilsLabelEXT(commandBuffer);
			});
	}
	void VulkanRenderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialOld> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		ASSERT(meshSource);
		Buffer pushConstantBuffer;
		if (additionalUniforms.Size)
		{
			pushConstantBuffer.Allocate(additionalUniforms.Size);
			pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size);
		}
		RENDER_SUBMIT([renderCommandBuffer, pipeline, meshSource, submeshIndex, material, pushConstantBuffer, transformBuffer, transformOffset, instanceCount]() mutable {
			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
			Ref<VulkanVertexBuffer> meshVertBuffer = meshSource->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vkMeshVertBuffer = meshVertBuffer->GetVulkanBuffer();
			VkDeviceSize vertexOffsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkMeshVertBuffer, vertexOffsets);  // ������Mesh�Ķ��㶼��
			Ref<VulkanVertexBuffer> vulkanTransformBuffer = transformBuffer.As<VulkanVertexBuffer>();
			VkBuffer vkTransformBuffer = vulkanTransformBuffer->GetVulkanBuffer();
			VkDeviceSize instanceOffsets[1] = { transformOffset };
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &vkTransformBuffer, instanceOffsets); // �ڶ������㻺�����󶨵�ǰSubMesh�ı任��������

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(meshSource->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32); // ����������ȫ��
			VkPipelineLayout layout = pipeline.As<VulkanPipeline>()->GetVulkanPipelineLayout();
			uint32_t pushConstantOffset = 0;
			if (material) {
				Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
				VkDescriptorSet matSet = vulkanMaterial->GetDescriptorSets()[frameIndex];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline.As<VulkanPipeline>()->GetVulkanPipelineLayout(),
					1, // Set=1
					1, &matSet,
					0, nullptr);

				Buffer mB = Buffer(&material->BuildPush(), sizeof(MaterialPush));
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantOffset, mB.Size, mB.Data);
				pushConstantOffset += mB.Size;
			}
			const auto& submeshes = meshSource->GetSubmeshes();
			const auto& submesh = submeshes[submeshIndex];


			if (pushConstantBuffer.Size > 0)
			{
				// LOG_INFO("Pass{}", pipeline->GetSpecification().DebugName);
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, pushConstantOffset, pushConstantBuffer.Size, pushConstantBuffer.Data);
			}

			vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);
			});
	}

	void VulkanRenderer::RenderSkeletonMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialOld> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		VERIFY(meshSource);

		Buffer pushConstantBuffer;
		bool isRigged = meshSource->IsSubmeshRigged(submeshIndex);

		if (additionalUniforms.Size || isRigged)
		{
			pushConstantBuffer.Allocate(additionalUniforms.Size + (isRigged ? sizeof(uint32_t) : 0));
			if (additionalUniforms.Size)
				pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size);

			if (isRigged)
				pushConstantBuffer.Write(&boneTransformsOffset, sizeof(uint32_t), additionalUniforms.Size);
		}

		RENDER_SUBMIT([additionalUniforms, renderCommandBuffer, pipeline, meshSource, submeshIndex, material, transformBuffer, transformOffset, instanceCount, pushConstantBuffer]() mutable
			{
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
				VkBuffer meshVB = meshSource->GetVertexBuffer().As<VulkanVertexBuffer>()->GetVulkanBuffer();
				VkDeviceSize vertexOffsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshVB, vertexOffsets);

				VkBuffer transformVB = transformBuffer.As<VulkanVertexBuffer>()->GetVulkanBuffer();
				VkDeviceSize instanceOffsets[1] = { transformOffset };
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &transformVB, instanceOffsets);

				VkBuffer meshIB = meshSource->GetIndexBuffer().As<VulkanIndexBuffer>()->GetVulkanBuffer();
				vkCmdBindIndexBuffer(commandBuffer, meshIB, 0, VK_INDEX_TYPE_UINT32);

				//RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

				const auto& submeshes = meshSource->GetSubmeshes();
				const auto& submesh = submeshes[submeshIndex];

				if (submesh.IsRigged)
				{
					Ref<VulkanVertexBuffer> vulkanBoneInfluencesVB = meshSource->GetBoneInfluenceBuffer().As<VulkanVertexBuffer>();
					VkBuffer vbBoneInfluencesBuffer = vulkanBoneInfluencesVB->GetVulkanBuffer();
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &vbBoneInfluencesBuffer, vertexOffsets);
				}
				uint32_t pushConstantOffset = 0;
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

				if (material) {
					Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();

					VkDescriptorSet matSet = vulkanMaterial->GetDescriptorSets()[frameIndex];
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline.As<VulkanPipeline>()->GetVulkanPipelineLayout(),
						1, // Set=1
						1, &matSet,
						0, nullptr);

					Buffer mB = Buffer(&material->BuildPush(), sizeof(MaterialPush));

					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantOffset, mB.Size, mB.Data);
					pushConstantOffset += mB.Size;
				}

				if (pushConstantBuffer.Size > 0)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, pushConstantOffset, pushConstantBuffer.Size, pushConstantBuffer.Data);
				}



				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);

				pushConstantBuffer.Release();
			});
	}

	void VulkanRenderer::BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		RENDER_SUBMIT([renderCommandBuffer, computePass]() mutable {
			Ref<VulkanComputePass> vulkanComputePass = computePass.As<VulkanComputePass>();
			Ref<VulkanComputePipeline> pipeline = computePass->GetSpecification().Pipeline.As<VulkanComputePipeline>();
			const uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
			VkDebugUtilsLabelEXT debugLabel{};
			debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			memcpy(&debugLabel.color, glm::value_ptr(vulkanComputePass->GetSpecification().MarkerColor), sizeof(float) * 4);
			debugLabel.pLabelName = vulkanComputePass->GetSpecification().DebugName.c_str();
			fpCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);
			pipeline->RT_Begin(renderCommandBuffer); // bind pipeline
			});
	}

	void VulkanRenderer::EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		RENDER_SUBMIT([renderCommandBuffer, computePass]() mutable
			{
				Ref<VulkanComputePass> vulkanComputePass = computePass.As<VulkanComputePass>();
				Ref<VulkanComputePipeline> pipeline = computePass->GetSpecification().Pipeline.As<VulkanComputePipeline>();
				pipeline->End();
				fpCmdEndDebugUtilsLabelEXT(renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer());
			});
	}

	void VulkanRenderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<MaterialOld> material, const glm::uvec3& workGroups, Buffer constants)
	{
		Buffer constantsBuffer;
		if (constants)
			constantsBuffer = Buffer::Copy(constants);

		RENDER_SUBMIT([renderCommandBuffer, computePass, material, workGroups, constantsBuffer]() mutable
			{
				Ref<VulkanComputePass> vulkanComputePass = computePass.As<VulkanComputePass>();
				Ref<VulkanComputePipeline> pipeline = computePass->GetSpecification().Pipeline.As<VulkanComputePipeline>();

				const uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				// Bind material descriptor set if exists
				auto descriptorSet = computePass->GetSpecification().Pipeline.As<VulkanComputePipeline>()->GetShader().As<VulkanShader>()->GetDescriptorSet();
				if (!descriptorSet.empty())
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet[frameIndex], 0, nullptr);
				// Bind material descriptor set if exists
				if (material)
				{
					Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
					VkDescriptorSet descriptorSet = vulkanMaterial->RT_GetDescriptorSet();
					if (descriptorSet)
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 1, 1, &descriptorSet, 0, nullptr);
				}
				if (constantsBuffer)
				{
					pipeline->SetPushConstants(constantsBuffer);
					constantsBuffer.Release();
				}

				pipeline->Dispatch(workGroups);
			});
	}

	void VulkanRenderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<MaterialOld> material, const glm::uvec3& workGroups, uint32_t descriptorIndex, Buffer constants)
	{
		Buffer constantsBuffer;
		if (constants)
			constantsBuffer = Buffer::Copy(constants);

		RENDER_SUBMIT([renderCommandBuffer, computePass, material, workGroups, constantsBuffer, descriptorIndex]() mutable
			{
				Ref<VulkanComputePass> vulkanComputePass = computePass.As<VulkanComputePass>();
				Ref<VulkanComputePipeline> pipeline = computePass->GetSpecification().Pipeline.As<VulkanComputePipeline>();

				const uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				// Bind material descriptor set if exists
				auto descriptorSet = vulkanComputePass->GetMoreDescriptorSet(descriptorIndex);
				if (descriptorSet)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);

				if (constantsBuffer)
				{
					pipeline->SetPushConstants(constantsBuffer);
					constantsBuffer.Release();
				}

				pipeline->Dispatch(workGroups);
			});
	}

	namespace Utils {
		void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}
	}
}