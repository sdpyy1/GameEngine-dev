#pragma once
#include "RenderPass.h"
#include "Hazel/Renderer/RenderResource/Shader.h"
#include <Hazel/Renderer/RenderResource/Texture.h>
namespace GameEngine {
	struct EnvironmentMap
	{
		bool hasPreCompute = false;
		TextureRef HDRTexture;
		TextureRef LutTexture;
		RHITextureRef IrradianceMap;
		RHITextureRef PreFilterMap;
		RHITextureRef CubeMap;
	};
	class IBLPass: public RenderPass
	{
	public:
		IBLPass() = default;
		~IBLPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "IBLPass"; }
		virtual PassType GetType() override final { return GRID_PASS; }
		void LoadEnv(std::string iblPath,std::string customKey = "");
	private:
		TextureRef Lut;

		bool hasPreCompute = false;   // 控制只有第一帧计算


		std::unordered_map<std::string, EnvironmentMap> environmentMaps;


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
