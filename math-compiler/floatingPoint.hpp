#pragma once

#include "utils/ints.hpp"
#include <bit>

static constexpr u32 F32_SIGNIFICAND_BITS = 23;
static constexpr u32 F32_EXPONENT_BITS = 8;
static constexpr u32 F32_SIGN_BITS = 1;

static constexpr u32 F32_SIGN_MASK = 1 << 31;
static constexpr u32 F32_EXPONENT_MASK = 0xFF << (32 - F32_SIGN_BITS - F32_EXPONENT_BITS);
static constexpr u32 F32_SIGNIFICAND_MASK = 0xFF'FF'FF >> 1;

static constexpr u32 F32_EXPONENT_SHIFT = F32_SIGNIFICAND_BITS;
static constexpr u32 F32_SIGN_SHIFT = F32_SIGNIFICAND_BITS + F32_EXPONENT_BITS;

static constexpr u32 F32_EXPONENT_BIAS = 127;
static constexpr u32 F32_SIGNIFICAND_PRECISON_BITS = 24; // 23 + hidden bit

i32 f32GetExponent(float x);
float f32GetSignificand(float x);

inline float f32Make(bool sign, u8 exponent, u32 significand);

inline bool f32GetSign(float x) {
	return std::bit_cast<u32>(x) & F32_SIGN_BITS;
}

inline float f32Make(bool sign, u8 exponent, u32 significand) {
	return std::bit_cast<float>((u32(sign) << F32_SIGN_SHIFT) | 
		(u32(exponent) << F32_EXPONENT_SHIFT) | 
		(significand & F32_SIGNIFICAND_MASK));
}