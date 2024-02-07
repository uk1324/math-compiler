#pragma once

#include "ir.hpp"
#include "input.hpp"
#include <vector>
#include <unordered_map>
#include <span>

struct DeadCodeElimination {
	void run(const std::vector<IrOp>& input, std::span<const FunctionParameter> parameters, std::vector<IrOp>& output);

	std::unordered_map<Register, i64> registerToDefinitionInstructionIndex;
	std::vector<bool> isInstructionMarkedUseful;
	std::vector<Register> usefulRegisterWorklist;
};