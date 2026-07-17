#pragma once

#include <glm/glm.hpp>

namespace GameEngine::Math {
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);
	template<typename T,
		std::enable_if_t<std::is_integral_v<T>, int> = 0>
	inline static T DivideAndRoundUp(T dividend, T divisor) {
		return (dividend + divisor - 1) / divisor;
	}

	template<typename DivisorT,
		std::enable_if_t<std::is_integral_v<DivisorT>, int> = 0>
	inline static glm::uvec2 DivideAndRoundUp(glm::uvec2 dividend, DivisorT divisor) {
		// 转换除数为 uint32_t（与 glm::uvec2 的分量类型匹配）
		uint32_t div = static_cast<uint32_t>(divisor);
		return {
			DivideAndRoundUp(dividend.x, div),
			DivideAndRoundUp(dividend.y, div)
		};
	}

	/*
		因为最后一行肯定是0001，所以可以忽略
	*/
	static void ConvertGlmMat4To3x4Transform(const glm::mat4& mat, float* outTransform)
	{
		memset(outTransform, 0, sizeof(float) * 3 * 4);
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				size_t offset = static_cast<size_t>(row) * 4 + col;
				outTransform[offset] = mat[col][row];
			}
		}
	}


}
