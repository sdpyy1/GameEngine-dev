#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class DDGIPass : public RenderPassNew
	{
	public:
		DDGIPass() = default;
		~DDGIPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "DDGIPass"; }
		virtual PassType GetType() override final { return DDGI_PASS; }
	private:
		
	};
}


