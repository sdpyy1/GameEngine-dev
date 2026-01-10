#include "hzpch.h"
#include "RHI.h"
#include "Vulkan/VulkanRHI.h"
namespace GameEngine {
	DynamicRHIRef DynamicRHI::s_DynamicRHI = nullptr;
	DynamicRHIRef DynamicRHI::Init(RHIConfig config)
	{
		if (s_DynamicRHI == nullptr) {
			switch (config.api) {
			case API::API_Vulkan:
				s_DynamicRHI = std::make_shared<VulkanDynamicRHI>(config);
				break;
			default:
				LOG_ERROR("DynamicRHI not supported");
				break;
			}
		}
		return s_DynamicRHI;
	}

    // 之前的架构使用延迟队列来删除，这里采用计数器来删除，也是延迟删除
	void DynamicRHI::Tick()
	{
        for (auto& resources : resourceMap)
        {
            bool needClean = false;
            for (RHIResourceRef& resource : resources)
            {
                if (resource)
                {
                    if (resource.use_count() == 1)   resource->lastUseTick++;
                    else                            resource->lastUseTick = 0;

                    if (resource->lastUseTick > 6)  //析构资源6帧后销毁
                    {
                        needClean = true;
                         //if(resource->GetType() != RHI_RENDER_PASS) 
                             // std::cout << "RHI resource [" << resource.get() << "] of type [" << resource->GetType() << "] destroied" << std::endl;
                        resource->Destroy();
                        resource = nullptr;
                    }
                }
            }

            // 删除空指针
            if (needClean) {
                resources.erase(std::remove_if(resources.begin(), resources.end(), [](RHIResourceRef x) {
                    return x == nullptr;
                    }), resources.end());
            }
        }
	}

	void DynamicRHI::Destroy()
	{
        for (int32_t i = resourceMap.size() - 1; i >= 0; i--)   // 倒序析构
        {
            auto& resources = resourceMap[i];
            for (RHIResourceRef& resource : resources)
            {
                if (resource)
                {
                    // if(resource->GetType() != RHI_RENDER_PASS && backendInfo.enableDebug) 
                    //     std::cout << "RHI resource [" << resource.get() << "] of type [" << resource->GetType() << "] destroied" << std::endl;

                    resource->Destroy();
                }
            }
        }
	}

}