#include "deadCodeElimination.hpp"
#include "utils/asserts.hpp"

void DeadCodeElimination::run(const std::vector<IrOp>& input, std::span<const Variable> parameters, std::vector<IrOp>& output) {

	output.clear();
	registerToDefinitionInstructionIndex.clear();
	for (i64 i = 0; i < i64(input.size()); i++) {
		const auto& op = input[i];
		bool alreadyAssigned = false;

		callWithOutputRegisters(op, [this, &alreadyAssigned, &i](Register destination) {
			alreadyAssigned = !registerToDefinitionInstructionIndex.insert({ destination, i }).second;
		});

		if (alreadyAssigned) {
			// The code assumes that the each register is assigned once.
			ASSERT_NOT_REACHED();
			output = input;
			return;
		}
	}

	usefulRegisterWorklist.clear();
	isInstructionMarkedUseful.clear();
	isInstructionMarkedUseful.resize(input.size(), false);
	for (usize i = 0; i < input.size(); i++) {
		const auto& irOp = input[i];
		if (const auto op = std::get_if<ReturnOp>(&irOp)) {
			usefulRegisterWorklist.push_back(op->returnedRegister);
			isInstructionMarkedUseful[i] = true;
		}
	}

	while (!usefulRegisterWorklist.empty()) {
		const auto reg = usefulRegisterWorklist.back();
		usefulRegisterWorklist.pop_back();

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

		callWithInputRegisters(instruction, [this](Register r) {
			usefulRegisterWorklist.push_back(r);
		});
	}

	for (usize i = 0; i < input.size(); i++) {
		if (isInstructionMarkedUseful[i]) {
			const auto& op = input[i];
			output.push_back(op);
		}
	}
}
