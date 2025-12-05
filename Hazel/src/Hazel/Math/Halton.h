#pragma once
#include <vector>
#include <glm/ext/vector_float2.hpp>
#include <stdexcept>
#include <utility>

namespace GameEngine {
	class Halton
	{
	private:
		static const int PRIMES[];
		static const int PRIME_COUNT;

		static std::vector<double> generate_halton_sequence(int index, int dim)
		{
			std::vector<int> t(dim, index);
			std::vector<double> prime_inv(dim);
			std::vector<double> r(dim, 0.0);

			for (int j = 0; j < dim; ++j)
			{
				prime_inv[j] = 1.0 / static_cast<double>(PRIMES[j]);
			}

			auto sum_t = [&t]() {
				int s = 0;
				for (int v : t) s += v;
				return s;
				};

			while (sum_t() > 0)
			{
				for (int j = 0; j < dim; ++j)
				{
					int d = t[j] % PRIMES[j];
					r[j] += static_cast<double>(d) * prime_inv[j];
					prime_inv[j] /= static_cast<double>(PRIMES[j]);
					t[j] = t[j] / PRIMES[j];
				}
			}

			return r;
		}

		static std::vector<std::vector<double>> precompute_halton_table(int table_size, int dim)
		{
			std::vector<std::vector<double>> table;
			table.reserve(table_size);
			for (int i = 1; i <= table_size; ++i)
			{
				table.push_back(generate_halton_sequence(i, dim));
			}
			return table;
		}

		static std::vector<double> get_halton_by_tick(int tick, const std::vector<std::vector<double>>& halton_table)
		{
			int table_idx = tick % static_cast<int>(halton_table.size());
			if (table_idx < 0)
			{
				table_idx += static_cast<int>(halton_table.size());
			}

			return halton_table[table_idx];
		}

	public:
		static glm::vec2 GetTAAJetter(uint32_t tick, double scale = 1.0, int table_size = 64)
		{
			static std::vector<std::vector<double>> halton_2d_table = precompute_halton_table(table_size, 2);

			std::vector<double> seq = get_halton_by_tick(static_cast<int>(tick), halton_2d_table);

			// 直接将Halton序列[0,1)映射到[-0.5, 0.5)区间，scale默认1.0保证范围
			double x = (seq[0] - 0.5) * scale;
			double y = (seq[1] - 0.5) * scale;

			return glm::vec2(static_cast<float>(x), static_cast<float>(y));
		}
	};

	const int Halton::PRIMES[] = { 2, 3, 5, 7 };
	const int Halton::PRIME_COUNT = sizeof(Halton::PRIMES) / sizeof(Halton::PRIMES[0]);
} // namespace GameEngine