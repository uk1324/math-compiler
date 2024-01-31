#include "executeFunction.hpp"
#include "os/os.hpp"
#include "utils/put.hpp"

void executeFunction(const CodeGenerator& codeGenerator, const std::vector<u8>& code, const std::vector<u8>& data, const float* inputArray, float* outputArray, i64 arrayElementCount) {
	auto codeBuffer = reinterpret_cast<u8*>(allocateExecutableMemory(code.size() + data.size()));
	if (codeBuffer == nullptr) {
		put("failed to allocate executable memory");
		exit(EXIT_FAILURE);
	}

	memcpy(codeBuffer, code.data(), code.size());
	const auto dataBuffer = codeBuffer + code.size();
	memcpy(dataBuffer, data.data(), data.size());
	codeGenerator.patchExecutableCodeRipAddresses(codeBuffer, data.data());

	ASSERT(uintptr_t(inputArray) % 32 == 0);
	ASSERT(uintptr_t(outputArray) % 32 == 0);
	using Function = void (*)(const float*, float*, i64);

	const auto function = reinterpret_cast<Function>(codeBuffer);
	function(inputArray, outputArray, arrayElementCount);
}

Real executeFunction(const CodeGenerator& codeGenerator, const std::vector<u8>& code, const std::vector<u8>& data, std::span<const float> arguments) {

	std::vector<YmmArgument> input(arguments.size());
	for (int i = 0; i < arguments.size(); i++) {
		input[i].x[0] = arguments[i];
	}
	YmmArgument output;
	executeFunction(codeGenerator, code, data, reinterpret_cast<float*>(input.data()), output.x, 1);
	return output.x[0];
}
