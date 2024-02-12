#include "floatingPointIndentityTest.hpp"
#include "../floatingPoint.hpp"
#include "../utils/put.hpp"
#include <limits>
#include <vector>
#include <span>

#pragma float_control(precise, on, push)
#pragma STDC FENV_ACCESS ON

#include <bitset>
#include <cfenv>

int ROUNDING_MODES[] = {
	FE_DOWNWARD,
	FE_TONEAREST,
	FE_TOWARDZERO,
	FE_UPWARD
};

const char* roundingModeName(int roundingMode) {
	switch (roundingMode) {
	case FE_DOWNWARD: return "DOWNWARD";
	case FE_TONEAREST: return "TONEAREST";
	case FE_TOWARDZERO: return "TOWARDZERO";
	case FE_UPWARD: return "UPWARD";
	}
	return "INVALID";
}

float TEST_ARGUMENTS[] = {
	0.2f, // Exacly representable
	0.123456789123456789f, // Try to cause rounding
	1000000000, // Big
	0.000000001, // Small
	std::numeric_limits<float>::lowest(),
	std::numeric_limits<float>::denorm_min(),
	std::numeric_limits<float>::min(),
	std::numeric_limits<float>::max(),
	std::numeric_limits<float>::quiet_NaN(),
	std::numeric_limits<float>::signaling_NaN(),
	0.0f,
	-0.0f,
	std::numeric_limits<float>::infinity(),
	-std::numeric_limits<float>::infinity()
};

const auto TEST_ARGUMENTS_COUNT = std::size(TEST_ARGUMENTS);

template<typename Function1, typename Function2>
void compare(Function1 f1, Function2 f2, i32 argumentCount, bool bitwiseEqualityDisabled = false) {

	std::vector<i32> indices;
	std::vector<float> arguments;
	indices.resize(argumentCount, 0);
	arguments.resize(argumentCount, 0.0f);
	for (const auto& roudingMode : ROUNDING_MODES) {
		for (;;) {
			for (i32 i = 0; i < argumentCount; i++) {
				arguments[i] = TEST_ARGUMENTS[indices[i]];
			}

			const float x1 = f1(arguments);
			const float x2 = f2(arguments);

			const auto normalEquality = (x1 == x2) || (isnan(x1) && isnan(x2));
			const auto bitwiseEquality = f32BitwiseEquals(x1, x2) || bitwiseEqualityDisabled;

			if (!normalEquality) {
				put("== not true");
			}
			if (!bitwiseEquality) {
				put("bitwise equality not true");
			}
			if (!normalEquality || !bitwiseEquality) {
				put("rounding mode = %", roundingModeName(roudingMode));
				put("arguments: ");
				for (i32 i = 0; i < argumentCount; i++) {
					put("[%] = %", i, arguments[i]);
				};
				put("lhs = %", x1);
				put("rhs = %", x2);
				if (!bitwiseEquality) {
					put("lhs = %", std::bitset<32>(std::bit_cast<u32>(x1)));
					put("rhs = %", std::bitset<32>(std::bit_cast<u32>(x2)));
				}
			}

			if (argumentCount > 0) {
				indices[0]++;
			}
			for (i32 argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++) {
				if (indices[argumentIndex] < TEST_ARGUMENTS_COUNT) {
					break;
				}

				if (argumentIndex == argumentCount - 1) {
					goto end;
				}

				indices[argumentIndex + 1]++;
				indices[argumentIndex] = 0;
			}
		}
	}
	end:;
	/*for (int i = 0; i < )*/
}

void testFloatingPointIdentites() {
	/*compare(
		[](std::span<const float> a) -> float { return a[0] + 0.0f; },
		[](std::span<const float> a) -> float { return a[0]; },
		1
	);*/

	/*compare(
		[](std::span<const float> a) -> float { return a[0] + a[1]; },
		[](std::span<const float> a) -> float { return a[1] + a[0]; },
		2
	);*/

	//compare(
	//	[](std::span<const float> a) -> float { return a[0] + -0.0f; },
	//	[](std::span<const float> a) -> float { return a[0]; },
	//	1
	//);

	//compare(
	//	[](std::span<const float> a) -> float { return a[0] + -a[1]; },
	//	[](std::span<const float> a) -> float { return a[0] - a[1]; },
	//	2,
	//	true
	//);

	//compare(
	//	[](std::span<const float> a) -> float { return a[0] - a[0]; },
	//	[](std::span<const float> a) -> float { return 0.0f; },
	//	1
	//);

	compare(
		[](std::span<const float> a) -> float { return a[0] + a[0]; },
		[](std::span<const float> a) -> float { return 2.0f * a[0]; },
		1
	);
}

#pragma float_control(pop)