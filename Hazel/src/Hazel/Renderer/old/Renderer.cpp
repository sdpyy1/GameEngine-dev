#include "hzpch.h"
#include "Hazel/Renderer/old/Renderer.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/old/RendererAPI.h"
#include <glm/gtc/random.hpp>
#include <imgui.h>
#include <Hazel/Platform/Vulkan/VulkanImage.h>
#include "Hazel/Platform/Windows/WindowsWindow.h"

#include "Hazel/Asset/Model/Material.h"
namespace GameEngine {
	// 这里存储常用渲染资源
	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
	constexpr static uint32_t s_RenderCommandQueueCount = 2; // 目前代码只支持=2
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];
	static RendererAPI* s_RendererAPI = nullptr;

	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

	// 在这里完成初始资源的加载
	void Renderer::Init()
	{
		// 存储一些提前创建好的渲染资源
		s_Data = new RendererData();
		// Shader缓存
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// 缓存队列 通过RENDER_SUBMIT提交的命令都会存储在RenderCommandQueue
		for (int i = 0; i < s_RenderCommandQueueCount; i++) {
			s_CommandQueue[i] = new RenderCommandQueue();
		}
		// 并发渲染数
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow()->GetSwapChain().GetImageCount());
		// 创建具体的渲染API对象
		s_RendererAPI = RendererAPI::CreateAPI();

		// Transmittance Lut
		s_Data->m_ShaderLibrary->LoadCommonShader("MultiScatteringLut", true);
		s_Data->m_ShaderLibrary->LoadCommonShader("TransmittanceLut", true);
		s_Data->m_ShaderLibrary->LoadCommonShader("SkyViewLut", true);
		// Bloom
		s_Data->m_ShaderLibrary->LoadCommonShader("Bloom", true);
		// GBuffer Pass
		s_Data->m_ShaderLibrary->LoadCommonShader("gBuffer");
		s_Data->m_ShaderLibrary->LoadCommonShader("gBufferAnim");

		// DirShadow Pass
		s_Data->m_ShaderLibrary->LoadCommonShader("DirShadowMap");
		s_Data->m_ShaderLibrary->LoadCommonShader("DirShadowMapAnim");

		// SpotShadow Pass
		s_Data->m_ShaderLibrary->LoadCommonShader("SpotShadowMap");
		s_Data->m_ShaderLibrary->LoadCommonShader("SpotShadowMapAnim");

		// Grid Pass
		s_Data->m_ShaderLibrary->LoadCommonShader("grid");

		// PreDepth Pass
		s_Data->m_ShaderLibrary->LoadCommonShader("PreDepth");
		s_Data->m_ShaderLibrary->LoadCommonShader("PreDepthAnim");

		// HZB Pass
		s_Data->m_ShaderLibrary->LoadCommonShader("HZB",true);

		// EquirectangularToCubeMap
		s_Data->m_ShaderLibrary->LoadCommonShader("EquirectangularToCubeMap",true);

		// Irradiance Map
		s_Data->m_ShaderLibrary->LoadCommonShader("EnvironmentIrradiance",true);

		// Prefilter Map
		s_Data->m_ShaderLibrary->LoadCommonShader("EnvironmentMipFilter", true);

		// Lighting
		s_Data->m_ShaderLibrary->LoadCommonShader("Lighting");

		// Final Color
		s_Data->m_ShaderLibrary->LoadCommonShader("FinalColor");

		// Sky
		s_Data->m_ShaderLibrary->LoadCommonShader("Sky");




		// 加载纹理
		{
			uint32_t whiteTextureData = 0xffffffff;
			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA;
			spec.Width = 1;
			spec.Height = 1;
			spec.DebugName = "DefaultWhiteTexture";
			s_Data->WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));

			constexpr uint32_t blackTextureData = 0xff000000;
			spec.DebugName = "DefaultBlackTexture";
			s_Data->BlackTexture = Texture2D::Create(spec, Buffer(&blackTextureData, sizeof(uint32_t)));
			spec.DebugName = "DefaultBlackCubeTexture";

			constexpr uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
			s_Data->BlackCubeTexture = TextureCube::Create(spec, Buffer(&blackTextureData, sizeof(blackCubeTextureData)));
			{
				TextureSpecification spec;
				spec.SamplerWrap = TextureWrap::Clamp;
				spec.needYFlip = false;
				spec.DebugName = "BRDFLutTexture";

				s_Data->BRDFLutTexture = Texture2D::Create(spec, std::filesystem::path("Assets/Texture/BRDF_LUT.png"));
			}
		}
		// 为并发帧创建了描述符池、提前准备了全屏顶点数据存入了GPU
		s_RendererAPI->Init();
	}
	void Renderer::Shutdown()
	{
	}
	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->BlackTexture;
	}
	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return s_Data->BRDFLutTexture;
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		return s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		return s_RendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		ASSERT(computePass, "ComputePass cannot be null!");

		s_RendererAPI->BeginComputePass(renderCommandBuffer, computePass);
	}

	void Renderer::EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		s_RendererAPI->EndComputePass(renderCommandBuffer, computePass);
	}

	void Renderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<MaterialOld> material, const glm::uvec3& workGroups, Buffer constants)
	{
		s_RendererAPI->DispatchCompute(renderCommandBuffer, computePass, material, workGroups, constants);
	}

	void Renderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<MaterialOld> material, const glm::uvec3& workGroups, uint32_t descrptorSetIndex, Buffer constants)
	{
		s_RendererAPI->DispatchCompute(renderCommandBuffer, computePass, material, workGroups, descrptorSetIndex, constants);
	}

	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	void Renderer::BindVertData(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> testVertexBuffer)
	{
		return s_RendererAPI->BindVertData(commandBuffer, testVertexBuffer);
	}
	void Renderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialOld> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		return s_RendererAPI->RenderStaticMeshWithMaterial(commandBuffer, pipeline, meshSource, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
	}
	void Renderer::RenderSkeletonMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialOld> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderSkeletonMeshWithMaterial(renderCommandBuffer, pipeline, meshSource, submeshIndex, material, transformBuffer, transformOffset, boneTransformsOffset, instanceCount, additionalUniforms);
	}
	void Renderer::DrawPrueVertex(Ref<RenderCommandBuffer> commandBuffer, uint32_t count)
	{
		return s_RendererAPI->DrawPrueVertex(commandBuffer, count);
	}
	void Renderer::RenderThreadFunc(RenderThread* renderThread)
	{
		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}
	void Renderer::WaitAndRender(RenderThread* renderThread)
	{
		// Wait for kick, then set render thread to busy
		{
			// 渲染线程循环等待Kick信号，收到信号后，设置为Busy信号并开始工作
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
		}
		// 工作就是把缓存命令全部执行
		s_CommandQueue[GetRenderQueueIndex()]->Execute();  // ???：感觉有问题，这怎么总在执行下一帧的命令（解决：他在执行NextFrame（）时，切换到了下一帧，这里再切换一下又回去了，所以这套逻辑只在一共2个缓冲区的时候没问题）再次解决：NextFrame已经切换了缓冲区，这里要执行的是上一个缓冲区，所以要切换，不过还是再能有2个缓冲区这代码才能跑

		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);
	}
	uint32_t Renderer::GetRenderQueueIndex()
	{
		return (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}
	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}
	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}
	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetCurrentFrameIndex();
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return Application::Get().GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}
}
