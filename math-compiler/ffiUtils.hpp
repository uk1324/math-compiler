#pragma once

#include <span>

float callSimdVectorCall(void* function, std::span<const float> inputs);