#include "executeFunction.hpp"
#include "os/os.hpp"
#include "utils/put.hpp"

void executeFunction(const MachineCode& machineCode, const float* inputArray, float* outputArray, i64 arrayElementCount) {
	auto codeBuffer = reinterpret_cast<u8*>(allocateExecutableMemory(machineCode.code.size() + machineCode.data.size()));
	if (codeBuffer == nullptr) {
		put("failed to allocate executable memory");
		exit(EXIT_FAILURE);
	}

	memcpy(codeBuffer, machineCode.code.data(), machineCode.code.size());
	const auto dataBuffer = codeBuffer + machineCode.code.size();
	// TODO: Alignment?
	memcpy(dataBuffer, machineCode.data.data(), machineCode.data.size());
	/*machineCode.patchRipRelativeOperands(codeBuffer, machineCode.data.data());*/
	// Use the same buffer so the rip relative operands are not outside the i32 range.
	machineCode.patchRipRelativeOperands(codeBuffer, dataBuffer);

	ASSERT(uintptr_t(inputArray) % 32 == 0);
	ASSERT(uintptr_t(outputArray) % 32 == 0);
	using Function = void (*)(const float*, float*, i64);

	const auto function = reinterpret_cast<Function>(codeBuffer);
	function(inputArray, outputArray, arrayElementCount);
}

Real executeFunction(const MachineCode& machineCode, std::span<const float> arguments) {
	std::vector<YmmArgument> input(arguments.size());
	for (int i = 0; i < arguments.size(); i++) {
		input[i].x[0] = arguments[i];
	}
	YmmArgument output;
	output.x[0] = 123456;
	executeFunction(machineCode, reinterpret_cast<float*>(input.data()), output.x, 1);
	return output.x[0];
}