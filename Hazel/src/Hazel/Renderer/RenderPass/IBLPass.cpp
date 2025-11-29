#include "hzpch.h"
#include "IBLPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
namespace GameEngine
{
	void IBLPass::Init()
	{
		SetEnable(true);
		{
			equirectangularConversionCompShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "EquirectangularToCubeMap.comp.spv", SHADER_FREQUENCY_COMPUTE);
			RHIRootSignatureInfo rootSignatureInfo = {};
			rootSignatureInfo.AddEntryFromReflect(equirectangularConversionCompShader->GetRHIShader()).AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo());
			equirectangularConversionCompRootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);
			RHIComputePipelineInfo pipelineInfo = {};
			pipelineInfo.rootSignature = equirectangularConversionCompRootSignature;
			pipelineInfo.computeShader = equirectangularConversionCompShader->GetRHIShader();
			equirectangularConversionCompPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{
			environmentIrradianceCompShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "EnvironmentIrradiance.comp.spv", SHADER_FREQUENCY_COMPUTE);
			RHIRootSignatureInfo rootSignatureInfo = {};
			rootSignatureInfo.AddEntryFromReflect(environmentIrradianceCompShader->GetRHIShader())
				.AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo())
				.AddPushConstant({4 ,SHADER_FREQUENCY_COMPUTE });
			environmentIrradianceCompRootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);
            RHIComputePipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = environmentIrradianceCompRootSignature;
            pipelineInfo.computeShader = environmentIrradianceCompShader->GetRHIShader();
            environmentIrradianceCompPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		{
            environmentMipFilterCompShader = std::make_shared<V2::Shader>(APP_SHADER_PATH + "EnvironmentMipFilter.comp.spv", SHADER_FREQUENCY_COMPUTE);
            RHIRootSignatureInfo rootSignatureInfo = {};
            rootSignatureInfo.AddEntryFromReflect(environmentMipFilterCompShader->GetRHIShader())
                .AddEntry(RENDER_RESOURCEMANAGER->GetSamplerRootSignature()->GetInfo())
                .AddPushConstant({ 4, SHADER_FREQUENCY_COMPUTE });
            environmentMipFilterCompRootSignature = APP_DYNAMICRHI->CreateRootSignature(rootSignatureInfo);
            RHIComputePipelineInfo pipelineInfo = {};
            pipelineInfo.rootSignature = environmentMipFilterCompRootSignature;
            pipelineInfo.computeShader = environmentMipFilterCompShader->GetRHIShader();
            environmentMipFilterCompPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
		}

		V2::TextureSpec spec;
		spec.path = APP_HDR_PATH + "6.hdr";
		HDRTexture = std::make_shared<V2::Texture>(spec);

		spec.path = APP_TEXTURE_PATH + "BRDF_LUT.png";
		spec.srgb = false;
		Lut = std::make_shared<V2::Texture>(spec);


		{
			RHITextureInfo rhiTextureInfo;
			rhiTextureInfo.extent = { 1024, 1024, 1 };
			rhiTextureInfo.format = FORMAT_R32G32B32A32_SFLOAT;
			rhiTextureInfo.mipLevels = rhiTextureInfo.extent.MipSize();
			rhiTextureInfo.arrayLayers = 6;
			rhiTextureInfo.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RW_TEXTURE | RESOURCE_TYPE_TEXTURE_CUBE;
			CubeMap = APP_DYNAMICRHI->CreateTexture(rhiTextureInfo);
			PreFilterMap = APP_DYNAMICRHI->CreateTexture(rhiTextureInfo);
		}

		{
			RHITextureInfo rhiTextureInfo;
			rhiTextureInfo.extent = { 32, 32, 1 };
            rhiTextureInfo.format = FORMAT_R32G32B32A32_SFLOAT;
            rhiTextureInfo.mipLevels = 1;
            rhiTextureInfo.arrayLayers = 6;
            rhiTextureInfo.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RW_TEXTURE | RESOURCE_TYPE_TEXTURE_CUBE;

			IrradianceMap = APP_DYNAMICRHI->CreateTexture(rhiTextureInfo);
		}
	}

	void IBLPass::Build(RDGBuilder& builder)
	{
		if (IsEnabled()) {
			auto [w, h] = APP_WINDOWSIZE;
			if (!hasPreCompute) {
				RDGTextureHandle hdr = builder.CreateTexture("HDR")
					.Import(HDRTexture->GetRHITexture(), RESOURCE_STATE_SHADER_RESOURCE)
					.Finish();
				RDGTextureHandle cubeMap = builder.CreateTexture("CubeMap")
					.Import(CubeMap, RESOURCE_STATE_UNDEFINED)
					.Finish();


				RDGTextureHandle prefilterMap = builder.CreateTexture("PrefilterMap")
					.Import(PreFilterMap, RESOURCE_STATE_UNDEFINED)
					.Finish();


				RDGTextureHandle irradianceMap = builder.CreateTexture("IrradianceMap")
					.Import(IrradianceMap, RESOURCE_STATE_UNDEFINED)
					.Finish();
				// 从HDR生成Prefilter的Mip0
				builder.CreateComputePass(GetName() + "/HDR->PrefilterMapMip0")
					.Read(0, 1, 0, hdr)
					.RootSignature(equirectangularConversionCompRootSignature)
					.ReadWrite(0, 0, 0, prefilterMap, VIEW_TYPE_CUBE)
					.Execute([&](RDGPassContext context)
						{
							RHICommandListRef command = context.command;
							command->SetComputePipeline(equirectangularConversionCompPipeline);
							command->BindDescriptorSet(context.descriptors[0], 0);
							command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
							command->Dispatch(1024 / 32, 1024 / 32, 6);
						})
					.Finish();
				// 把prefilter的mip0拷贝到CubeMap，并生成mip，后续prefilter计算mip时会用到
				builder.CreateCopyPass(GetName() + "/HDR->CubeMap")
					.From(prefilterMap)
					.To(cubeMap)
					.GenerateMips()
					.OutputRead(cubeMap)
					.OutputReadWrite(prefilterMap)
					.Finish();


				builder.CreateComputePass(GetName() + "/IrradianceMap")
					.Read(0, 1, 0, cubeMap,VIEW_TYPE_CUBE)
					.ReadWrite(0, 0, 0, irradianceMap, VIEW_TYPE_CUBE)
					.RootSignature(environmentIrradianceCompRootSignature)
					.Execute([&](RDGPassContext context)
						{
							RHICommandListRef command = context.command;
							command->SetComputePipeline(environmentIrradianceCompPipeline);
							command->BindDescriptorSet(context.descriptors[0], 0);
							command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);
							uint32_t samleperCount = 1;    // 配好IBL后这里要改为512
							command->PushConstants(&samleperCount, sizeof(uint32_t), SHADER_FREQUENCY_COMPUTE);
							command->Dispatch(32 / 32, 32 / 32, 6);
						})
					.Finish();


				uint32_t mipLevels = (uint32_t)(std::floor(std::log2(std::max(1024, 1024)))) + 1;
				const float deltaRoughness = 1.0f / glm::max((float)mipLevels - 1.0f, 1.0f);
				for (uint32_t i = 1, size = 1024; i < mipLevels; i++, size /= 2) {
					// 注意外部定义的变量只能拿进来初始值
					builder.CreateComputePass(GetName() + "/PrefilterMapMip" + std::to_string(i))
						.Read(0, 1, 0, cubeMap, VIEW_TYPE_CUBE)
						.PassIndex(size, i)
						.ReadWrite(0, 0, 0, prefilterMap, VIEW_TYPE_CUBE, { TEXTURE_ASPECT_COLOR, i, 1, 0, 6 })   // TODO: FIX 资源屏障流程
						.RootSignature(environmentMipFilterCompRootSignature)
						.Execute([&](RDGPassContext context)
							{
								RHICommandListRef command = context.command;
								command->SetComputePipeline(environmentMipFilterCompPipeline);
								command->BindDescriptorSet(context.descriptors[0], 0);
								command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetSamplerDescriptorSet(), 1);

								uint32_t numGroups = glm::max(1u, context.passIndex[0] / 32);
								float roughness = context.passIndex[1] * 0.1; // TODO: FIX  0.1是上边计算的deltaRoughness，没传递进来
								command->PushConstants(&roughness, sizeof(float), SHADER_FREQUENCY_COMPUTE);
								command->Dispatch(numGroups, numGroups, 6);
							})
						.Finish();
				}
                hasPreCompute = true;
			}
			else {
				// 执行过一次后 布局变了
				RDGTextureHandle hdr = builder.CreateTexture("HDR")
					.Import(HDRTexture->GetRHITexture(), RESOURCE_STATE_SHADER_RESOURCE)
					.Finish();
				RDGTextureHandle cubeMap = builder.CreateTexture("CubeMap")
					.Import(CubeMap, RESOURCE_STATE_SHADER_RESOURCE)
					.Finish();
				RDGTextureHandle prefilterMap = builder.CreateTexture("PrefilterMap")
					.Import(PreFilterMap, RESOURCE_STATE_SHADER_RESOURCE)
					.Finish();
				RDGTextureHandle irradianceMap = builder.CreateTexture("IrradianceMap")
					.Import(IrradianceMap, RESOURCE_STATE_SHADER_RESOURCE)
					.Finish();
			}
		}
	}













}