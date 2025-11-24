#pragma once

#include "Hazel/Renderer/old/RendererAPI.h"

#include "Hazel/Renderer/old/Shader.h"
#include "Hazel/Renderer/old/RenderContext.h"
#include "Hazel/Core/Application.h"
#include "RendererConfig.h"
#include <Hazel/Core/RenderThread.h>
#include "RenderCommandQueue.h"
#include "RendererCapabilities.h"
#include "IndexBuffer.h"
#include "ComputePass.h"

namespace GameEngine {
	class ShaderLibrary;
	class MeshSource;
	// 管理命令缓冲区的调度和渲染相关的初始资源
	class Renderer
	{
	public:
		static RendererConfig& GetConfig();
		static void Init();
		static void Shutdown();

		static Ref<ShaderLibrary> GetShaderLibrary();

		static void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
		static void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer);

		static void BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass);
		static void EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass);
		static void DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<MaterialOld> material, const glm::uvec3& workGroups, Buffer constants = Buffer());
		static void DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<MaterialOld> material, const glm::uvec3& workGroups, uint32_t descrptorSetIndex, Buffer constants = Buffer());

		static void SwapQueues(); // 交换Buffer命令缓冲
		static void BindVertData(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> testVertexBuffer);
		static void RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialOld> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Buffer additionalUniforms = Buffer());
		static void RenderSkeletonMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialOld> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount, Buffer additionalUniforms = Buffer());
		static void DrawPrueVertex(Ref<RenderCommandBuffer> commandBuffer, uint32_t count);
		static RendererAPI::Type Current() { return RendererAPI::Current(); }
		static void RenderThreadFunc(RenderThread* renderThread);
		static void WaitAndRender(RenderThread* renderThread);
		static uint32_t GetRenderQueueIndex();
		static void BeginFrame();
		static void EndFrame();

		template<typename FuncT>
		static void Submit(FuncT&& func, const char* file = nullptr, int line = 0, const char* function = nullptr)
		{
			auto& queue = GetRenderCommandQueue();
#ifdef RTDEBUG
			queue.m_DebugInfos.push_back({ file, line, function });
#endif
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();
				pFunc->~FuncT();
				};

			auto storageBuffer = queue.Allocate(renderCmd, sizeof(FuncT));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
				};

			if (RenderThread::IsCurrentThreadRT())
			{
				const uint32_t index = Renderer::RT_GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			}
			else
			{
				const uint32_t index = Renderer::GetCurrentFrameIndex();
				Submit([renderCmd, func, index]()
					{
						auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
						new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
					});
			}
		}
		static RenderCommandQueue& GetRenderCommandQueue();
		static RenderCommandQueue& GetRenderResourceReleaseQueue(uint32_t index);
		static RendererCapabilities& GetCapabilities();
		static uint32_t GetCurrentFrameIndex();
		static uint32_t RT_GetCurrentFrameIndex();


		static Ref<Texture2D> GetWhiteTexture();
		static Ref<TextureCube> GetBlackCubeTexture();
		static Ref<Texture2D> GetBRDFLutTexture();
		static Ref<Texture2D> GetBlackTexture();
	};
}
