#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include <chrono>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>

namespace GameEngine::Math {
	
	class RandomStream
	{
	public:
		// 用固定整数种子初始化
		RandomStream()
			: InitialSeed(0)
			, Seed(0)
		{
		}

		explicit RandomStream(int32_t inSeed)
		{
			Initialize(inSeed);
		}

		// 用字符串初始化（字符串哈希作为种子）
		explicit RandomStream(const std::string& name)
		{
			Initialize(name);
		}

		// 用固定整数种子初始化
		void Initialize(int32_t inSeed)
		{
			InitialSeed = inSeed;
			Seed = static_cast<uint32_t>(inSeed);
		}

		// 用字符串初始化；若传入空字符串，则使用当前时间作为种子（不可复现）
		void Initialize(const std::string& name)
		{
			if (!name.empty())
			{
				InitialSeed = static_cast<int32_t>(std::hash<std::string>{}(name));
			}
			else
			{
				InitialSeed = static_cast<int32_t>(
					std::chrono::steady_clock::now().time_since_epoch().count());
			}
			Seed = static_cast<uint32_t>(InitialSeed);
		}

		// 将种子重置为初始值，从而重新生成完全相同的序列
		void Reset() const
		{
			Seed = static_cast<uint32_t>(InitialSeed);
		}

		int32_t GetInitialSeed() const
		{
			return InitialSeed;
		}

		// 重新生成一个全新的种子（基于全局随机）
		void GenerateNewSeed()
		{
			Initialize(static_cast<int32_t>(FRand()));
		}

		// 返回 [0, 1) 的浮点数
		float GetFraction() const
		{
			MutateSeed();

			float result;
			*reinterpret_cast<uint32_t*>(&result) = 0x3F800000U | (Seed >> 9);
			return result - 1.0f;
		}

		// 返回 [0, MAXUINT) 的无符号整数
		uint32_t GetUnsignedInt() const
		{
			MutateSeed();
			return Seed;
		}

		// 返回当前种子
		int32_t GetCurrentSeed() const
		{
			return static_cast<int32_t>(Seed);
		}

		// 返回 [0, 1) 的浮点数（与 UE FMath 风格一致）
		float FRand() const
		{
			return GetFraction();
		}

		// Rand() 为 FRand() 的别名，方便两种命名风格混用
		float Rand() const
		{
			return FRand();
		}

		// 返回 [0, A) 的整数
		int32_t RandHelper(int32_t a) const
		{
			return (a > 0) ? static_cast<int32_t>(std::trunc(GetFraction() * static_cast<float>(a))) : 0;
		}

		// 返回 [Min, Max] 的整数
		int32_t RandRange(int32_t min, int32_t max) const
		{
			const int32_t range = (max - min) + 1;
			return min + RandHelper(range);
		}

		// 返回 [inMin, inMax] 的浮点数
		float FRandRange(float inMin, float inMax) const
		{
			return inMin + (inMax - inMin) * Rand();
		}

		// 返回单位随机向量
		glm::vec3 GetUnitVector() const
		{
			glm::vec3 result;
			float lengthSq;

			do
			{
				// 在单位球内采样，使结果统计上均匀
				result.x = GetFraction() * 2.f - 1.f;
				result.y = GetFraction() * 2.f - 1.f;
				result.z = GetFraction() * 2.f - 1.f;
				lengthSq = glm::dot(result, result);
			} while (lengthSq > 1.f || lengthSq < 1e-8f);

			return glm::normalize(result);
		}

		// 返回单位随机向量（与 FMath 风格一致的别名）
		glm::vec3 VRand() const
		{
			return GetUnitVector();
		}

		// 返回盒子 [min, max] 内的随机点
		glm::vec3 RandPointInBox(const glm::vec3& min, const glm::vec3& max) const
		{
			return glm::vec3(
				FRandRange(min.x, max.x),
				FRandRange(min.y, max.y),
				FRandRange(min.z, max.z));
		}

		// 返回以 dir 为中心、半锥角为 coneHalfAngleRad 的锥形内均匀分布的随机单位向量
		glm::vec3 VRandCone(const glm::vec3& dir, float coneHalfAngleRad) const
		{
			if (coneHalfAngleRad > 0.f)
			{
				const float randU = Rand();
				const float randV = Rand();

				// 在单位球上均匀取点
				const float theta = 2.f * glm::pi<float>() * randU;
				float phi = std::acos((2.f * randV) - 1.f);
				phi = std::fmod(phi, coneHalfAngleRad);

				glm::vec3 zAxis = glm::normalize(dir);
				// 选取与 zAxis 不共线的参考轴，构造正交基
				glm::vec3 ref = (std::abs(zAxis.z) < 0.999f) ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(1.f, 0.f, 0.f);
				glm::vec3 xAxis = glm::normalize(glm::cross(ref, zAxis)); // 对应 UE 的 DirY
				glm::vec3 yAxis = glm::cross(zAxis, xAxis);

				glm::vec3 result = zAxis;
				result = glm::angleAxis(phi, yAxis) * result;
				result = glm::angleAxis(theta, zAxis) * result;
				return glm::normalize(result);
			}
			return glm::normalize(dir);
		}

		// 返回以 dir 为中心、水平/垂直半锥角分别为对应值的锥形内均匀分布的随机单位向量
		glm::vec3 VRandCone(const glm::vec3& dir, float horizontalConeHalfAngleRad, float verticalConeHalfAngleRad) const
		{
			if ((verticalConeHalfAngleRad > 0.f) && (horizontalConeHalfAngleRad > 0.f))
			{
				const float randU = Rand();
				const float randV = Rand();

				const float theta = 2.f * glm::pi<float>() * randU;
				float phi = std::acos((2.f * randV) - 1.f);

				// 由 theta 决定当前角度下的椭圆锥半角
				float coneHalfAngleRad =
					std::pow(std::cos(theta) / verticalConeHalfAngleRad, 2.f) +
					std::pow(std::sin(theta) / horizontalConeHalfAngleRad, 2.f);
				coneHalfAngleRad = std::sqrt(1.f / coneHalfAngleRad);

				phi = std::fmod(phi, coneHalfAngleRad);

				glm::vec3 zAxis = glm::normalize(dir);
				glm::vec3 ref = (std::abs(zAxis.z) < 0.999f) ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(1.f, 0.f, 0.f);
				glm::vec3 xAxis = glm::normalize(glm::cross(ref, zAxis));
				glm::vec3 yAxis = glm::cross(zAxis, xAxis);

				glm::vec3 result = zAxis;
				result = glm::angleAxis(phi, yAxis) * result;
				result = glm::angleAxis(theta, zAxis) * result;
				return glm::normalize(result);
			}
			return glm::normalize(dir);
		}

		std::string ToString() const
		{
			return "RandomStream(InitialSeed=" + std::to_string(InitialSeed) +
				", Seed=" + std::to_string(Seed) + ")";
		}

	protected:
		// 将当前种子变异为下一个种子（线性同余生成器）
		void MutateSeed() const
		{
			Seed = (Seed * 196314165U) + 907633515U;
		}

	private:
		int32_t InitialSeed = 0;
		mutable uint32_t Seed = 0;
	};

} // namespace GameEngine::Math
