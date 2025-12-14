#pragma once
#include "RenderPass.h"
namespace GameEngine {
	class PresentPass : public RenderPass
	{
	public:
		PresentPass() {};
		~PresentPass() {};

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() override final { return "PresentPass"; }

		virtual PassType GetType() override final { return PRESENT_PASS; }

	private:
	};


}