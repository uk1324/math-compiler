#include "floatingPoint.hpp"

i32 f32GetExponent(float x) {
    return i32((std::bit_cast<u32>(x) & F32_EXPONENT_MASK) >> F32_EXPONENT_SHIFT) - i32(F32_EXPONENT_BIAS);
}

float f32GetSignificand(float x) {
    u32 b = std::bit_cast<u32>(x);
    b &= F32_SIGNIFICAND_MASK;
    b |= F32_EXPONENT_BIAS << F32_EXPONENT_SHIFT;
    return std::bit_cast<float>(b);
}
