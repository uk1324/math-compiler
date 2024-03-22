#pragma once

#include <immintrin.h>
#include "floatingPoint.hpp"

/*
Range reduction:
Find an integer k and r that is between -ln(2)/2 and ln(2)/2 such that x = k * ln(2) + r.
This can be done because if r = ln(2)/2 then k * ln(2) + ln(2)/2 = (k + 1) * ln(2) - ln(2)/2.

Calculating r:
k * ln(2) + r = x
r = x - k * ln(2)

Calculating function after range reduction:
e^(k * ln(2) + r) =
e^(k * ln(2)) * e^r =
2^k * e^r
*/
inline __m256 __vectorcall expSimd(__m256 x) {
	const auto minusLn2 = _mm256_set1_ps(-0.6931471805599453f);
	const auto ln2Inv = _mm256_set1_ps(1.4426950408889634f);

	const auto kFloat = _mm256_round_ps(_mm256_mul_ps(x, ln2Inv), _MM_FROUND_NO_EXC);
	const auto r = _mm256_fmadd_ps(kFloat, minusLn2, x);

	const auto a0 = _mm256_set1_ps(1.0000000754895593f);
	const auto a1 = _mm256_set1_ps(1.0000000647006064f);
	const auto a2 = _mm256_set1_ps(0.4999886914692487f);
	const auto a3 = _mm256_set1_ps(0.16666325650514743f);
	const auto a4 = _mm256_set1_ps(0.041917526523052265f);
	const auto a5 = _mm256_set1_ps(0.008381111717943628f);

	// exp(r) approximation
	__m256 m;
	m = _mm256_fmadd_ps(r, a5, a4);
	m = _mm256_fmadd_ps(r, m, a3);
	m = _mm256_fmadd_ps(r, m, a2);
	m = _mm256_fmadd_ps(r, m, a1);
	m = _mm256_fmadd_ps(r, m, a0);

	auto kInt = _mm256_cvtps_epi32(kFloat);
	auto exponent = _mm256_add_epi32(kInt, _mm256_set1_epi32(F32_EXPONENT_BIAS));
	// TODO: Maybe could do satured add instead of clamping. The probem is that it has to be able to handle negative values of k and I couldn't find an instruction that converts u32 into u8 in such a way that the u8 are stored in the lower bytes of the u32s.
	exponent = _mm256_max_epi32(_mm256_min_epi32(exponent, _mm256_set1_epi32(255)), _mm256_set1_epi32(0));
	const auto twoToK = _mm256_slli_epi32(exponent, F32_EXPONENT_SHIFT);
	return _mm256_mul_ps(_mm256_castsi256_ps(twoToK), m);
}

/*
Range reduction:
Find an integer k and f that is between 0 and 1 such that x = 2^k * (f + 1).
This can be done because if f = 0 then x = 2^k and if f = 1 then x = 2^k * (1 + 1) = 2^(k+1).

Calculating f:
x = 2^k * (f + 1)
x = 2^k * f + 2^k
x - 2^k = 2^k * f
(x - 2^k) / 2^k = f
x / 2^k - 1 = f

Calculating function after range reduction:
ln(x) =
ln(2^k * (f + 1)) =
ln(2^k) + ln(f + 1) =
log2(2^k)/log2(e) + ln(f + 1) =
k/log2(e) + ln(f + 1)

TODO: Apparently there is a way to range reduce further into (sqrt(2)/2, sqrt(2)). Used for example in fdlibm and also mentionted here https://math.stackexchange.com/questions/3619158/most-efficient-way-to-calculate-logarithm-numerically.
*/
inline __m256 __vectorcall lnSimd(__m256 x) {
	x = _mm256_max_ps(x, _mm256_set1_ps(0.0f));
	const auto xBytes = _mm256_castps_si256(x);
	// k is the exponent of x.
	// Calculate 2^k by just making away the mantissa and sign bits of x.
	const auto twoToKBytes = _mm256_and_epi32(xBytes, _mm256_set1_epi32(F32_EXPONENT_MASK));
	const auto twoToK = _mm256_castsi256_ps(twoToKBytes);
	// Bitshift k and debias to get the integer value of it.
	const auto kInt = _mm256_sub_epi32(_mm256_srli_epi32(twoToKBytes, F32_EXPONENT_SHIFT), _mm256_set1_epi32(F32_EXPONENT_BIAS));
	const auto k = _mm256_cvtepi32_ps(kInt);

	const auto f = _mm256_add_ps(_mm256_div_ps(x, twoToK), _mm256_set1_ps(-1.0f));

	////const auto a0 = _mm256_set1_ps(6.071404325574954e-05f);
	//const auto a0 = _mm256_set1_ps(0.0f);
	////const auto a1 = _mm256_set1_ps(0.9965407439405233f);
	//const auto a1 = _mm256_set1_ps(1.0f);
	//const auto a2 = _mm256_set1_ps(-0.46783477377051086f);
	//const auto a3 = _mm256_set1_ps(0.22089156008190866f);
	//const auto a4 = _mm256_set1_ps(-0.05657177777848718f);

	//// minimax log(f + 1) approximation
	//__m256 m;
	//m = _mm256_fmadd_ps(f, a4, a3);
	//m = _mm256_fmadd_ps(f, m, a2);
	//m = _mm256_fmadd_ps(f, m, a1);
	//m = _mm256_fmadd_ps(f, m, a0);

	//const auto a0 = _mm256_set1_ps(1.2793255078168223e-06);
	const auto a0 = _mm256_set1_ps(0.0f);
	const auto a1 = _mm256_set1_ps(0.9998615614234192f);
	//const auto a1 = _mm256_set1_ps(1.0f);
	const auto a2 = _mm256_set1_ps(-0.4975348624679797f);
	const auto a3 = _mm256_set1_ps(0.3164367089548567f);
	const auto a4 = _mm256_set1_ps(-0.19168345004150775f);
	const auto a5 = _mm256_set1_ps(0.08387237597258754f);
	const auto a6 = _mm256_set1_ps(-0.017807711932446457f);

	__m256 m;
	m = _mm256_fmadd_ps(f, a6, a5);
	m = _mm256_fmadd_ps(f, m, a4);
	m = _mm256_fmadd_ps(f, m, a3);
	m = _mm256_fmadd_ps(f, m, a2);
	m = _mm256_fmadd_ps(f, m, a1);
	m = _mm256_fmadd_ps(f, m, a0);

	const auto invLogBase2OfE = _mm256_set1_ps(0.69314718056f);
	return _mm256_fmadd_ps(k, invLogBase2OfE, m);
}

// https://stackoverflow.com/questions/16988199/how-to-choose-avx-compare-predicate-variants
// https://stackoverflow.com/questions/8627331/what-does-ordered-unordered-comparison-mean


inline __m256 __vectorcall powSimd(__m256 x, __m256 y) {
	//const auto yRounded = _mm256_round_ps(x, _MM_FROUND_NO_EXC);
	//const auto yInt = _mm256_castsi256_ps(x);
	//const auto isYIntMask = _mm256_cmp_ps(y, yRounded, _CMP_EQ_OQ);
	//const auto firstBitMask = _mm256_set1_epi32()
	//const auto yFirstBit
}

inline __m256 __vectorcall sinSimd(__m256 x) {
	return _mm256_sin_ps(x);
}

inline __m256 __vectorcall cosSimd(__m256 x) {
	return _mm256_cos_ps(x);
}

inline __m256 __vectorcall sqrtSimd(__m256 x) {
	return _mm256_sqrt_ps(x);
}

/*

lnTest lower degree with calculated coefficients

max relative error 6.059741534792049577617945033125579357147216796875 on x = 1.000010013580322265625
correct       = 1.0013530186704881283911265643649102230483549647033214569091796875e-05
approximation = 7.06929349689744412899017333984375e-05
numbers between = 23871794

max absolute error 6.127357045926373757538385689258575439453125e-05 on x = 124169.90625
correct       = 11.72940611839731417376242461614310741424560546875
approximation = 11.7294673919677734375
numbers between = 64

max number between error 23871794 on x = 1.000010013580322265625

lnTest lower degree with modified a0

max relative error 0.003459028157338775964768640136526300921104848384857177734375 on x = 1.000010013580322265625
correct       = 1.0013530186704881283911265643649102230483549647033214569091796875e-05
approximation = 9.9788931038347072899341583251953125e-06
numbers between = 38084

max absolute error 0.0001219231723315061799439718015491962432861328125 on x = 6672.27001953125
correct       = 8.8057154137729174436799439718015491962432861328125
approximation = 8.8055934906005859375
numbers between = 128

max number between error 79565 on x = 0.9900100231170654296875

lnTest lower degree with modified a1

max relative error 6.06320093757560751868140869191847741603851318359375 on x = 1.000010013580322265625
correct       = 1.0013530186704881283911265643649102230483549647033214569091796875e-05
approximation = 7.072757580317556858062744140625e-05
numbers between = 23876555

max absolute error 0.0033990477987320133479443029500544071197509765625 on x = 131066.296875
correct       = 11.7834585571817367366520556970499455928802490234375
approximation = 11.78685760498046875
numbers between = 3564

max number between error 23876555 on x = 1.000010013580322265625

lnTest lower degree with modified a0, a1

max relative error 0.004880011178682267837525277087706854217685759067535400390625 on x = 1.94000995159149169921875
correct       = 0.66269310274841586316796337996493093669414520263671875
approximation = 0.66592705249786376953125
numbers between = 54257

max absolute error 0.0033383696311179988924777717329561710357666015625 on x = 131068.75
correct       = 11.7834772736794288761075222282670438289642333984375
approximation = 11.786815643310546875
numbers between = 3501

max number between error 4729267 on x = 0.9900100231170654296875

lnTest higher degree with calculated coefficients

max relative error 0.1276212564837866381139974691905081272125244140625 on x = 1.000010013580322265625
correct       = 1.0013530186704881283911265643649102230483549647033214569091796875e-05
approximation = 1.1291469490970484912395477294921875e-05
numbers between = 1405109

max absolute error 1.8215708177393707956071011722087860107421875e-06 on x = 127456.3828125
correct       = 11.7555294894643385106292043928988277912139892578125
approximation = 11.75553131103515625
numbers between = 2

max number between error 1405109 on x = 1.000010013580322265625

lnTest higher degree with modified a0

max relative error 0.00013849921478230266431996842158014260348863899707794189453125 on x = 1.000010013580322265625
correct       = 1.0013530186704881283911265643649102230483549647033214569091796875e-05
approximation = 1.001214332063682377338409423828125e-05
numbers between = 1525

max absolute error 3.09741412962694084853865206241607666015625e-06 on x = 3682.52001953125
correct       = 8.21135258471881712694084853865206241607666015625
approximation = 8.2113494873046875
numbers between = 3

max number between error 1525 on x = 1.000010013580322265625

lnTest higher degree with modified a1

max relative error 0.1277596761918874113117539081940776668488979339599609375 on x = 1.000010013580322265625
correct       = 1.0013530186704881283911265643649102230483549647033214569091796875e-05
approximation = 1.1292855560895986855030059814453125e-05
numbers between = 1406633

max absolute error 0.0001377019637107679272958193905651569366455078125 on x = 65516.48046875
correct       = 11.0900570001847267320727041806094348430633544921875
approximation = 11.0901947021484375
numbers between = 144

max number between error 1406633 on x = 1.000010013580322265625

lnTest higher degree with modified a0, a1

max relative error 0.00019737642604026920073283235534944424216519109904766082763671875 on x = 1.9700100421905517578125
correct       = 0.67803864029556037973378579408745281398296356201171875
approximation = 0.67817246913909912109375
numbers between = 2245

max absolute error 0.0001364234054523905115274828858673572540283203125 on x = 131003.953125
correct       = 11.7829827782547038594884725171141326427459716796875
approximation = 11.78311920166015625
numbers between = 143

max number between error 144755 on x = 0.9900100231170654296875
*/