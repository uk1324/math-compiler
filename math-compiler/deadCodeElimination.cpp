#include "deadCodeElimination.hpp"
#include "utils/asserts.hpp"

void DeadCodeElimination::run(const std::vector<IrOp>& input, std::span<const FunctionParameter> parameters, std::vector<IrOp>& output) {

	output.clear();
	registerToDefinitionInstructionIndex.clear();
	for (i64 i = 0; i < input.size(); i++) {
		const auto& op = input[i];
		bool alreadyAssigned = false;
		bool usesInvalidDestination = false;

		callWithDestination(op, [this, &alreadyAssigned, &usesInvalidDestination, &i, &parameters](Register destination) {
			alreadyAssigned = !registerToDefinitionInstructionIndex.insert({ destination, i }).second;
			if (registerIsParamter(parameters, destination)) {
				usesInvalidDestination = true;
			}
		});

		if (alreadyAssigned || usesInvalidDestination) {
			// The code assumes that the each register is assigned once.
			ASSERT_NOT_REACHED();
			output = input;
			return;
		}
	}

	usefulRegisterWorklist.clear();
	isInstructionMarkedUseful.clear();
	isInstructionMarkedUseful.resize(input.size(), false);
	for (i64 i = 0; i < input.size(); i++) {
		const auto& irOp = input[i];
		if (const auto op = std::get_if<ReturnOp>(&irOp)) {
			usefulRegisterWorklist.push_back(op->returnedRegister);
			isInstructionMarkedUseful[i] = true;
		}
	}

	while (!usefulRegisterWorklist.empty()) {
		const auto reg = usefulRegisterWorklist.back();
		usefulRegisterWorklist.pop_back();

		if (registerIsParamter(parameters, reg)) {
			continue;
		}

		const auto instructionIndexIt = registerToDefinitionInstructionIndex.find(reg);
		if (instructionIndexIt == registerToDefinitionInstructionIndex.end()) {
			ASSERT_NOT_REACHED();
			output = input;
			return;
		}

		const auto instructionIndex = instructionIndexIt->second;
		if (isInstructionMarkedUseful[instructionIndex]) {
			continue;
		}
		isInstructionMarkedUseful[instructionIndex] = true;
		const auto instruction = input[instructionIndex];

		callWithUsedRegisters(instruction, [this](Register r) {
			usefulRegisterWorklist.push_back(r);
		});
	}

	for (i64 i = 0; i < input.size(); i++) {
		if (isInstructionMarkedUseful[i]) {
			const auto& op = input[i];
			output.push_back(op);
		}
	}
}
