#pragma once
#include "RenderPass.h"
#include "Hazel/Renderer/RenderResource/Shader.h"
#include <Hazel/Renderer/RenderResource/Texture.h>
namespace GameEngine {

	class IBLPass: public RenderPassNew
	{
	public:
		IBLPass() = default;
		~IBLPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "IBLPass"; }
		virtual PassType GetType() override final { return GRID_PASS; }

	private:
		TextureRef HDRTexture;
		TextureRef Lut;

		bool hasPreCompute = false;   // 控制只有第一帧计算

		RHITextureRef IrradianceMap;
		RHITextureRef PreFilterMap;
		RHITextureRef CubeMap;




		ShaderRef equirectangularConversionCompShader;
		RHIRootSignatureRef equirectangularConversionCompRootSignature;
		RHIComputePipelineRef equirectangularConversionCompPipeline;



		ShaderRef environmentIrradianceCompShader;
        RHIRootSignatureRef environmentIrradianceCompRootSignature;
        RHIComputePipelineRef environmentIrradianceCompPipeline;


		ShaderRef preFilterCompShader;
		ShaderRef environmentMipFilterCompShader;
		RHIRootSignatureRef environmentMipFilterCompRootSignature;
		RHIComputePipelineRef environmentMipFilterCompPipeline;
	};







}
