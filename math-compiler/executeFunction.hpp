#pragma once

#include "codeGenerator.hpp"

struct alignas(32) YmmArgument {
	float x[8];
};

void executeFunction(const CodeGenerator& codeGenerator, const std::vector<u8>& code, const std::vector<u8>& data, const float* inputArray, float* outputArray, i64 arrayElementCount);

Real executeFunction(const CodeGenerator& codeGenerator, const std::vector<u8>& code, const std::vector<u8>& data, std::span<const float> arguments);
