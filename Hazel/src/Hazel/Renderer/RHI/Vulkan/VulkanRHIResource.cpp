#include "hzpch.h"
#include "VulkanRHIResource.h"
#include "VulkanRHI.h"
#include "VulkanUtil.h"
#include <regex>
#include "spirv_reflect.h"
#include "Hazel/Renderer/RHI/RHICommandList.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace GameEngine
{
	VulkanRHISurface::VulkanRHISurface(GLFWwindow* window)
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		extent = { (uint32_t)width, (uint32_t)height };
		VK_CHECK_RESULT(glfwCreateWindowSurface(VULKAN_INSTANCE, window, nullptr, &handle));
	}

	void VulkanRHISurface::Destroy()
	{
		vkDestroySurfaceKHR(VULKAN_INSTANCE, handle, nullptr);
	}

	VulkanRHISwapchain::VulkanRHISwapchain(const RHISwapchainInfo& info) : RHISwapchain(info)
	{
		VkPhysicalDevice device = VULKAN_PHYSICALDEVICE;
		VkDevice logicalDevice = VULKAN_DEVICE;
		VkSurfaceKHR surface = std::static_pointer_cast<VulkanRHISurface>(info.surface)->GetHandle();

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

		// 获取设备支持的图片格式，最终呈现的VkImage支持的格式
		uint32_t size;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &size, nullptr);
		availableFormats.resize(size);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &size, availableFormats.data());

		// 支持的呈现模式
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &size, nullptr);
		availablePresentModes.resize(size);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &size, availablePresentModes.data());

		// 交换链基本信息
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(VulkanUtil::RHIFormatToVkFormat(info.format));
		RHIFormat targetFormat = VulkanUtil::VkFormatToRHIFormat(surfaceFormat.format);
		if (targetFormat != info.format)
		{
			this->info.format = targetFormat;
			LOG_ERROR("Cant find swapchain image format support!");
		}

		// 呈现模式
		VkPresentModeKHR presentMode = ChooseSwapPresentMode();

		// Extent
		VkExtent2D extent = ChooseSwapExtent();
		if (extent.width != info.extent.width || extent.height != info.extent.height)
		{
			this->info.extent = { extent.width, extent.height };
			LOG_ERROR("Cant find suitable swapchain image extent!");
		}

		// 交换链图像数目
		uint32_t imageCount = std::max(info.imageCount, capabilities.minImageCount);
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}
		if (info.imageCount != imageCount)
		{
			this->info.imageCount = capabilities.maxImageCount;
			LOG_ERROR("Swapchain image count is greater than capability maximum!");
		}

		// 创建交换链信息
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // 因为要copy到它上边，所以加VK_IMAGE_USAGE_TRANSFER_DST_BIT

		// 检测队列族对交换链图像的操作方式
		/*
		uint32_t queueFamilyIndices[] = { (uint32_t)queueInfo.graphicsFamily, (uint32_t)queueInfo.presentFamily };
		if (queueInfo.graphicsFamily != queueInfo.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   // 图像可被多个队列族访问
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;    // 图像同一时间只能被单个队列族访问
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
		*/
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;    // 图像同一时间只能被单个队列族访问
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional

		createInfo.preTransform = capabilities.currentTransform;                    // 变换操作，例如旋转反转，用默认
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;              // 透明度混合
		createInfo.presentMode = presentMode;                                       // 刷新模式
		createInfo.clipped = VK_TRUE;                                               // 裁剪（遮挡或再可视范围外等）
		createInfo.oldSwapchain = VK_NULL_HANDLE;                                   // 交换链更新时使用

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, nullptr);   // 获取image句柄
		images.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, images.data());

		imageFormat = surfaceFormat.format;  // 存储extent和format
		imageExtent = extent;
		RHI_DYNAMICRHI->GetImmediateCommandList();
		for (uint32_t i = 0; i < imageCount; i++)
		{
			RHITextureInfo info = {};
			info.format = targetFormat;
			info.extent = { extent.width, extent.height, 1 };
			info.arrayLayers = 1;
			info.mipLevels = 1;
			info.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			info.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RENDER_TARGET;
			info.creationFlag = TEXTURE_CREATION_NONE;
			RHITextureRef texture = std::make_shared<VulkanRHITexture>(info, images[i]);
			textures.push_back(texture);

			// 修改布局
			RHI_DYNAMICRHI->GetImmediateCommandList()->TextureBarrier({ texture, RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_PRESENT,{ TEXTURE_ASPECT_COLOR, 0, 1, 0, 1 } });
		}
		RHI_DYNAMICRHI->GetImmediateCommandList()->Flush();
	}

	GameEngine::RHITextureRef VulkanRHISwapchain::GetNewFrame(RHIFenceRef fence, RHISemaphoreRef signalSemaphore)
	{
		VkFence signalFence = VK_NULL_HANDLE;
		VkSemaphore semaphore = VK_NULL_HANDLE;

		if (fence != nullptr) signalFence = std::static_pointer_cast<VulkanRHIFence>(fence)->GetHandle();
		if (signalSemaphore != nullptr) semaphore = std::static_pointer_cast<VulkanRHISemaphore>(signalSemaphore)->GetHandle();

		VkResult result = vkAcquireNextImageKHR(VULKAN_DEVICE, handle, UINT64_MAX, semaphore, signalFence, &currentIndex);

		return textures[currentIndex];
	}

	// SRGB空间：https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
	/*
		理清楚：伽马矫正和SRGB
		伽马矫正：pow(1/2.2) 用于抵消显示器的pow(2.2),让显示器最终显示的内容就是我想要输出的内容
		SRGB空间： 它不仅考虑到了伽马矫正，而且考虑到了人眼对暗部更敏感，所以线性处理完后，转到SRGB空间后不仅会抵消显示器的pow(2.2)并且会把线性颜色转为适合人眼的非线性颜色分布
	*/
	VkSurfaceFormatKHR VulkanRHISwapchain::ChooseSwapSurfaceFormat(VkFormat targetFormat)
	{
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		// 交换链的VkImage色彩空间应该选择SRGB的，自己在后处理时把线性转为SRGB再传递给交换链的VkImage  Vulkan官方教程中选择的是VK_FORMAT_B8G8R8A8_SRGB
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == targetFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];  // 默认就是 return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	VkPresentModeKHR VulkanRHISwapchain::ChooseSwapPresentMode()
	{
		// 选择刷新模式
	   /* std::cout << "Available swapchain present modes:" << std::endl;
		for (const auto mode : availablePresentModes) {
			std::cout << mode << std::endl;
		}
		std::cout << " " << std::endl;*/

		VkPresentModeKHR bestMode;
		//bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;         // normal
		//bestMode = VK_PRESENT_MODE_MAILBOX_KHR;             // low latency
		//bestMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;      // minimize stuttering
		//bestMode = VK_PRESENT_MODE_FIFO_KHR;              // low power consumption

		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	VkExtent2D VulkanRHISwapchain::ChooseSwapExtent()
	{
		// 选择分辨率
		//std::cout << "Swapchain extent: " << capabilities.currentExtent.width << " : " << capabilities.currentExtent.height << std::endl;

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else {
			//int width, height;
			//glfwGetWindowSize(pWindow, &width, &height);
			VkExtent2D actualExtent = { this->info.extent.width, this->info.extent.height };

			//std::cout << width << " " << height << std::endl;

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void VulkanRHISwapchain::Present(RHISemaphoreRef waitSemaphore)
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		if (waitSemaphore != nullptr) semaphore = std::static_pointer_cast<VulkanRHISemaphore>(waitSemaphore)->GetHandle();

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &handle;
		presentInfo.pImageIndices = &currentIndex;
		presentInfo.pResults = nullptr;
		presentInfo.waitSemaphoreCount = semaphore == VK_NULL_HANDLE ? 0 : 1;
		presentInfo.pWaitSemaphores = &semaphore;
		VkResult result = vkQueuePresentKHR(std::static_pointer_cast<VulkanRHIQueue>(this->info.presentQueue)->GetHandle(), &presentInfo);
		if (result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				Resize();
			}
			else
			{
				VK_CHECK_RESULT(result);
			}
		}
	}

	void VulkanRHISwapchain::Destroy()
	{
		vkDestroySwapchainKHR(VULKAN_DEVICE, handle, nullptr);
	}

	void VulkanRHISwapchain::Resize()
	{
		// 1. 等待设备空闲，确保旧Swapchain资源不再被使用
		vkDeviceWaitIdle(VULKAN_DEVICE);

		VkSurfaceKHR surface = std::static_pointer_cast<VulkanRHISurface>(info.surface)->GetHandle();

		// 2. 销毁旧Swapchain相关资源
		// 销毁纹理对象（如果纹理由Swapchain管理）
		textures.clear();
		// 销毁旧Swapchain句柄
		if (handle != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(VULKAN_DEVICE, handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
		VkPhysicalDevice device = VULKAN_PHYSICALDEVICE;
		VkDevice logicalDevice = VULKAN_DEVICE;
		// 3. 重新查询Surface支持信息（窗口Resize后可能变化）
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

		uint32_t size = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &size, nullptr);
		availableFormats.resize(size);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &size, availableFormats.data());

		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &size, nullptr);
		availablePresentModes.resize(size);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &size, availablePresentModes.data());

		// 4. 重新计算Swapchain参数（与构造函数逻辑一致，但使用新窗口尺寸）
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(VulkanUtil::RHIFormatToVkFormat(this->info.format));
		RHIFormat targetFormat = VulkanUtil::VkFormatToRHIFormat(surfaceFormat.format);
		if (targetFormat != this->info.format)
		{
			this->info.format = targetFormat;
			LOG_ERROR("Swapchain format adjusted to supported format");
		}

		VkPresentModeKHR presentMode = ChooseSwapPresentMode();
		VkExtent2D newExtent = ChooseSwapExtent(); // 自动适配新窗口尺寸
		this->info.extent = { newExtent.width, newExtent.height }; // 更新info中的尺寸

		// 图像数量（保持与原逻辑一致）
		uint32_t imageCount = std::max(this->info.imageCount, capabilities.minImageCount);
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}
		this->info.imageCount = imageCount;

		// 5. 重新创建Swapchain
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = newExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT;

		// 图像共享模式（保持原逻辑）
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE; // 旧Swapchain已销毁

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to recreate Swapchain!");
		}

		// 6. 重新获取新Swapchain的图像和纹理
		vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, nullptr);
		images.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, images.data());

		// 更新图像格式和尺寸
		imageFormat = surfaceFormat.format;
		imageExtent = newExtent;
		info.extent = { newExtent.width, newExtent.height };
		// 重新创建纹理对象（与构造函数逻辑一致）
		for (uint32_t i = 0; i < imageCount; i++)
		{
			RHITextureInfo textureInfo;
			textureInfo.format = targetFormat;
			textureInfo.extent = { newExtent.width, newExtent.height, 1 };
			textureInfo.arrayLayers = 1;
			textureInfo.mipLevels = 1;
			textureInfo.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			textureInfo.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RENDER_TARGET;
			textureInfo.creationFlag = TEXTURE_CREATION_NONE;

			RHITextureRef texture = std::make_shared<VulkanRHITexture>(textureInfo, images[i]);
			textures.push_back(texture);

			// 纹理屏障：从UNDEFINED过渡到PRESENT状态
			RHI_DYNAMICRHI->GetImmediateCommandList()->TextureBarrier({
				texture,
				RESOURCE_STATE_UNDEFINED,
				RESOURCE_STATE_PRESENT,
				{ TEXTURE_ASPECT_COLOR, 0, 1, 0, 1 }
				});
			RHI_DYNAMICRHI->GetImmediateCommandList()->Flush();
		}

		currentIndex = 0;
		LOG_INFO("Swapchain recreated successfully! New extent: ({}, {})", newExtent.width, newExtent.height);
	}

	VulkanRHITexture::VulkanRHITexture(const RHITextureInfo& info, VkImage image) : RHITexture(info)
	{
		/*
			TextureAspectFlags: 颜色、深度、模板、深度 + 模板
			VkFormat: SRGBA URGBA....
			VkImageUsageFlags：默认都加了 SRC、DST ,根据ResourceType添加SAMPLED、STORAGE、（COLOR_ATTACHMENT/DEPTH_STENCIL_ATTACHMENT）
			VkImageType：1D、2D、3D （creationFlag用来强制）
			VkImageCreateFlags：Cube的话需要添加（还有很多其他功能）

			创建后布局为 UNDEFINED
		*/
		// 创建默认的View参数
		ASSERT(info.mipLevels > 0, "RHI层不会自动Mip！！！");
		ASSERT(info.arrayLayers > 0, "layer没传？");
		TextureAspectFlags aspects = IsDepthStencilFormat(info.format) ? TEXTURE_ASPECT_DEPTH_STENCIL : IsDepthFormat(info.format) ? TEXTURE_ASPECT_DEPTH : IsStencilFormat(info.format) ? TEXTURE_ASPECT_STENCIL : TEXTURE_ASPECT_COLOR;
		defaultRange = { aspects, 0, info.mipLevels, 0, info.arrayLayers };
		defaultLayers = { aspects, 0, 0, info.arrayLayers };

		if (image != VK_NULL_HANDLE)
		{
			handle = image;
			return;
		}

		VkFormat format = VulkanUtil::RHIFormatToVkFormat(info.format);

		VkImageUsageFlags usage = VulkanUtil::ResourceTypeToImageUsage(info.type);
		if (IsDepthFormat(info.format) || IsStencilFormat(info.format))
			usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		else if (info.type & RESOURCE_TYPE_RENDER_TARGET)
			usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageType type = info.extent.depth > 1 ? VK_IMAGE_TYPE_3D : info.extent.height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
		if (info.creationFlag & TEXTURE_CREATION_FORCE_2D) type = VK_IMAGE_TYPE_2D;
		if (info.creationFlag & TEXTURE_CREATION_FORCE_3D) type = VK_IMAGE_TYPE_3D;

		VkImageCreateFlags flag = 0;
		if (info.type & RESOURCE_TYPE_TEXTURE_CUBE)      flag |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		if (type & VK_IMAGE_TYPE_3D)                    flag |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;   // 运行按照2D数组来处理3D纹理（不然就需要创建3DImageView来使用，其实一直用的都是这种）

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = type;
		imageInfo.extent.width = info.extent.width;
		imageInfo.extent.height = info.extent.height;
		imageInfo.extent.depth = info.extent.depth;
		imageInfo.mipLevels = info.mipLevels;
		imageInfo.arrayLayers = info.arrayLayers;
		if (info.type & RESOURCE_TYPE_TEXTURE_CUBE) imageInfo.arrayLayers = std::max(imageInfo.arrayLayers, (uint32_t)6);
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // 物理布局方式
		imageInfo.usage = usage;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;   // 逻辑状态 / 访问规则的标记 不影响数据本身，但是会影响Vulkan如何使用它（只能undefined，直接指定shaderread就报错了）
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // 某个队列族独占
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // MSAA
		imageInfo.flags = flag; // Optional

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VulkanUtil::MemoryUsageToVma(info.memoryUsage);
		allocationInfo = {};
		if (vmaCreateImage(VULKAN_VMA, &imageInfo, &allocationCreateInfo, &handle, &allocation, &allocationInfo) != VK_SUCCESS)
		{
			LOG_ERROR("VMA failed to allocate image!");
		}
	}

	void VulkanRHITexture::Destroy()
	{
		vmaDestroyImage(VULKAN_VMA, handle, allocation);
	}

	VulkanRHIFence::VulkanRHIFence(bool signaled)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		vkCreateFence(VULKAN_DEVICE, &fenceInfo, nullptr, &handle);
	}

	void VulkanRHIFence::Wait()
	{
		vkWaitForFences(VULKAN_DEVICE, 1, &handle, VK_TRUE, UINT64_MAX);    //TODO 设置超时时间
		vkResetFences(VULKAN_DEVICE, 1, &handle);
	}

	void VulkanRHIFence::Destroy()
	{
		vkDestroyFence(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHISemaphore::VulkanRHISemaphore()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		vkCreateSemaphore(VULKAN_DEVICE, &semaphoreInfo, nullptr, &handle);
	}

	void VulkanRHISemaphore::Destroy()
	{
		vkDestroySemaphore(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHICommandPool::VulkanRHICommandPool(const RHICommandPoolInfo& info)
		: RHICommandPool(info)
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = CAST<VulkanRHIQueue>(info.queue)->GetQueueFamilyIndex();  //命令池需要绑定队列族，使用其指定的命令类型
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		if (vkCreateCommandPool(VULKAN_DEVICE, &poolInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create command pool!");
		}
	}

	void VulkanRHICommandPool::Destroy()
	{
		vkDestroyCommandPool(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHITextureView::VulkanRHITextureView(const RHITextureViewInfo& info) : RHITextureView(info)
	{
		if (info.subresource.aspect == TEXTURE_ASPECT_NONE)  this->info.subresource = info.texture->GetDefaultSubresourceRange();

		RHITextureInfo textureInfo = CAST<VulkanRHITexture>(info.texture)->GetInfo();
		VkImageAspectFlags aspectMask = VulkanUtil::TextureAspectToVk(this->info.subresource.aspect);

		// if(IsDepthStencilFormat(info.format))
		// {
		//     aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		//     if(IsStencilFormat(info.format))        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		// }
		// else
		// {
		//     if(textureInfo.type & RESOURCE_TYPE_TEXTURE)    aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		//     if(textureInfo.type & RESOURCE_TYPE_RW_TEXTURE) aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		// }

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = CAST<VulkanRHITexture>(info.texture)->GetHandle();
		viewInfo.viewType = VulkanUtil::TextureViewTypeToVk(info.viewType);
		viewInfo.format = VulkanUtil::RHIFormatToVkFormat(info.format);
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = aspectMask;
		viewInfo.subresourceRange.baseArrayLayer = this->info.subresource.baseArrayLayer;
		viewInfo.subresourceRange.baseMipLevel = this->info.subresource.baseMipLevel;
		viewInfo.subresourceRange.layerCount = this->info.subresource.layerCount;
		viewInfo.subresourceRange.levelCount = this->info.subresource.levelCount;

		if (vkCreateImageView(VULKAN_DEVICE, &viewInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create texture image view!");
		}
	}

	void VulkanRHITextureView::Destroy()
	{
		vkDestroyImageView(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHISampler::VulkanRHISampler(const RHISamplerInfo& info) : RHISampler(info)
	{
		/*
			magFilter：放大滤波，当纹理分辨率小，放大观看 较近时使用
			minFilter：缩小滤波，当纹理分辨率大，缩小观看 较远时使用
			mipmapMode：mipmap模式，可以选择线性插值还是最近邻插值
			anisotropyEnable：各向异性过滤启用，用于改善纹理在倾斜视角下的拉伸和模糊问题
			maxAnisotropy：设置各向异性过滤的强度，值越高，质量越好，但性能开销越大
			minLod：mipmap 最小层级
			maxLod：mipmap 最大层级
			mipLodBias：调整 mipmap 层级的选择，让采样器偏向使用更高或更低分辨率的 mipmap
			addressModeU / addressModeV / addressModeW： 定义当纹理坐标（UVW）超出 [0.0, 1.0] 范围时，如何处理采样
			borderColor：当上边超出UVW设置为clamp_to_edge时，这就是边界颜色
			compareEnable/compareOp：启用比较模式后，采样器会将采样得到的纹理值与一个参考值进行比较，在Shader中采样时使用，但是能力有限，只能判断比较结果
			Reduction：本来采样是插值，这个拓展可以改成返回最大值或最小值
		*/
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter = VulkanUtil::FilterTypeToVk(info.magFilter);
		samplerInfo.minFilter = VulkanUtil::FilterTypeToVk(info.minFilter);
		samplerInfo.mipmapMode = VulkanUtil::MipMapModeToVk(info.mipmapMode);
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 100.0f;
		samplerInfo.mipLodBias = info.mipLodBias;
		samplerInfo.addressModeU = VulkanUtil::AddressModeToVk(info.addressModeU);
		samplerInfo.addressModeV = VulkanUtil::AddressModeToVk(info.addressModeV);
		samplerInfo.addressModeW = VulkanUtil::AddressModeToVk(info.addressModeW);
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		samplerInfo.compareEnable = (info.compareFunction != CompareFunction::COMPARE_FUNCTION_NEVER);
		samplerInfo.compareOp = VulkanUtil::CompareFunctionToVk(info.compareFunction);

		// 各向异性需要设备支持
		if (info.maxAnisotropy > 0.0f) {
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(VULKAN_PHYSICALDEVICE, &properties);

			float deviceMaxAnisotropy = properties.limits.maxSamplerAnisotropy;

			if (deviceMaxAnisotropy > 1.0f) {
				samplerInfo.maxAnisotropy = std::min(info.maxAnisotropy, deviceMaxAnisotropy);
				samplerInfo.anisotropyEnable = VK_TRUE;
			}
			else {
				LOG_WARN("Requested maxAnisotropy {}, but device does not support anisotropic filtering. Disabling it.", info.maxAnisotropy);
			}
		}

		//add a extension struct to enable Min mode
		VkSamplerReductionModeCreateInfoEXT createInfoReduction = {};
		createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;
		createInfoReduction.reductionMode = VulkanUtil::SamplerReductionModeToVk(info.reductionMode);
		samplerInfo.pNext = &createInfoReduction;

		if (vkCreateSampler(VULKAN_DEVICE, &samplerInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create texture sampler!");
		}
	}

	void VulkanRHISampler::Destroy()
	{
		vkDestroySampler(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHIShader::VulkanRHIShader(const RHIShaderInfo& info) : RHIShader(info)
	{
		// 从spv文件的字符串信息里收集定义的宏
		std::regex pattern("#define (\\w+)");
		for (std::cregex_iterator it((char*)info.code.data(), (char*)info.code.data() + info.code.size(), pattern);
			it != std::cregex_iterator{}; it++)
		{
			reflectInfo.definedSymbols.insert((*it)[1].str());
		}

		// 创建ShaderModule
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = info.code.size();
		createInfo.pCode = (const uint32_t*)info.code.data();

		if (vkCreateShaderModule(VULKAN_DEVICE, &createInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create shader module!");
		}
		this->info.code.clear();    // 代码不需要带着了

		// 收集反射信息
		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(info.code.size(), info.code.data(), &module);
		if (result != SPV_REFLECT_RESULT_SUCCESS)    LOG_ERROR("Failed to generate shader reflect data!");
		if (module.entry_point_count != 1)           LOG_ERROR("Shader file contains more than one entry!");

		const SpvReflectEntryPoint* entry = spvReflectGetEntryPoint(&module, module.entry_points[0].name);

		reflectInfo.name = std::string(entry->name);
		reflectInfo.frequency = VulkanUtil::SpvShaderStageToFrequency(entry->shader_stage);
		if (reflectInfo.frequency == SHADER_FREQUENCY_COMPUTE)
		{
			reflectInfo.localSizeX = entry->local_size.x;
			reflectInfo.localSizeY = entry->local_size.y;
			reflectInfo.localSizeZ = entry->local_size.z;
		}

		// bool isGLSL = module.source_language & SpvSourceLanguageGLSL;
		// bool isHLSL = module.source_language & SpvSourceLanguageHLSL;

		// pushConstant
		// uint32_t pushConstantCnt;
		// spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCnt, NULL);
		// if (pushConstantCnt > 0)
		// {
		//     std::vector<SpvReflectBlockVariable*> blockVariables(pushConstantCnt + 1);
		//     spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCnt, blockVariables.data());
		// }

		// 着色器输入和输出
		uint32_t inputVariableCnt;
		spvReflectEnumerateInputVariables(&module, &inputVariableCnt, NULL);
		if (inputVariableCnt > 0)
		{
			std::vector<SpvReflectInterfaceVariable*> inputVariables(inputVariableCnt);
			spvReflectEnumerateInputVariables(&module, &inputVariableCnt, inputVariables.data());

			for (uint32_t i = 0; i < inputVariableCnt; i++)
			{
				if (inputVariables[i]->location < MAX_SHADER_IN_OUT_VARIABLES)
					reflectInfo.inputVariables[inputVariables[i]->location] = VulkanUtil::SpvFormatToRHIFormat(inputVariables[i]->format);
			}
		}

		uint32_t outputVariableCnt;
		spvReflectEnumerateOutputVariables(&module, &outputVariableCnt, NULL);
		if (outputVariableCnt > 0)
		{
			std::vector<SpvReflectInterfaceVariable*> outputVariables(outputVariableCnt);
			spvReflectEnumerateOutputVariables(&module, &outputVariableCnt, outputVariables.data());

			for (uint32_t i = 0; i < outputVariableCnt; i++)
			{
				if (outputVariables[i]->location < MAX_SHADER_IN_OUT_VARIABLES)
					reflectInfo.outputVariables[outputVariables[i]->location] = VulkanUtil::SpvFormatToRHIFormat(outputVariables[i]->format);
			}
		}

		// specialization constant
		// uint32_t specializationConstantCnt;
		// spvReflectEnumerateSpecializationConstants(&module, &specializationConstantCnt, NULL);
		// if(specializationConstantCnt > 0)
		// {
		//     std::vector<SpvReflectSpecializationConstant*> specializationConstants(specializationConstantCnt);
		//     spvReflectEnumerateSpecializationConstants(&module, &specializationConstantCnt, specializationConstants.data());

		//     for(uint32_t i = 0; i < specializationConstantCnt; i++)
		//     {
		//         specializationConstants[i];
		//     }
		// }

		// interface variable
		// uint32_t interfaceVariableCnt;
		// spvReflectEnumerateInterfaceVariables(&module, &interfaceVariableCnt, NULL);
		// if(interfaceVariableCnt > 0)
		// {
		//     std::vector<SpvReflectInterfaceVariable*> interfaceVariables(interfaceVariableCnt);
		//     spvReflectEnumerateInterfaceVariables(&module, &interfaceVariableCnt, interfaceVariables.data());

		//     for(uint32_t i = 0; i < interfaceVariableCnt; i++)
		//     {
		//         interfaceVariables[i];
		//     }
		// }

		// 描述符
		uint32_t descriptorSetCnt;
		spvReflectEnumerateDescriptorSets(&module, &descriptorSetCnt, NULL);
		if (descriptorSetCnt > 0)
		{
			std::vector<SpvReflectDescriptorSet*> descriptorSets(descriptorSetCnt);
			spvReflectEnumerateDescriptorSets(&module, &descriptorSetCnt, descriptorSets.data());

			uint32_t descriptorSize = 0;
			for (uint32_t i = 0; i < descriptorSetCnt; i++)   descriptorSize += descriptorSets[i]->binding_count;
			reflectInfo.resources.resize(descriptorSize);

			uint32_t i = 0;
			for (uint32_t set = 0; set < descriptorSetCnt; set++)
			{
				SpvReflectDescriptorSet* currentSet = descriptorSets[set];

				for (uint32_t binding = 0; binding < currentSet->binding_count; binding++, i++)
				{
					SpvReflectDescriptorBinding* currentBinding = currentSet->bindings[binding];

					ShaderResourceEntry& entry = reflectInfo.resources[i];
					//entry.name = std::string(currentBinding->name);
					entry.set = currentBinding->set;
					entry.binding = currentBinding->binding;
					entry.size = currentBinding->count;
					entry.type = VulkanUtil::SpvDescriptorTypeToResourceType(currentBinding->descriptor_type);
					entry.frequency = reflectInfo.frequency;

					if ((currentBinding->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE) ||
						(currentBinding->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE))
					{
						bool isArray = (currentBinding->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY);
						// entry.textureViewType = VulkanUtil::SpvDimToTextureViewType(currentBinding->image.dim, isArray); // 暂时不需要这个信息
					}
				}
			}
		}
	}

	VkPipelineShaderStageCreateInfo VulkanRHIShader::GetShaderStageCreateInfo()
	{
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = VulkanUtil::ShaderFrequencyToVkStageFlagBits(info.frequency); // 就是Shader类型给一个标志位
		shaderStage.module = handle; // shaderModule
		shaderStage.pName = info.entry.c_str(); // 入口 一般都是main

		return shaderStage;
	}

	void VulkanRHIShader::Destroy()
	{
		vkDestroyShaderModule(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHIBuffer::VulkanRHIBuffer(const RHIBufferInfo& info) : RHIBuffer(info)
	{
		VkBufferUsageFlags usage = VulkanUtil::ResourceTypeToBufferUsage(info.type, VULKAN_RHI->GetConfig().enableRayTracing);
		if (info.memoryUsage == MEMORY_USAGE_GPU_ONLY || info.memoryUsage == MEMORY_USAGE_GPU_TO_CPU)   usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = info.size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.queueFamilyIndexCount = 0,
			bufferInfo.pQueueFamilyIndices = NULL;

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VulkanUtil::MemoryUsageToVma(info.memoryUsage);
		if (info.creationFlag & BUFFER_CREATION_PERSISTENT_MAP)
		{
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
			mapped = true;
		}
		//allocationCreateInfo.requiredFlags    //必要需求
		//allocationCreateInfo.preferredFlags   //尽量满足的需求

		//VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT 强制要求单独开辟内存
		//VMA_ALLOCATION_CREATE_MAPPED_BIT  强制持久映射，可由allocationInfo.pMappedData直接访问
		//vmaFlushAllocation(), vmaInvalidateAllocation() 指定缓存写入和缓存失效

		allocationInfo = {};
		if (info.creationFlag & BUFFER_CREATION_FORCE_ALIGNMENT)
		{
			// 带上一个256位对齐，光追等会用到
			if (vmaCreateBufferWithAlignment(VULKAN_VMA,
				&bufferInfo,
				&allocationCreateInfo,
				256,
				&handle,
				&allocation,
				&allocationInfo) != VK_SUCCESS)
			{
				LOG_ERROR("VMA failed to allocate buffer!");
			}
		}
		else
		{
			if (vmaCreateBuffer(VULKAN_VMA,
				&bufferInfo,
				&allocationCreateInfo,
				&handle,
				&allocation,
				&allocationInfo) != VK_SUCCESS)
			{
				LOG_ERROR("VMA failed to allocate buffer!");
			}
		}
	}

	void* VulkanRHIBuffer::Map()
	{
		if (info.creationFlag & BUFFER_CREATION_PERSISTENT_MAP) return allocationInfo.pMappedData;
		if (!mapped)
		{
			vmaMapMemory(VULKAN_VMA, allocation, &pointer);
			mapped = true;
		}
		return pointer;
	}

	void VulkanRHIBuffer::UnMap()
	{
		if (mapped && !(info.creationFlag & BUFFER_CREATION_PERSISTENT_MAP))
		{
			vmaUnmapMemory(VULKAN_VMA, allocation);
			pointer = nullptr;
			mapped = false;
		}
	}

	void VulkanRHIBuffer::Destroy()
	{
		vmaDestroyBuffer(VULKAN_VMA, handle, allocation);
	}

	VulkanRHIRootSignature::VulkanRHIRootSignature(const RHIRootSignatureInfo& info) : RHIRootSignature(info)
	{
		for (const ShaderResourceEntry& entry : info.GetEntries())
		{
			//描述符布局绑定信息
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = entry.binding;
			layoutBinding.stageFlags = VulkanUtil::ShaderFrequencyToVkStageFlags(entry.frequency);
			layoutBinding.descriptorType = VulkanUtil::ResourceTypeToVk(entry.type);
			layoutBinding.descriptorCount = entry.size == 0 ? 8192 : entry.size;    //指定该绑定处的描述符数量，>1为数组（一个layout多个binding，一个binding多个descriptor）
			//开启扩展后为最大可能的数量，且这种binding必须在layout的最后
			layoutBinding.pImmutableSamplers = nullptr;

			if (setInfos.size() < entry.set + 1) setInfos.resize(entry.set + 1);
			setInfos[entry.set].bindings.push_back(layoutBinding);
		}

		for (SetInfo& set : setInfos)
		{
			if (set.bindings.size() > 0)
			{
				//描述符布局信息
				VkDescriptorSetLayoutCreateInfo layoutInfo;
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.bindingCount = (uint32_t)set.bindings.size();
				layoutInfo.pBindings = set.bindings.data();
				layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;  //TODO 使得描述符可以实时更新

				// 启用可变大小描述符数量标志位
				//VkDescriptorBindingFlagsEXT descriptorBindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
				std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {};
				descriptorBindingFlags.resize((uint32_t)set.bindings.size());
				for (auto& descriptorBindingFlag : descriptorBindingFlags)
				{
					descriptorBindingFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;  //允许 Variable Descriptor binding 的 Descriptor 在没有被动态访问时不指定为有效的描述符
				}

				// 用于bindless创建可变的binding descriptor数目
				VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags{};
				setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
				setLayoutBindingFlags.bindingCount = (uint32_t)set.bindings.size();
				setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

				// 指定 Descriptor Set Layout CreateInfo 扩展
				layoutInfo.pNext = &setLayoutBindingFlags;

				if (vkCreateDescriptorSetLayout(VULKAN_DEVICE, &layoutInfo, nullptr, &set.layout) != VK_SUCCESS)
				{
					LOG_ERROR("Failed to create descriptor set layout!");
				}
			}
		}
	}

	RHIDescriptorSetRef VulkanRHIRootSignature::CreateDescriptorSet(uint32_t set)
	{
		if (setInfos.size() > set && setInfos[set].bindings.size() > 0)
		{
			RHIDescriptorSetRef descriptorSet = std::make_shared<VulkanRHIDescriptorSet>(setInfos[set].layout);
			DYNAMICRHI->RegisterResource(descriptorSet);

			return descriptorSet;
		}

		LOG_ERROR("Unable to find descriptor info!");
		return nullptr;
	}

	void VulkanRHIRootSignature::Destroy()
	{
		for (SetInfo& set : setInfos)
		{
			vkDestroyDescriptorSetLayout(VULKAN_DEVICE, set.layout, nullptr);
		}
	}

	VulkanRHIDescriptorSet::VulkanRHIDescriptorSet(VkDescriptorSetLayout setLayout) : RHIDescriptorSet()
	{
		//描述符集合信息
		VkDescriptorSetLayout layouts[] = { setLayout };

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = VULKAN_DESCPOOL;      //指定描述符池
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;                                //指定描述符集合的布局

		if (vkAllocateDescriptorSets(VULKAN_DEVICE, &allocInfo, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to allocate descriptor set!");
		}

		// printRawHandle();
	}

	VulkanRHIDescriptorSet::VulkanRHIDescriptorSet(VkDescriptorSet aSet)
	{
		handle = aSet;
	}

	GameEngine::RHIDescriptorSet& VulkanRHIDescriptorSet::UpdateDescriptor(const RHIDescriptorUpdateInfo& descriptorUpdateInfo)
	{
		//更新写入信息
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = handle;
		descriptorWrite.dstBinding = descriptorUpdateInfo.binding;
		descriptorWrite.dstArrayElement = descriptorUpdateInfo.index;
		descriptorWrite.descriptorType = VulkanUtil::ResourceTypeToVk(descriptorUpdateInfo.resourceType);
		descriptorWrite.descriptorCount = 1;

		VkDescriptorImageInfo imageDescriptor = {};
		VkDescriptorBufferInfo bufferDescriptor = {};
		VkWriteDescriptorSetAccelerationStructureKHR accelerationDescriptor = {};

		switch (descriptorUpdateInfo.resourceType) {
		case RESOURCE_TYPE_SAMPLER:
			imageDescriptor.sampler = CAST<VulkanRHISampler>(descriptorUpdateInfo.sampler)->GetHandle();
			descriptorWrite.pImageInfo = &imageDescriptor;
			break;

		case RESOURCE_TYPE_TEXTURE:
		case RESOURCE_TYPE_RW_TEXTURE:
		case RESOURCE_TYPE_TEXTURE_CUBE:
			imageDescriptor.imageView = CAST<VulkanRHITextureView>(descriptorUpdateInfo.textureView)->GetHandle();
			imageDescriptor.imageLayout = VulkanUtil::ResourceTypeToImageLayout(descriptorUpdateInfo.resourceType);
			descriptorWrite.pImageInfo = &imageDescriptor;
			break;

		case RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:
			imageDescriptor.sampler = CAST<VulkanRHISampler>(descriptorUpdateInfo.sampler)->GetHandle();
			imageDescriptor.imageView = CAST<VulkanRHITextureView>(descriptorUpdateInfo.textureView)->GetHandle();
			imageDescriptor.imageLayout = VulkanUtil::ResourceTypeToImageLayout(descriptorUpdateInfo.resourceType);
			descriptorWrite.pImageInfo = &imageDescriptor;
			break;

		case RESOURCE_TYPE_BUFFER:
		case RESOURCE_TYPE_RW_BUFFER:
		case RESOURCE_TYPE_UNIFORM_BUFFER:
			bufferDescriptor.buffer = CAST<VulkanRHIBuffer>(descriptorUpdateInfo.buffer)->GetHandle();
			bufferDescriptor.offset = descriptorUpdateInfo.bufferOffset;
			bufferDescriptor.range = (descriptorUpdateInfo.bufferRange > 0) ? descriptorUpdateInfo.bufferRange : VK_WHOLE_SIZE;
			descriptorWrite.pBufferInfo = &bufferDescriptor;
			break;

		case RESOURCE_TYPE_RAY_TRACING:
			accelerationDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			accelerationDescriptor.accelerationStructureCount = 1;
			accelerationDescriptor.pAccelerationStructures = &CAST<VulkanRHITopLevelAccelerationStructure>(descriptorUpdateInfo.tlas)->GetHandle();
			descriptorWrite.pNext = &accelerationDescriptor;
			break;

		default:    LOG_ERROR("Unsupported resource type!");
		}

		vkUpdateDescriptorSets(VULKAN_DEVICE, 1, &descriptorWrite, 0, nullptr);

		return *this;
	}

	void VulkanRHIDescriptorSet::Destroy()
	{
		// TODO:
	}

	VulkanRHIGraphicsPipeline::VulkanRHIGraphicsPipeline(const RHIGraphicsPipelineInfo& info) : RHIGraphicsPipeline(info)
	{
		// 描述符 push constant
		std::vector<VkPushConstantRange> pushConstants;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		for (const auto& pushConstant : info.rootSignature->GetInfo().GetPushConstants())
		{
			pushConstants.push_back(VulkanUtil::GetPushConstantInfo(pushConstant));
		}
		for (const auto& setInfo : CAST<VulkanRHIRootSignature>(info.rootSignature)->GetSetInfos())
		{
			descriptorSetLayouts.push_back(setInfo.layout);
		}
		pipelineLayout = VulkanUtil::CreatePipelineLayout(VULKAN_DEVICE, descriptorSetLayouts, pushConstants);

		// 着色器
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		if (info.vertexShader)   shaderStages.push_back(CAST<VulkanRHIShader>(info.vertexShader)->GetShaderStageCreateInfo());
		if (info.geometryShader) shaderStages.push_back(CAST<VulkanRHIShader>(info.geometryShader)->GetShaderStageCreateInfo());
		if (info.fragmentShader) shaderStages.push_back(CAST<VulkanRHIShader>(info.fragmentShader)->GetShaderStageCreateInfo());

		// Pipeline创建需要一个RenderPass
		uint32_t attachmentSize = 0;
		VulkanUtil::VulkanRenderPassAttachments renderPassAttachments = {};
		for (uint32_t i = 0; i < info.colorAttachmentFormats.size(); i++)
		{
			if (info.colorAttachmentFormats[i] == FORMAT_UKNOWN)  // 设计是 默认8个位置，判断到FORMAT_UKNOWN说明后续都没有了
				break;

			attachmentSize++;

			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = VulkanUtil::RHIFormatToVkFormat(info.colorAttachmentFormats[i]);
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			renderPassAttachments.colorAttachments.push_back(colorAttachment);
		}

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VulkanUtil::RHIFormatToVkFormat(info.depthStencilAttachmentFormat);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		renderPassAttachments.depthStencilAttachment = depthAttachment;  // 如果没有，格式是FORMAT_UKNOWN
		VkRenderPass renderPass = VULKAN_RHI->FindOrCreateVkRenderPass(renderPassAttachments);

		// 光栅固定管线状态
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetInputStateCreateInfo(info.vertexInputState);
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = GetPipelineInputAssemblyStateCreateInfo(info.primitiveType);
		VkPipelineViewportStateCreateInfo viewportState = GetPipelineViewportStateCreateInfo();
		VkPipelineRasterizationStateCreateInfo rasterizer = GetPipelineRasterizationStateCreateInfo(info.rasterizerState);
		VkPipelineMultisampleStateCreateInfo multisampling = GetPipelineMultisampleStateCreateInfo();
		VkPipelineColorBlendStateCreateInfo colorBlending = GetPipelineColorBlendStateCreateInfo(info.blendState, attachmentSize);
		VkPipelineDepthStencilStateCreateInfo depthStencil = GetPipelineDepthStencilStateCreateInfo(info.depthStencilState);
		VkPipelineDynamicStateCreateInfo dynamicState = GetPipelineDynamicStateCreateInfo();

		GetDynamicInputStateCreateInfo(info.vertexInputState);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		//pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(VULKAN_DEVICE, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create graphics pipeline!");
		}
	}

	VulkanRHIRayTracingPipeline::VulkanRHIRayTracingPipeline(const RHIRayTracingPipelineInfo& info) : RHIRayTracingPipeline(info)
	{
		// 描述符 push constant
		std::vector<VkPushConstantRange> pushConstants;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		for (const auto& pushConstant : info.rootSignature->GetInfo().GetPushConstants())
		{
			pushConstants.push_back(VulkanUtil::GetPushConstantInfo(pushConstant));
		}
		for (const auto& setInfo : CAST<VulkanRHIRootSignature>(info.rootSignature)->GetSetInfos())
		{
			descriptorSetLayouts.push_back(setInfo.layout);
		}
		pipelineLayout = VulkanUtil::CreatePipelineLayout(VULKAN_DEVICE, descriptorSetLayouts, pushConstants);
		// 着色器，用SBT描述

		VkRayTracingPipelineCreateInfoKHR pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.maxPipelineRayRecursionDepth = 1;	//光线最多的弹射次数
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.stageCount = (uint32_t)CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetStages().size();
		pipelineInfo.pStages = CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetStages().data();
		pipelineInfo.groupCount = (uint32_t)CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetGroups().size();
		pipelineInfo.pGroups = CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetGroups().data();

		if (vkCreateRayTracingPipelinesKHR(VULKAN_DEVICE, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create compute pipeline!");
		}

		// 处理SBT句柄
		BuildShaderGroupHandle();
	}

	void VulkanRHIGraphicsPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, handle);

		vkCmdSetVertexInputEXT(commandBuffer,   // 加了一个动态绑定，暂时只为了不报错？
			(uint32_t)dynamicBindingDescriptions.size(),
			dynamicBindingDescriptions.data(),
			(uint32_t)dynamicAttributeDescriptions.size(),
			dynamicAttributeDescriptions.data());
	}

	void VulkanRHIRayTracingPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, handle);
	}

	void VulkanRHIRayTracingPipeline::Destroy()
	{
		vkDestroyPipelineLayout(VULKAN_DEVICE, pipelineLayout, nullptr);
		vkDestroyPipeline(VULKAN_DEVICE, handle, nullptr);
	}
	uint32_t Align(uint32_t value, uint32_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}
	void VulkanRHIRayTracingPipeline::BuildShaderGroupHandle()
	{
		uint32_t rayGenGroupSize = CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetRayGenGroupSize();
		uint32_t hitGroupSize = CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetHitGroupSize();
		uint32_t rayMissGroupSize = CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetRayMissGroupSize();
		uint32_t groupSize = CAST<VulkanRHIShaderBindingTable>(info.shaderBindingTable)->GetGroups().size();

		// 0. 初始化有关内存偏移和对齐的信息
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = VULKAN_RHI->GetRayTracingPipelineProperties();
		uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		uint32_t handleSizeAligned = Align(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		{
			// 每个handle按照shaderGroupHandleSize对齐；
			// 每个table按照shaderGroupBaseAlignment对齐
			raygenRegion.deviceAddress = 0;
			raygenRegion.stride = Align(rayGenGroupSize * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);	//对于pRayGenShaderBindingTable，步长和大小要一致？
			raygenRegion.size = Align(rayGenGroupSize * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

			missRegion.deviceAddress = raygenRegion.size;
			missRegion.stride = handleSizeAligned;
			missRegion.size = Align(rayMissGroupSize * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);;

			hitRegion.deviceAddress = raygenRegion.size + missRegion.size;
			hitRegion.stride = handleSizeAligned;
			hitRegion.size = Align(hitGroupSize * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);;
		}

		// 1. 获取ShaderGroup的句柄信息，在绘制时需要使用
		std::vector<uint8_t> shaderHandleStorage;
		{
			uint32_t sbtSize = groupSize * handleSize;

			shaderHandleStorage = std::vector<uint8_t>(sbtSize);
			vkGetRayTracingShaderGroupHandlesKHR(VULKAN_DEVICE, handle, 0, groupSize, sbtSize, shaderHandleStorage.data());
		}

		// 2. 创建buffer保存句柄信息
		{
			uint32_t totalSize = raygenRegion.size + missRegion.size + hitRegion.size;
			RHIBufferInfo bufferInfo = {};
			bufferInfo.type = RESOURCE_TYPE_RAY_TRACING;
			bufferInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
			bufferInfo.size = totalSize;
			shaderGroupHandleBuffer = VULKAN_RHI->CreateBuffer(bufferInfo);

			uint64_t address = VulkanUtil::GetBufferDeviceAddress(CAST<VulkanRHIBuffer>(shaderGroupHandleBuffer)->GetHandle(), VULKAN_DEVICE);
			raygenRegion.deviceAddress += address;
			missRegion.deviceAddress += address;
			hitRegion.deviceAddress += address;

			uint32_t handleIndex = 0;

			// 把获取到的句柄信息按内存对齐拷贝到buffer内
			for (int i = 0; i < rayGenGroupSize; i++)
			{
				memcpy((uint8_t*)shaderGroupHandleBuffer->Map() + (0 + (i * raygenRegion.stride)),
					shaderHandleStorage.data() + (handleIndex++ * handleSize),
					handleSize);
			}

			for (int i = 0; i < rayMissGroupSize; i++)
			{
				memcpy((uint8_t*)shaderGroupHandleBuffer->Map() + (raygenRegion.size + (i * missRegion.stride)),
					shaderHandleStorage.data() + (handleIndex++ * handleSize),
					handleSize);
			}

			for (int i = 0; i < hitGroupSize; i++)
			{
				memcpy((uint8_t*)shaderGroupHandleBuffer->Map() + (raygenRegion.size + missRegion.size + (i * hitRegion.stride)),
					shaderHandleStorage.data() + (handleIndex++ * handleSize),
					handleSize);
			}
		}
	}

	void VulkanRHIGraphicsPipeline::Destroy()
	{
		vkDestroyPipelineLayout(VULKAN_DEVICE, pipelineLayout, nullptr);
		vkDestroyPipeline(VULKAN_DEVICE, handle, nullptr);
	}
	VkPipelineVertexInputStateCreateInfo VulkanRHIGraphicsPipeline::GetInputStateCreateInfo(const VertexInputStateInfo& vertexInputState)
	{
		for (const VertexElement& vertexElement : vertexInputState.vertexElements)
		{
			uint8_t binding = vertexElement.streamIndex;
			VkVertexInputAttributeDescription attributeDescription = {};
			attributeDescription.binding = binding;
			attributeDescription.location = vertexElement.attributeIndex;                                       // 对应layout location
			attributeDescription.format = VulkanUtil::RHIFormatToVkFormat(vertexElement.format);   // 属性格式
			attributeDescription.offset = vertexElement.offset;                                                 // 字段偏移
			attributeDescriptions.push_back(attributeDescription);

			while (bindingDescriptions.size() < vertexElement.streamIndex + 1) bindingDescriptions.push_back({});   // 下三项对所有同binding的vertexElement应该全部一致
			bindingDescriptions[binding].binding = binding;
			bindingDescriptions[binding].stride = vertexElement.stride;                                                                             //步长
			bindingDescriptions[binding].inputRate = vertexElement.useInstanceIndex ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;  //输入速率，逐顶点/逐实例
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		return vertexInputInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineInputAssemblyStateCreateInfo(const PrimitiveType& primitiveType)
	{
		// 输入Assembly信息
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VulkanUtil::PrimitiveTypeToVk(primitiveType);      // 图元拓扑
		inputAssembly.primitiveRestartEnable = VK_FALSE;                            // 设为true，可以通过0xFFFF或者0xFFFFFFFF为特殊索引，分解_STRIP拓扑下的结构
		inputAssembly.flags = 0;

		return inputAssembly;
	}

	VkPipelineViewportStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineViewportStateCreateInfo()
	{
		// 视窗信息
		// VkViewport viewport = {};
		// viewport.x = 0.0f;
		// viewport.y = 0.0f;
		// viewport.width = (float)extent.width;
		// viewport.height = (float)extent.height;
		// viewport.minDepth = 0.0f;
		// viewport.maxDepth = 1.0f;

		// 裁剪矩形信息
		// VkRect2D scissor = {};
		// scissor.offset = { 0, 0 };
		// scissor.extent = extent;

		// 使用dynamic 不在这里创建
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;
		viewportState.flags = 0;

		return viewportState;
	}

	VkPipelineRasterizationStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineRasterizationStateCreateInfo(const RHIRasterizerStateInfo& rasterizerState)
	{
		// 光栅化信息
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = rasterizerState.depthClipMode == DEPTH_CLAMP ? VK_TRUE : VK_FALSE;            //对于超过远近裁剪平面的处理
		rasterizer.rasterizerDiscardEnable = VK_FALSE;                                                              //禁止图元传输
		rasterizer.polygonMode = VulkanUtil::FillModeToVk(rasterizerState.fillMode);                       //多边形的填充模式
		rasterizer.lineWidth = 1.0f;                                                                                //填充模式为线框时的线宽度
		rasterizer.cullMode = VulkanUtil::CullModeToVk(rasterizerState.cullMode);                          //裁剪模式
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;                                                     //面手性

		// rasterizer.depthBiasEnable = (  rasterizerState.depthBias > 0.0f ||
		//                                 rasterizerState.slopeScaleDepthBias > 0.0f) ? VK_TRUE : VK_FALSE;        //深度缓冲的bias
		rasterizer.depthBiasEnable = VK_TRUE;                                                                       //动态设置
		rasterizer.depthBiasConstantFactor = rasterizerState.depthBias;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = rasterizerState.slopeScaleDepthBias;
		rasterizer.flags = 0;

		return rasterizer;
	}

	VkPipelineMultisampleStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineMultisampleStateCreateInfo()
	{
		// 多重采样信息
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;     // TODO 之后再支持
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;
		multisampling.flags = 0;

		return multisampling;
	}

	VkPipelineColorBlendStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineColorBlendStateCreateInfo(const RHIBlendStateInfo& blendState, uint32_t size)
	{
		//混合信息
		for (uint32_t i = 0; i < size; i++)
		{
			auto& attachment = blendState.renderTargets[i];

			VkPipelineColorBlendAttachmentState attachmentState = {};
			attachmentState.blendEnable = attachment.enable;
			attachmentState.colorBlendOp = VulkanUtil::BlendOpToVk(attachment.colorBlendOp);
			attachmentState.srcColorBlendFactor = VulkanUtil::BlendFactorToVk(attachment.colorSrcBlend);
			attachmentState.dstColorBlendFactor = VulkanUtil::BlendFactorToVk(attachment.colorDstBlend);
			attachmentState.alphaBlendOp = VulkanUtil::BlendOpToVk(attachment.alphaBlendOp);
			attachmentState.srcAlphaBlendFactor = VulkanUtil::BlendFactorToVk(attachment.alphaSrcBlend);
			attachmentState.dstAlphaBlendFactor = VulkanUtil::BlendFactorToVk(attachment.alphaDstBlend);
			attachmentState.colorWriteMask = VulkanUtil::ColorWriteMaskToVk(attachment.colorWriteMask);

			blendStates.push_back(attachmentState);
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = (uint32_t)blendStates.size();
		colorBlending.pAttachments = blendStates.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		return colorBlending;
	}

	VkPipelineDepthStencilStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineDepthStencilStateCreateInfo(const RHIDepthStencilStateInfo& depthStencilState)
	{
		//深度/模板缓冲信息
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = depthStencilState.enableDepthTest;                                               //深度测试
		depthStencil.depthWriteEnable = depthStencilState.enableDepthWrite;                                             //深度写入
		depthStencil.depthCompareOp = VulkanUtil::CompareFunctionToVk(depthStencilState.depthTest);    //深度比较方式
		depthStencil.depthBoundsTestEnable = VK_FALSE;              //边界检测
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;

		depthStencil.stencilTestEnable = VK_FALSE;                  //模板测试
		depthStencil.front = {};
		depthStencil.back = {};

		return depthStencil;
	}

	VkPipelineDynamicStateCreateInfo VulkanRHIGraphicsPipeline::GetPipelineDynamicStateCreateInfo()
	{
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		return dynamicState;
	}

	void VulkanRHIGraphicsPipeline::GetDynamicInputStateCreateInfo(const VertexInputStateInfo& vertexInputState)
	{
		// 跟静态的声明基本一样，但是填充的结构体不一样
		for (const VertexElement& vertexElement : vertexInputState.vertexElements)
		{
			uint8_t binding = vertexElement.streamIndex;
			VkVertexInputAttributeDescription2EXT dynamicAttributeDescription = {};
			dynamicAttributeDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
			dynamicAttributeDescription.binding = binding;
			dynamicAttributeDescription.location = vertexElement.attributeIndex;
			dynamicAttributeDescription.format = VulkanUtil::RHIFormatToVkFormat(vertexElement.format);
			dynamicAttributeDescription.offset = vertexElement.offset;
			dynamicAttributeDescriptions.push_back(dynamicAttributeDescription);

			while (dynamicBindingDescriptions.size() < vertexElement.streamIndex + 1) dynamicBindingDescriptions.push_back({});
			dynamicBindingDescriptions[binding].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
			dynamicBindingDescriptions[binding].binding = binding;
			dynamicBindingDescriptions[binding].stride = vertexElement.stride;
			dynamicBindingDescriptions[binding].divisor = 1;    // 这是啥？
			dynamicBindingDescriptions[binding].inputRate = vertexElement.useInstanceIndex ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
		}
	}

	VulkanRHIRenderPass::VulkanRHIRenderPass(const RHIRenderPassInfo& info) : RHIRenderPass(info)
	{
		// 创建FrameBuffer 每个附件需要有ImageView
		std::vector<VkImageView> imageViews;
		VulkanUtil::VulkanRenderPassAttachments renderPassAttachments = {};
		for (uint32_t i = 0; i < info.colorAttachments.size(); i++)
		{
			if (info.colorAttachments[i].textureView == nullptr)
				break;

			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = VulkanUtil::RHIFormatToVkFormat(info.colorAttachments[i].textureView->GetInfo().format);
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VulkanUtil::AttachmentLoadOpToVk(info.colorAttachments[i].loadOp);
			colorAttachment.storeOp = VulkanUtil::AttachmentStoreOpToVk(info.colorAttachments[i].storeOp);

			renderPassAttachments.colorAttachments.push_back(colorAttachment);

			imageViews.push_back(CAST<VulkanRHITextureView>(info.colorAttachments[i].textureView)->GetHandle());
		}
		// 深度附件需要有ImageView
		if (info.depthStencilAttachment.textureView != nullptr)
		{
			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = VulkanUtil::RHIFormatToVkFormat(info.depthStencilAttachment.textureView->GetInfo().format);
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VulkanUtil::AttachmentLoadOpToVk(info.depthStencilAttachment.loadOp);
			depthAttachment.storeOp = VulkanUtil::AttachmentStoreOpToVk(info.depthStencilAttachment.storeOp);

			renderPassAttachments.depthStencilAttachment = depthAttachment;

			imageViews.push_back(CAST<VulkanRHITextureView>(info.depthStencilAttachment.textureView)->GetHandle());
		}

		// Pool中缓存RenderPass   imageViews最后一项是深度附件
		handle = VULKAN_RHI->FindOrCreateVkRenderPass(renderPassAttachments);

		// 创建framebuffer  TODO:把FrameBuffer塞到RenderPass里了
		// Framebuffer创建时是需要具体资源的，因为这里把imageViews传进去了
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = handle;
		framebufferInfo.attachmentCount = (uint32_t)imageViews.size();
		framebufferInfo.pAttachments = imageViews.data();
		framebufferInfo.width = info.extent.width;
		framebufferInfo.height = info.extent.height;
		framebufferInfo.layers = info.layers;

		frameBuffer = VULKAN_RHI->FindOrCreateVkFramebuffer(framebufferInfo);
	}

	void VulkanRHIRenderPass::Destroy()
	{
	}

	VulkanRHIComputePipeline::VulkanRHIComputePipeline(const RHIComputePipelineInfo& info) : RHIComputePipeline(info)
	{
		// 描述符 push constant
		std::vector<VkPushConstantRange> pushConstants;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		for (const auto& pushConstant : info.rootSignature->GetInfo().GetPushConstants())
		{
			pushConstants.push_back(VulkanUtil::GetPushConstantInfo(pushConstant));
		}
		for (const auto& setInfo : CAST<VulkanRHIRootSignature>(info.rootSignature)->GetSetInfos())
		{
			descriptorSetLayouts.push_back(setInfo.layout);
		}
		pipelineLayout = VulkanUtil::CreatePipelineLayout(VULKAN_DEVICE, descriptorSetLayouts, pushConstants);

		// 着色器
		VkPipelineShaderStageCreateInfo shaderStage = CAST<VulkanRHIShader>(info.computeShader)->GetShaderStageCreateInfo();

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.stage = shaderStage;
		pipelineInfo.layout = pipelineLayout;

		if (vkCreateComputePipelines(VULKAN_DEVICE, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create compute pipeline!");
		}
	}

	void VulkanRHIComputePipeline::Destroy()
	{
		vkDestroyPipelineLayout(VULKAN_DEVICE, pipelineLayout, nullptr);
		vkDestroyPipeline(VULKAN_DEVICE, handle, nullptr);
	}

	void VulkanRHIComputePipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, handle);
	}

	VulkanRHIBottomLevelAccelerationStructure::VulkanRHIBottomLevelAccelerationStructure(const RHIBottomLevelAccelerationStructureInfo& info) : RHIBottomLevelAccelerationStructure(info)
	{
		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		vertexBufferDeviceAddress.deviceAddress = VulkanUtil::GetBufferDeviceAddress(CAST<VulkanRHIBuffer>(info.vertexBuffer)->GetHandle(), VULKAN_DEVICE);
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
		indexBufferDeviceAddress.deviceAddress = VulkanUtil::GetBufferDeviceAddress(CAST<VulkanRHIBuffer>(info.indexBuffer)->GetHandle(), VULKAN_DEVICE);

		// 定义顶点/索引数据（设备地址）的读取位置及数据解释方式（格式、步长等
		VkAccelerationStructureGeometryTrianglesDataKHR triangles = {};
		triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		triangles.vertexData = vertexBufferDeviceAddress;
		triangles.vertexStride = info.vertexStride;
		// triangles.maxVertex = 3;   // TODO:这对么？
		triangles.maxVertex = info.vertexCount - 1;
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData = indexBufferDeviceAddress;

		// 指定几何体类型（三角形、实例、AABB）及构建标志的包装器
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles = triangles;
		geometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR; // 去重 | 不透明

		// 定义要处理的数据部分（原始计数、偏移量等）
		VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
		rangeInfo.primitiveCount = info.triangleCount;
		rangeInfo.primitiveOffset = info.indexOffset;
		rangeInfo.firstVertex = info.vertexOffset / info.vertexStride;    //所有index将加上该值来索引vertex信息
		rangeInfo.transformOffset = 0;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildInfo.geometryCount = 1; // Deal with one geometry at a time
		buildInfo.pGeometries = &geometry;

		// 查询构建需要的内存大小
		VkAccelerationStructureBuildSizesInfoKHR buildSize = {};
		buildSize.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(VULKAN_DEVICE, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
			&rangeInfo.primitiveCount, &buildSize);

		// 创建临时缓存区
		RHIBufferInfo bufferInfo = {};
		bufferInfo.size = buildSize.accelerationStructureSize;
		bufferInfo.memoryUsage = MEMORY_USAGE_GPU_ONLY;
		bufferInfo.type = RESOURCE_TYPE_RAY_TRACING;
		accelerationStructureBuffer = VULKAN_RHI->CreateBuffer(bufferInfo);

		// 创建加速结构
		VkAccelerationStructureCreateInfoKHR accelerationStructureInfo = {};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureInfo.buffer = CAST<VulkanRHIBuffer>(accelerationStructureBuffer)->GetHandle();
		accelerationStructureInfo.size = accelerationStructureBuffer->GetInfo().size;
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		if (vkCreateAccelerationStructureKHR(VULKAN_DEVICE, &accelerationStructureInfo, nullptr, &handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create Acceleration Structure!");
		}

		// 获取加速结构地址
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = handle;
		address = vkGetAccelerationStructureDeviceAddressKHR(VULKAN_DEVICE, &accelerationDeviceAddressInfo);

		RHIBufferInfo bufferInfo1 = {};
		bufferInfo1.size = buildSize.buildScratchSize;
		bufferInfo1.memoryUsage = MEMORY_USAGE_GPU_ONLY;
		bufferInfo1.type = RESOURCE_TYPE_RW_BUFFER;
		bufferInfo1.creationFlag = BUFFER_CREATION_PERSISTENT_MAP | BUFFER_CREATION_FORCE_ALIGNMENT; // 内存对齐
		RHIBufferRef scratchBuffer = VULKAN_RHI->CreateBuffer(bufferInfo1);

		buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		buildInfo.dstAccelerationStructure = handle;
		buildInfo.scratchData.deviceAddress = VulkanUtil::GetBufferDeviceAddress(CAST<VulkanRHIBuffer>(scratchBuffer)->GetHandle(), VULKAN_DEVICE);
		const VkAccelerationStructureBuildRangeInfoKHR* pBuildRange = &rangeInfo;

		auto immediateCommandContest = VULKAN_RHI->GetImmediateCommandList();
		vkCmdBuildAccelerationStructuresKHR(
			CAST<VulkanRHICommandContextImmediate>(VULKAN_RHI->GetImmediateCommandContext())->GetHandle(),
			1,
			&buildInfo,
			&pBuildRange);

		immediateCommandContest->Flush();
		// scratchBuffer->Destroy();
	}

	void VulkanRHIBottomLevelAccelerationStructure::Destroy()
	{
		vkDestroyAccelerationStructureKHR(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHITopLevelAccelerationStructure::VulkanRHITopLevelAccelerationStructure(const RHITopLevelAccelerationStructureInfo& info) : RHITopLevelAccelerationStructure(info)
	{
		RHIBufferInfo bufferInfo = {};
		bufferInfo.size = sizeof(VkAccelerationStructureInstanceKHR) * info.maxInstance;
		bufferInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
		bufferInfo.type = RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_RAY_TRACING;
		bufferInfo.creationFlag = BUFFER_CREATION_PERSISTENT_MAP;
		instanceBuffer = VULKAN_RHI->CreateBuffer(bufferInfo);
		Update(this->info.instanceInfos);
		this->info.instanceInfos.clear();
	}
	void VulkanRHITopLevelAccelerationStructure::Update(const std::vector<RHIAccelerationStructureInstanceInfo>& instanceInfos)
	{
		bool update = (handle == VK_NULL_HANDLE) ? false : true;

		std::vector<VkAccelerationStructureInstanceKHR> blasInstances;
		for (int i = 0; i < instanceInfos.size(); i++)
		{
			blasInstances.push_back(VulkanUtil::AccelerationStructureInstanceInfoToVk(instanceInfos[i]));
		}
		memcpy(instanceBuffer->Map(), blasInstances.data(), blasInstances.size() * sizeof(VkAccelerationStructureInstanceKHR));

		// 0. 数据结构
		// 填充顶层加速结构使用的几何信息（只有一个），使用的图元是VK_GEOMETRY_TYPE_INSTANCES_KHR
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data.deviceAddress = VulkanUtil::GetBufferDeviceAddress(CAST<VulkanRHIBuffer>(instanceBuffer)->GetHandle(), VULKAN_DEVICE);

		// 构建加速结构的信息
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;			//顶层
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;											//只有一个几何信息，就是顶层的
		accelerationStructureBuildGeometryInfo.mode = update ?
			VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR :   //加速结构是否更新
			VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		accelerationStructureBuildGeometryInfo.dstAccelerationStructure = VK_NULL_HANDLE;

		// 1. 获取需要分配的buffer的尺寸信息
		VkAccelerationStructureBuildSizesInfoKHR buildSize = {};
		buildSize.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		{
			uint32_t primitiveCount = update ? instanceInfos.size() : info.maxInstance;

			//获取buffer尺寸，在下面进行分配
			vkGetAccelerationStructureBuildSizesKHR(
				VULKAN_DEVICE,
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&accelerationStructureBuildGeometryInfo,
				&primitiveCount,
				&buildSize);
		}

		// 2. 如果非更新，需要首先创建加速结构
		if (update == false)
		{
			RHIBufferInfo bufferInfo1 = {};
			bufferInfo1.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			bufferInfo1.type = RESOURCE_TYPE_RAY_TRACING;
			bufferInfo1.creationFlag = 0;
			bufferInfo1.size = buildSize.accelerationStructureSize;
			accelerationStructureBuffer = VULKAN_RHI->CreateBuffer(bufferInfo1);

			VkAccelerationStructureCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			createInfo.size = buildSize.accelerationStructureSize;
			createInfo.buffer = CAST<VulkanRHIBuffer>(accelerationStructureBuffer)->GetHandle();
			createInfo.offset = 0;

			// 此处仅创建了加速结构，并没有实际构建（分配了空间，还没往里填数据）
			if (vkCreateAccelerationStructureKHR(VULKAN_DEVICE, &createInfo, nullptr, &handle) != VK_SUCCESS)
			{
				LOG_ERROR("Failed to create Acceleration Structure!");
			}

			// 获取加速结构地址
			VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			accelerationDeviceAddressInfo.accelerationStructure = handle;
			address = vkGetAccelerationStructureDeviceAddressKHR(VULKAN_DEVICE, &accelerationDeviceAddressInfo);
		}

		// 3. 创建构建过程需要使用的scratch buffer
		RHIBufferInfo bufferInfo2 = {};
		bufferInfo2.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
		bufferInfo2.type = RESOURCE_TYPE_RW_BUFFER;
		bufferInfo2.creationFlag = BUFFER_CREATION_PERSISTENT_MAP | BUFFER_CREATION_FORCE_ALIGNMENT;
		bufferInfo2.size = buildSize.buildScratchSize;
		RHIBufferRef scratchBuffer = VULKAN_RHI->CreateBuffer(bufferInfo2);

		// 4. 命令执行加速结构创建
		{
			//补齐构建所需的信息（需要构建的加速结构，和scratch buffer）
			accelerationStructureBuildGeometryInfo.srcAccelerationStructure = update ?
				handle :
				VK_NULL_HANDLE;
			accelerationStructureBuildGeometryInfo.dstAccelerationStructure = handle;
			accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = VulkanUtil::GetBufferDeviceAddress(CAST<VulkanRHIBuffer>(scratchBuffer)->GetHandle(), VULKAN_DEVICE);

			// 构建
			VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
			accelerationStructureBuildRangeInfo.primitiveCount = instanceInfos.size();
			//accelerationStructureBuildRangeInfo.primitiveCount = 0;
			accelerationStructureBuildRangeInfo.primitiveOffset = 0;
			accelerationStructureBuildRangeInfo.firstVertex = 0;
			accelerationStructureBuildRangeInfo.transformOffset = 0;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

			auto immediateCommandContest = VULKAN_RHI->GetImmediateCommandList();
			vkCmdBuildAccelerationStructuresKHR(
				CAST<VulkanRHICommandContextImmediate>(VULKAN_RHI->GetImmediateCommandContext())->GetHandle(),
				1,
				&accelerationStructureBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());

			immediateCommandContest->Flush();
		}

		// 5. 回收scratch buffer内存
	}

	void VulkanRHITopLevelAccelerationStructure::Destroy()
	{
		vkDestroyAccelerationStructureKHR(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHIShaderBindingTable::VulkanRHIShaderBindingTable(const RHIShaderBindingTableInfo& info) : RHIShaderBindingTable(info)
	{
		typedef struct hitGroupInfo
		{
			VkPipelineShaderStageCreateInfo* closestHitStage = nullptr;
			VkPipelineShaderStageCreateInfo* anyHitStage = nullptr;
			VkPipelineShaderStageCreateInfo* intersectionStage = nullptr;
		}HitGroupInfo;

		std::vector<VkPipelineShaderStageCreateInfo>    rayGenStages;
		std::vector<VkPipelineShaderStageCreateInfo>    missStages;
		std::vector<VkPipelineShaderStageCreateInfo>    hitStages;
		std::vector<HitGroupInfo>                       hitGroupInfos;

		for (auto& shader : info.rayGenGroups)
		{
			rayGenStages.push_back(CAST<VulkanRHIShader>(shader)->GetShaderStageCreateInfo());
		}
		for (auto& shader : info.missGroups)
		{
			missStages.push_back(CAST<VulkanRHIShader>(shader)->GetShaderStageCreateInfo());
		}
		hitStages.reserve(info.hitGroups.size() * 3);
		hitGroupInfos.reserve(info.hitGroups.size());
		for (auto& shaders : info.hitGroups)
		{
			HitGroupInfo groupInfo = {};

			assert(shaders.closestHitShader != nullptr);
			{
				hitStages.push_back(CAST<VulkanRHIShader>(shaders.closestHitShader)->GetShaderStageCreateInfo());
				groupInfo.closestHitStage = &hitStages.back();
			}
			if (shaders.anyHitShader != nullptr)
			{
				hitStages.push_back(CAST<VulkanRHIShader>(shaders.anyHitShader)->GetShaderStageCreateInfo());
				groupInfo.closestHitStage = &hitStages.back();
			}
			if (shaders.intersectionShader != nullptr)
			{
				hitStages.push_back(CAST<VulkanRHIShader>(shaders.intersectionShader)->GetShaderStageCreateInfo());
				groupInfo.closestHitStage = &hitStages.back();
			}

			hitGroupInfos.push_back(groupInfo);
		}
		rayGenGroupSize = rayGenStages.size();
		hitGroupSize = hitGroupInfos.size();
		rayMissGroupSize = missStages.size();

		// Ray gen shaders
		for (auto& stage : rayGenStages)
		{
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = stages.size();
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

			stages.push_back(stage);
			groups.push_back(shaderGroup);
		}

		// Ray miss shaders
		for (auto& stage : missStages)
		{
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = stages.size();
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

			stages.push_back(stage);
			groups.push_back(shaderGroup);
		}

		// Ray hit shaders
		for (auto& group : hitGroupInfos)
		{
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.closestHitShader = stages.size();
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

			stages.push_back(*group.closestHitStage);

			if (group.anyHitStage)
			{
				shaderGroup.anyHitShader = stages.size();
				stages.push_back(*group.anyHitStage);
			}

			if (group.intersectionStage)
			{
				shaderGroup.intersectionShader = stages.size();
				stages.push_back(*group.intersectionStage);
			}

			groups.push_back(shaderGroup);
		}
	}
	void VulkanRHIShaderBindingTable::Destroy()
	{
	}
}