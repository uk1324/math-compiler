#pragma once

#include "codeGenerator.hpp"
#include "machineCode.hpp"

struct alignas(32) YmmArgument {
	float x[8];
};

void executeFunction(const MachineCode& machineCode, const float* inputArray, float* outputArray, i64 arrayElementCount);

Real executeFunction(const MachineCode& machineCode, std::span<const float> arguments);
