#include "simdFunctionsTest.hpp"
#include "../utils/put.hpp"
#include "../simdFunctions.hpp"
#include <cmath>
#include <optional>
#include <iomanip>

float expTest(float x) {
	return expSimd(_mm256_set1_ps(x)).m256_f32[0];
}

float lnTest(float x) {
	return lnSimd(_mm256_set1_ps(x)).m256_f32[0];
}

template<typename T>
T lerp(T a, T b, T t) {
	return a * (1 - t) + b * t;
}

long double f32UlpError(float approximation, long double correct) {
	const long double significand = (long double)(f32GetSignificand(approximation));
	const i32 exponent = f32GetExponent(approximation);
	return std::abs(significand - (correct / exp2l(exponent))) * exp2l(F32_SIGNIFICAND_PRECISON_BITS - 1);
}

std::optional<i32> f32NumbersBetween(float a, float b) {
	if (!isfinite(a) || !isfinite(b)) {
		return std::nullopt;
	}

	const u32 aBits = std::bit_cast<u32>(a);
	const u32 bBits = std::bit_cast<u32>(b);

	// Floats are ordered the same way as ints. When the significand overflows the bit gets added to the exponent. That is when the significand reached 2 then 1 gets added to the expoenent and the significand is set to 1 (all zeros represent a one because the most signicand hidden bit is always 1). This doesn't mean that floating point addition is the same as integer addition (You can't simply add the exponents).
	if (aBits > bBits) {
		return i32(aBits - bBits);
	} else {
		return i32(bBits - aBits);
	}
}

#include <bitset>
#include <cfenv>

//template<typename Function>
//void testWithAllRoundingModes() {
//#pragma STDC FENV_ACCESS ON
//	/*volatile float a = 0.1f;
//	volatile float b = 0.2f;*/
//	volatile float a = 0.0f;
//	volatile float b = 0.0f;
//
//	int ROUNDING_MODES[] = {
//		FE_DOWNWARD,
//		FE_TONEAREST,
//		FE_TOWARDZERO,
//		FE_UPWARD
//	};
//
//	for (const auto& roudingMode : ROUNDING_MODES) {
//		std::fesetround(roudingMode);
//		put("%", a - b);
//	}
//}

void testFunction(
	long double (*correctFunction)(long double),
	float (*approximationFunction)(float),
	long double intervalStart,
	long double intervalEnd,
	i64 stepCount) {
	
	const auto oldPrecision = std::cout.precision();
	std::cout << std::setprecision(100000);

	long double maxAbsoluteError = -std::numeric_limits<long double>::infinity();
	float maxAbsoluteErrorInput = std::numeric_limits<float>::quiet_NaN();
	long double maxRelativeError = -std::numeric_limits<long double>::infinity();
	float maxRelativeErrorInput = std::numeric_limits<float>::quiet_NaN();
	long double maxUlpError = -std::numeric_limits<long double>::infinity();
	float maxUlpErrorInput = std::numeric_limits<float>::quiet_NaN();
	i32 maxNumbersBetweenError = 0;
	float maxNumbersBetweenErrorInput = std::numeric_limits<float>::quiet_NaN();

	for (i32 i = 0; i <= stepCount - 1; i++) {
		const long double t = (long double)(i) / stepCount;
		const float x = float(lerp<long double>(intervalStart, intervalEnd, t));
		const long double correct = correctFunction((long double)(x));
		const long double approximation = (long double)(approximationFunction(x));

		if (std::isnan(correct) && !std::isnan(approximation)) {
			put("Number is not NaN on input %.", x);
			continue;
		}

		if (std::isinf(correct)) {
			if (correct != approximation) {
				put("Incorect value on input %. Expected %, found %", x, correct, approximation);
			}
			continue;
		}

		const long double absoluteError = std::fabsl(correct - approximation);
		if (absoluteError > maxAbsoluteError) {
			maxAbsoluteError = absoluteError;
			maxAbsoluteErrorInput = x;
		}

		if (correct != 0.0f) {
			const long double relativeError = absoluteError / correct;
			if (relativeError > maxRelativeError) {
				maxRelativeError = relativeError;
				maxRelativeErrorInput = x;
			}
		}

		const long double ulpError = f32UlpError(approximation, correct);
		if (ulpError > maxUlpError) {
			maxUlpError = ulpError;
			maxUlpErrorInput = x;
		}

		const std::optional<i32> numberBetweenError = f32NumbersBetween(approximation, float(correct));
		if (numberBetweenError.has_value() && *numberBetweenError > maxNumbersBetweenError) {
			maxNumbersBetweenError = *numberBetweenError;
			maxNumbersBetweenErrorInput = x;
		}
	}

	auto additionalInfo = [&](float input) {
		const float approximation = approximationFunction(input);
		const long double correct = correctFunction((long double)(input));
		put("correct       = %", correct);
		put("approximation = %", approximation);
		const auto numbersBetween = f32NumbersBetween(approximation, float(correct));
		if (!numbersBetween.has_value()) {
			return;
		}
		put("numbers between = %", *numbersBetween);
	};
	put("max relative error % on x = %", maxRelativeError, maxRelativeErrorInput);
	additionalInfo(maxRelativeErrorInput);
	std::cout << '\n';
	put("max absolute error % on x = %", maxAbsoluteError, maxAbsoluteErrorInput);
	additionalInfo(maxAbsoluteErrorInput);
	std::cout << '\n';
	put("max number between error % on x = %", maxNumbersBetweenError, maxNumbersBetweenErrorInput);
	// Not sure if the ulp calculation is correct (not sure if the code is correct and even if it is there might still be issue due to rounding errors).
	//put("max ulp error % on x = %", maxUlpError, maxUlpErrorInput);

	std::cout << std::setprecision(oldPrecision);
}

void testSimdFunctions() {
	//std::cout << std::setprecision(100000);
	//const auto a = f32Make(0, u32(-5 + i32(F32_EXPONENT_BIAS)), 0);
	//const auto b = f32Make(0, 20 + F32_EXPONENT_BIAS, 0);
	///*const auto a = f32Make(0, u32(-4 + i32(F32_EXPONENT_BIAS)), 0);
	//const auto b = f32Make(0, u32(-4 + i32(F32_EXPONENT_BIAS)), 0);*/
	//std::cout << a << '\n';
	//std::cout << b << '\n';
	//std::cout << a + b << '\n';
	//std::cout << std::bitset<32>(std::bit_cast<u32>(a)) << '\n';
	//std::cout << std::bitset<32>(std::bit_cast<u32>(b)) << '\n';
	//std::cout << std::bitset<32>(std::bit_cast<u32>(a + b)) << '\n';
	//std::cout << std::bitset<32>(std::bit_cast<u32>(a) + std::bit_cast<u32>(b)) << '\n';
	//std::cout << std::bit_cast<float>(std::bit_cast<u32>(a) + std::bit_cast<u32>(b)) << '\n';

	//std::cout << std::bit_cast<float>(std::bit_cast<u32>(f32Make(0, 16 + F32_EXPONENT_BIAS, 1)) + 10) << '\n';

	// One msvc some of the standard functions for lower precision just return cast values from the higher precision ones.

	auto printLine1 = [] {
		put("----------------------------------------------");
	};
	auto printLine2 = [] {
		put("**********************************************");
		put("**********************************************");
	};

	{
		const auto start = -40.0l;
		const auto end = 30.0l;
		put("expTest\n");
		testFunction(expl, expTest, start, end, 200000);
		printLine1();
		put("expf\n");
		testFunction(expl, expf, start, end, 200000);
		printLine2();
	}

	{
		const auto start = 0.00001f;
		/*const auto end = 200000.0f;*/
		const auto end = 2.0f;
		const auto sampleCount = 20000000;
		put("lnTest\n");
		testFunction(logl, lnTest, start, end, sampleCount);
		printLine1();
		put("logf\n");
		testFunction(logl, logf, start, end, sampleCount);
		printLine2();
	}
}
