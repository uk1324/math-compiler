#include "ffiUtils.hpp"
#include "utils/asserts.hpp"
#include <immintrin.h>

float callSimd(void* function, std::span<const float> inputs) {
	const auto arity = inputs.size();
	switch (arity) {
	case 0:
		return reinterpret_cast<__m256(*)()>(function)().m256_f32[0];
	case 1: {
		__m256 a1 = _mm256_set1_ps(inputs[0]);
		return reinterpret_cast<__m256(*)(__m256)>(function)(a1).m256_f32[0];
	}
	case 2: {
		__m256 a1 = _mm256_set1_ps(inputs[0]);
		__m256 a2 = _mm256_set1_ps(inputs[1]);
		return reinterpret_cast<__m256(*)(__m256, __m256)>(function)(a1, a2).m256_f32[0];
	}

	default:
		ASSERT_NOT_REACHED();
		break;
	}

	return 0.0f;
}
