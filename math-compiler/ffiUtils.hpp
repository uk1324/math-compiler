#pragma once

#include <span>

float callSimd(void* function, std::span<const float> inputs);